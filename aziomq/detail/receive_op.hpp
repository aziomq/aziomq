/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_RECEIVE_OP_HPP_
#define AZIOMQ_RECEIVE_OP_HPP_

#include "../error.hpp"
#include "../message.hpp"
#include "socket_ops.hpp"
#include "reactor_op.hpp"

#include <boost/asio/io_service.hpp>

#include <zmq.h>
#include <iterator>

namespace aziomq {
namespace detail {
template<typename MutableBufferSequence>
class receive_buffer_op_base : public reactor_op {
public:
    receive_buffer_op_base(MutableBufferSequence const& buffers,
                           flags_type flags,
                           complete_func_type complete_func)
        : reactor_op(select_func(buffers, flags), complete_func)
        , buffers_(buffers)
        , flags_(flags)
        , it_(std::begin(buffers))
        , end_(std::end(buffers))
        { }

    static bool do_perform_receive_more(reactor_op* base, socket_type & socket) {
        auto o = static_cast<receive_buffer_op_base*>(base);
        o->ec_ = boost::system::error_code();

        o->bytes_transferred_ += socket_ops::receive(o->buffers_, socket, o->flags_ | ZMQ_DONTWAIT, o->ec_);
        if (o->ec_)
            return !o->try_again();
        return true;
    }

    static bool do_perform(reactor_op* base, socket_type & socket) {
        auto o = static_cast<receive_buffer_op_base*>(base);
        o->ec_ = boost::system::error_code();

        o->msg_.rebuild();
        auto bt = socket_ops::receive(o->msg_, socket, o->flags_ | ZMQ_DONTWAIT, o->ec_);
        if (o->ec_)
            return !o->try_again();
        o->bytes_transferred_ += bt;
        return ++o->it_ == o->end_;
    }

protected:
    bool more() const {
        return ec_ == boost::system::errc::no_buffer_space && bytes_transferred_;
    }

private:
    static perform_func_type select_func(MutableBufferSequence const& buffers,
                                         socket_ops::flags_type flags) {
        if (std::distance(std::begin(buffers), std::end(buffers)) == 0)
            return nullptr;

        return (flags & ZMQ_RCVMORE) ? &receive_buffer_op_base::do_perform_receive_more
                                     : &receive_buffer_op_base::do_perform;
    }

    using const_iterator = typename MutableBufferSequence::const_iterator;

    MutableBufferSequence const& buffers_;
    flags_type flags_;
    const_iterator it_;
    const_iterator end_;
    message msg_;
};

template<typename MutableBufferSequence,
         typename Handler>
class receive_buffer_op : public receive_buffer_op_base<MutableBufferSequence> {
public:
    receive_buffer_op(MutableBufferSequence const& buffers,
                      Handler handler,
                      socket_ops::flags_type flags)
        : receive_buffer_op_base<MutableBufferSequence>(buffers, flags,
                                                        &receive_buffer_op::do_complete)
        , handler_(std::move(handler))
        { }

    static void do_complete(reactor_op* base,
                            const boost::system::error_code &,
                            size_t) {
        auto o = static_cast<receive_buffer_op*>(base);
        auto h = std::move(o->handler_);
        auto ec = o->ec_;
        auto bt = o->bytes_transferred_;
        delete o;
        h(ec, bt);
    }

private:
    Handler handler_;
};

template<typename MutableBufferSequence,
         typename Handler>
class receive_more_buffer_op : public receive_buffer_op_base<MutableBufferSequence> {
public:
    receive_more_buffer_op(MutableBufferSequence const& buffers,
                           Handler handler,
                           socket_ops::flags_type flags)
        : receive_buffer_op_base<MutableBufferSequence>(buffers, flags,
                                                        &receive_more_buffer_op::do_complete)
        , handler_(std::move(handler))
        { }

    static void do_complete(reactor_op* base,
                            const boost::system::error_code &,
                            size_t) {
        auto o = static_cast<receive_more_buffer_op*>(base);
        auto h = std::move(o->handler_);
        auto ec = o->ec_;
        auto bt = o->bytes_transferred_;
        delete o;
        h(ec, bt);
    }

private:
    Handler handler_;
};

class receive_op_base : public reactor_op {
public:
    receive_op_base(message & msg,
                    socket_ops::flags_type flags,
                    complete_func_type complete_func)
        : reactor_op(&receive_op_base::do_perform, complete_func)
        , msg_(msg)
        , flags_(flags)
        {
            msg_.rebuild();
        }

    static bool do_perform(reactor_op* base, socket_type & socket) {
        auto o = static_cast<receive_op_base*>(base);
        o->ec_ = boost::system::error_code();

        o->bytes_transferred_ = socket_ops::receive(o->msg_, socket, o->flags_ | ZMQ_DONTWAIT, o->ec_);
        if (o->ec_)
            return !o->try_again();
        return true;
    }

private:
    message & msg_;
    flags_type flags_;
};

template<typename Handler>
class receive_op : public receive_op_base {
public:
    receive_op(void* socket,
               message & msg,
               Handler handler,
               socket_ops::flags_type flags,
               complete_func_type complete_func)
        : receive_op_base(msg, flags, &receive_op::do_complete)
        , handler_(std::move(handler))
        { }

    static void do_complete(reactor_op* base,
                            const boost::system::error_code &,
                            size_t) {
        auto o = static_cast<receive_op*>(base);
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
#endif // AZIOMQ_RECEIVE_OP_HPP_


