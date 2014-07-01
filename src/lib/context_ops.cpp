/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include <aziomq/detail/context_ops.hpp>

#include <zmq.h>

#include <memory>
#include <mutex>

namespace aziomq {
namespace detail {
using lock_type = std::lock_guard<std::mutex>;
lock_type::mutex_type mtx;
std::weak_ptr<void> ctx;

context_ops::context_type ctx_new() {
    return context_ops::context_type(zmq_ctx_new(), [](void *p) {
        zmq_ctx_term(p);
    });
}

context_ops::context_type context_ops::get_context(bool create_new) {
    if (create_new) return ctx_new();

    lock_type lock(mtx);
    auto p = ctx.lock();
    if (!p) ctx = p = ctx_new();
    return p;
}
} // namespace detail
} // namespace aziomq
