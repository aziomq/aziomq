/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_SEND_OP_HPP_
#define AZIOMQ_SEND_OP_HPP_
#include "../error.hpp"
#include "../message.hpp"
#include "socket_ops.hpp"
#include "reactor_op.hpp"

#include <boost/asio/io_service.hpp>

#include <zmq.h>
#include <iterator>

#include <iostream>

namespace aziomq {
namespace detail {

template<typename ConstBufferSequence>
class send_buffer_op_base : public reactor_op {
public:
    send_buffer_op_base(ConstBufferSequence const& buffers,
                        flags_type flags,
                        complete_func_type complete_func)
        : reactor_op(select_func(buffers, flags), complete_func)
        , buffers_(buffers)
        , flags_(flags)
        , it_(std::begin(buffers))
        , end_(std::end(buffers))
        { }

    static bool do_perform_send_more(reactor_op* base, socket_type & socket) {
        auto o = static_cast<send_buffer_op_base*>(base);
        o->ec_ = boost::system::error_code();
        o->bytes_transferred_ += socket_ops::send(o->buffers_, socket, o->flags_ | ZMQ_DONTWAIT, o->ec_);
        if (o->ec_) {
            std::cout << "send error " << o->ec_ << std::endl;
            return !o->try_again();
        }
        return true;
    }

    static bool do_perform(reactor_op* base, socket_type & socket) {
        auto o = static_cast<send_buffer_op_base*>(base);
        o->ec_ = boost::system::error_code();

        auto bt = socket_ops::send(*o->it_, socket, o->flags_ | ZMQ_DONTWAIT, o->ec_);
        if (o->ec_)
            return !o->try_again();
        o->bytes_transferred_ += bt;
        return ++o->it_ == o->end_;
    }

private:
    static perform_func_type select_func(ConstBufferSequence const& buffers,
                                         socket_ops::flags_type flags) {
        if (!std::distance(std::begin(buffers), std::end(buffers)))
            return nullptr;

        return (flags & ZMQ_SNDMORE) ? &send_buffer_op_base::do_perform_send_more
                                     : &send_buffer_op_base::do_perform;
    }

    using const_iterator = typename ConstBufferSequence::const_iterator;
    ConstBufferSequence const& buffers_;
    flags_type flags_;
    const_iterator it_;
    const_iterator end_;
};

template<typename ConstBufferSequence,
         typename Handler>
class send_buffer_op : public send_buffer_op_base<ConstBufferSequence> {
public:
    send_buffer_op(ConstBufferSequence const& buffers,
                   Handler handler,
                   reactor_op::flags_type flags)
        : send_buffer_op_base<ConstBufferSequence>(buffers, flags,
                                                   &send_buffer_op::do_complete)
        , handler_(std::move(handler))
    { }

    static void do_complete(reactor_op* base,
                            const boost::system::error_code &,
                            size_t) {
        auto o = static_cast<send_buffer_op*>(base);
        auto h = std::move(o->handler_);
        auto ec = o->ec_;
        auto bt = o->bytes_transferred_;
        delete o;

        h(ec, bt);
    }

private:
    Handler handler_;
};

class send_op_base : public reactor_op {
public:
    send_op_base(message msg,
                 flags_type flags,
                 complete_func_type complete_func)
        : reactor_op(&send_op_base::do_perform, complete_func)
        , msg_(std::move(msg))
        , flags_(flags)
        { }

    static bool do_perform(reactor_op* base, socket_type & socket) {
        auto o = static_cast<send_op_base*>(base);
        o->ec_ = boost::system::error_code();
        o->bytes_transferred_ = socket_ops::send(o->msg_, socket, o->flags_ | ZMQ_DONTWAIT, o->ec_);

        if (o->ec_)
            return !o->try_again(); // some other error
        return true;
    };

private:
    message msg_;
    flags_type flags_;
};

template<typename Handler>
class send_op : public send_op_base {
public:
    send_op(message msg,
            Handler handler,
            flags_type flags)
        : send_op_base(std::move(msg), flags, &send_op::do_complete)
        , handler_(std::move(handler))
    { }

    static void do_complete(reactor_op* base,
                            const boost::system::error_code &,
                            size_t) {
        auto o = static_cast<send_op*>(base);
        auto h = std::move(o->handler_);
        auto ec = o->ec_;
        auto bt = o->bytes_transferred_;
        delete o;
        h(ec, bt);
    }

private:
    Handler handler_;
};

} // namespace detail
} // namespace aziomq
#endif // AZIOMQ_SEND_OP_HPP_


