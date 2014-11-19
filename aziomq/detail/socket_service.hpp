/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_DETAIL_SOCKET_SERVICE_HPP__
#define AZIOMQ_DETAIL_SOCKET_SERVICE_HPP__
#include "../error.hpp"
#include "../message.hpp"
#include "../option.hpp"
#include "../util/scope_guard.hpp"
#include "context_ops.hpp"
#include "socket_ops.hpp"
#include "socket_ext.hpp"
#include "reactor_op.hpp"
#include "send_op.hpp"
#include "receive_op.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/system/system_error.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/container/flat_map.hpp>

#include <memory>
#include <typeindex>
#include <string>
#include <vector>
#include <tuple>
#include <mutex>

namespace aziomq {
namespace detail {
    struct socket_service : boost::asio::io_service::service {
        using socket_type = socket_ops::socket_type;
        using native_handle_type= socket_ops::raw_socket_type;
        using stream_descriptor = socket_ops::stream_descriptor;
        using endpoint_type = socket_ops::endpoint_type;
        using flags_type = socket_ops::flags_type;
        using more_result_type = socket_ops::more_result_type;
        using context_type = context_ops::context_type;
        using op_queue_type = boost::intrusive::list<reactor_op,
                                    boost::intrusive::member_hook<
                                        reactor_op,
                                        boost::intrusive::list_member_hook<>,
                                        &reactor_op::member_hook_
                                    >>;
        using exts_type = boost::container::flat_map<std::type_index, socket_ext>;
        using allow_speculative = opt::boolean<static_cast<int>(opt::limits::lib_socket_min)>;

        static boost::asio::io_service::id id;

        enum class shutdown_type {
            none = 0,
            send,
            receive
        };

        enum op_type : unsigned {
            read_op = 0,
            write_op = 1,
            max_ops = 2
        };

        struct per_descriptor_data {
            bool optimize_single_threaded_ = false;
            socket_type socket_;
            stream_descriptor sd_;
            mutable std::mutex mutex_;
            bool in_speculative_completion_ = false;
            bool scheduled_ = false;
            bool allow_speculative_ = true;
            shutdown_type shutdown_ = shutdown_type::none;
            exts_type exts_;
            endpoint_type endpoint_;
            std::array<op_queue_type, max_ops> op_queue_;

            ~per_descriptor_data() {
                sd_.release();
                for (auto& ext : exts_)
                    ext.second.on_remove();
            }

            void do_open(boost::asio::io_service & ios,
                         context_type & ctx,
                         int type,
                         bool optimize_single_threaded,
                         boost::system::error_code & ec) {
                BOOST_ASSERT_MSG(!socket_, "socket already open");
                socket_ = socket_ops::create_socket(ctx, type, ec);
                if (ec) return;

                sd_ = socket_ops::get_stream_descriptor(ios, socket_, ec);
                if (ec) return;

                optimize_single_threaded_ = optimize_single_threaded;
            }

            bool perform_ops(int evs, op_queue_type & ops) {
                bool res  = false;
                int filter[max_ops] = { ZMQ_POLLIN, ZMQ_POLLOUT };
                for (auto i = 0; i < max_ops; ++i) {
                    if ((evs & filter[i]) && !op_queue_[i].empty()) {
                        if (op_queue_[i].front().do_perform(socket_)) {
                            op_queue_[i].pop_front_and_dispose([&ops](reactor_op * op) {
                                ops.push_back(*op);
                            });
                        }
                    }
                    res |= !op_queue_[i].empty();
                }
                return res;
            }

            void cancel_ops(boost::system::error_code const& ec, op_queue_type & ops) {
                for (auto i = 0; i < max_ops; ++i) {
                    while (!op_queue_[i].empty()) {
                        op_queue_[i].front().ec_ = make_error_code(boost::system::errc::operation_canceled);
                        op_queue_[i].pop_front_and_dispose([&ops](reactor_op * op) {
                            ops.push_back(*op);
                        });
                    }
                }
            }

            bool is_single_thread() const { return optimize_single_threaded_; }

            void lock() const {
                if (optimize_single_threaded_) return;
                mutex_.lock();
            }

            void try_lock() const {
                if (optimize_single_threaded_) return;
                mutex_.try_lock();
            }

            void unlock() const {
                if (optimize_single_threaded_) return;
                mutex_.unlock();
            }

        };
        using unique_lock = std::unique_lock<per_descriptor_data>;
        using implementation_type = std::shared_ptr<per_descriptor_data>;

        explicit socket_service(boost::asio::io_service & ios)
            : boost::asio::io_service::service(ios)
            , ctx_(context_ops::get_context())
        { }

        void shutdown_service() override;

        context_type context() const { return ctx_; }

        void construct(implementation_type & impl) {
            impl = std::make_shared<per_descriptor_data>();
        }

        void move_construct(implementation_type & impl,
                            socket_service &,
                            implementation_type & other) {
            impl = std::move(other);
        }

        void move_assign(implementation_type & impl,
                         socket_service &,
                         implementation_type & other) {
            impl = std::move(other);
        }

        boost::system::error_code do_open(implementation_type & impl,
                                          int type,
                                          bool optimize_single_threaded,
                                          boost::system::error_code & ec) {
            BOOST_ASSERT_MSG(impl, "impl");
            unique_lock l{ *impl };
            impl->do_open(get_io_service(), ctx_, type, optimize_single_threaded, ec);
            if (ec)
                impl.reset();
            return ec;
        }

        void destroy(implementation_type & impl) {
            impl.reset();
        }

        native_handle_type native_handle(implementation_type & impl) {
            BOOST_ASSERT_MSG(impl, "impl");
            unique_lock l{ *impl };
            return impl->socket_.get();
        }

        template<typename Extension>
        bool associate_ext(implementation_type & impl, Extension ext) {
            BOOST_ASSERT_MSG(impl, "impl");
            unique_lock l{ *impl };
            exts_type::iterator it;
            bool res;
            std::tie(it, res) = impl->exts_.emplace(std::type_index(typeid(Extension)),
                                                    socket_ext(std::move(ext)));
            if (res)
                it->second.on_install(get_io_service(), impl->socket_.get());
            return res;
        }

        template<typename Extension>
        bool remove_ext(implementation_type & impl) {
            BOOST_ASSERT_MSG(impl, "impl");
            unique_lock l{ *impl };
            auto it = impl->exts_.find(std::type_index(typeid(Extension)));
            if (it != std::end(impl->exts_)) {
                it->second.on_remove();
                impl->exts_.erase(it);
                return true;
            }
            return false;
        }

        template<typename Option>
        boost::system::error_code set_option(Option const& option,
                                             boost::system::error_code & ec) {
            return context_ops::set_option(ctx_, option, ec);
        }

        template<typename Option>
        boost::system::error_code get_option(Option & option,
                                             boost::system::error_code & ec) {
            return context_ops::get_option(ctx_, option, ec);
        }

        template<typename Option>
        boost::system::error_code set_option(implementation_type & impl,
                                             Option const& option,
                                             boost::system::error_code & ec) {
            unique_lock l{ *impl };
            switch (option.name()) {
            case allow_speculative::static_name::value :
                    ec = boost::system::error_code();
                    impl->allow_speculative_ = option.value_;
                break;
            default:
                for (auto& ext : impl->exts_) {
                    if (ext.second.set_option(option, ec)) {
                        if (ec.value() == boost::system::errc::not_supported) continue;
                        return ec;
                    }
                }
                ec = boost::system::error_code();
                socket_ops::set_option(impl->socket_, option, ec);
            }
            return ec;
        }

        template<typename Option>
        boost::system::error_code get_option(implementation_type & impl,
                                             Option & option,
                                             boost::system::error_code & ec) {
            unique_lock l{ *impl };
            switch (option.name()) {
            case allow_speculative::static_name::value :
                    ec = boost::system::error_code();
                    option.value_ = impl->allow_speculative_;
                break;
            default:
                for (auto& ext : impl->exts_) {
                    if (ext.second.get_option(option, ec)) {
                        if (ec.value() == boost::system::errc::not_supported) continue;
                        return ec;
                    }
                }
                ec = boost::system::error_code();
                socket_ops::get_option(impl->socket_, option, ec);
            }
            return ec;
        }

        void cancel(implementation_type & impl) {
            op_queue_type ops;
            {
                unique_lock l{ *impl };
                impl->cancel_ops(reactor_op::canceled(), ops);
            }

            while (!ops.empty())
                ops.pop_front_and_dispose(reactor_op::do_complete);
        }

        boost::system::error_code shutdown(implementation_type & impl,
                                           shutdown_type what,
                                           boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (impl->shutdown_ < what)
                impl->shutdown_ = what;
            else
                ec = make_error_code(boost::system::errc::operation_not_permitted);
            return ec;
        }

        endpoint_type endpoint(implementation_type const& impl,
                               boost::system::error_code & ec) const {
            unique_lock l{ *impl };
            if (!impl->endpoint_.empty())
                ec = make_error_code(boost::system::errc::not_connected);
            return impl->endpoint_;
        }

        boost::system::error_code bind(implementation_type & impl,
                                       socket_ops::endpoint_type endpoint,
                                       boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (socket_ops::bind(impl->socket_, std::move(endpoint), ec))
                return ec;
            impl->endpoint_ = endpoint;
            return ec;
        }

        boost::system::error_code connect(implementation_type & impl,
                                          socket_ops::endpoint_type endpoint,
                                          boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (socket_ops::connect(impl->socket_, std::move(endpoint), ec))
                return ec;
            impl->endpoint_ = endpoint;
            return ec;
        }

        template<typename ConstBufferSequence>
        size_t send(implementation_type & impl,
                    ConstBufferSequence const& buffers,
                    flags_type flags,
                    boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (!is_shutdown(impl, op_type::write_op, ec))
                return socket_ops::send(buffers, impl->socket_, flags, ec);
            return 0;
        }

        size_t send(implementation_type & impl,
                    message const& msg,
                    flags_type flags,
                    boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (!is_shutdown(impl, op_type::write_op, ec))
                return socket_ops::send(msg, impl->socket_, flags, ec);
            return 0;
        }

        template<typename MutableBufferSequence>
        size_t receive(implementation_type & impl,
                       MutableBufferSequence const& buffers,
                       flags_type flags,
                       boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (!is_shutdown(impl, op_type::read_op, ec))
                return socket_ops::receive(buffers, impl->socket_, flags, ec);
            return 0;
        }

        size_t receive(implementation_type & impl,
                       message & msg,
                       flags_type flags,
                       boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (!is_shutdown(impl, op_type::read_op, ec))
                return socket_ops::receive(msg, impl->socket_, flags, ec);
            return 0;
        }

        size_t receive_more(implementation_type & impl,
                       message_vector & vec,
                       flags_type flags,
                       boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (!is_shutdown(impl, op_type::read_op, ec))
                return socket_ops::receive_more(vec, impl->socket_, flags, ec);
            return 0;
        }

        template<typename MutableBufferSequence>
        more_result_type receive_more(implementation_type & impl,
                                      MutableBufferSequence const& buffers,
                                      flags_type flags,
                                      boost::system::error_code & ec) {
            unique_lock l{ *impl };
            if (!is_shutdown(impl, op_type::read_op, ec)) {
                bool more = false;
                auto res = socket_ops::receive(buffers, impl->socket_, flags | ZMQ_RCVMORE, ec);
                if (ec == boost::system::errc::no_buffer_space && res) {
                    more = true;
                    ec = boost::system::error_code();
                }
                return std::make_pair(res, more);
            } else {
                return std::make_pair(0, false);
            }
        }

        using reactor_op_ptr = std::unique_ptr<reactor_op>;
        template<typename T, typename... Args>
        void enqueue(implementation_type & impl, op_type o, Args&&... args) {
            reactor_op_ptr p{ new T(std::forward<Args>(args)...) };
            boost::system::error_code ec = enqueue(impl, o, p);
            if (ec) {
                BOOST_ASSERT_MSG(p, "op ptr");
                p->ec_ = ec;
                reactor_op::do_complete(p.release());
            }
        }

        private:
            context_type ctx_;

            bool is_shutdown(implementation_type & impl, op_type o, boost::system::error_code & ec) {
                if (is_shutdown(o, impl->shutdown_)) {
                    ec = make_error_code(boost::system::errc::operation_not_permitted);
                    return true;
                }
                return false;
            }

            static bool is_shutdown(op_type o, shutdown_type what) {
                return (o == op_type::write_op) ? what >= shutdown_type::send
                                                : what >= shutdown_type::receive;
            }

            struct reactor_handler {
                std::weak_ptr<per_descriptor_data> per_descriptor_data_;

                reactor_handler(implementation_type per_descriptor_data)
                    : per_descriptor_data_(per_descriptor_data)
                { }

                void operator()(boost::system::error_code const& e, size_t) const {
                    boost::system::error_code ec{ e };
                    op_queue_type ops;
                    if(auto p = per_descriptor_data_.lock()) {
                        unique_lock l{ *p };
                        p->scheduled_ = false;
                        int evs = 0;

                        if (!ec)
                            evs = socket_ops::get_events(p->socket_, ec);

                        if (!ec)
                            p->scheduled_ = p->perform_ops(evs, ops);
                        else
                            p->cancel_ops(ec, ops);

                        if (p->scheduled_)
                            schedule(p);
                    }
                    while (!ops.empty())
                        ops.pop_front_and_dispose(reactor_op::do_complete);
                }

                static void schedule(implementation_type & impl) {
                    reactor_handler handler(impl);
                    int evs = 0;
                    boost::system::error_code ec;
                    evs = socket_ops::get_events(impl->socket_, ec);
                    if (evs || ec) {
                        impl->sd_->get_io_service().post([handler, ec] { handler(ec, 0); });
                    } else {
                        impl->sd_->async_read_some(boost::asio::null_buffers(),
                                                std::move(handler));
                    }
                }
            };

            struct deferred_completion {
                std::weak_ptr<per_descriptor_data> owner_;
                reactor_op *op_;

                deferred_completion(implementation_type owner,
                                    reactor_op_ptr op)
                    : owner_(owner)
                    , op_(op.release())
                { }

                void operator()() {
                    reactor_op::do_complete(op_);
                    if (auto p = owner_.lock()) {
                        unique_lock l{ *p };
                        p->in_speculative_completion_ = false;
                    }
                }

                friend
                bool asio_handler_is_continuation(deferred_completion* handler) { return true; }
            };

            boost::system::error_code enqueue(implementation_type & impl,
                                            op_type o, reactor_op_ptr & op) {
                unique_lock l{ *impl };
                boost::system::error_code ec;
                if (is_shutdown(impl, o, ec))
                    return ec;

                // we have at most one speculative completion in flight at any time
                if (impl->allow_speculative_ && !impl->in_speculative_completion_) {
                    // attempt to execute speculatively when the op_queue is empty
                    if (impl->op_queue_[o].empty()) {
                        if (op->do_perform(impl->socket_)) {
                            impl->in_speculative_completion_ = true;
                            l.unlock();
                            get_io_service().post(deferred_completion(impl, std::move(op)));
                            return ec;
                        }
                    }
                }
                impl->op_queue_[o].push_back(*op.release());

                if (!impl->scheduled_) {
                    impl->scheduled_ = true;
                    l.unlock();
                    reactor_handler::schedule(impl);
                }
                return ec;
            }
        };
    } // namespace detail
    } // namespace aziomq
#endif // AZIOMQ_DETAIL_SOCKET_SERVICE_HPP__

