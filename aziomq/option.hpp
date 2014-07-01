/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_OPTION_HPP_
#define AZIOMQ_OPTION_HPP_

#include <zmq.h>
#include <boost/asio/buffer.hpp>
#include <boost/logic/tribool.hpp>

#include <vector>
#include <string>

namespace aziomq { namespace opt {
    // limits for user/aziomq-defined options (should be well outside of the valid ZMQ range)
    enum class limits : int {
        lib_min = 1000000,
        lib_ctx_min = lib_min,
        lib_ctx_max = lib_ctx_min + 9999,
        lib_socket_min,
        lib_socket_max = lib_socket_min + 9999,
        lib_max = lib_socket_max,
        user_min = 2000000,
        user_ctx_min = user_min,
        user_ctx_max = user_ctx_min + 9999,
        user_socket_min,
        user_socket_max = user_socket_min + 9999,
        user_max = user_socket_max
    };

    template<typename T, int N>
    struct base {
        using static_name = std::integral_constant<int, N>;
        using value_t = T;
        T value_;

        base() = default;
        base(T v) : value_(std::move(v)) { }

        int name() const { return N; }
        const void* data() const { return reinterpret_cast<const void*>(&value_); }
        void* data() { return reinterpret_cast<void*>(&value_); }
        size_t size() const { return sizeof(T); }

        void set(T value) { value_ = value; }
        T value() const { return value_; }
    };

    template<int N>
    using integer = base<int, N>;

    template<int N>
    using ulong_integer = base<uint64_t, N>;

    template<int N>
    struct boolean {
        using static_name = std::integral_constant<int, N>;
        int value_;

        boolean() : value_{ 0 } { }
        boolean(bool v) : value_{ v ? 1 : 0 } { }

        int name() const { return N; }
        const void* data() const { return reinterpret_cast<const void*>(&value_); }
        void* data() { return reinterpret_cast<void*>(&value_); }
        size_t size() const { return sizeof(int); }

        bool value() const { return value_; }
    };

    template<int N>
    struct binary {
        using static_name = std::integral_constant<int, N>;
        void* pv_;
        size_t size_;

        binary() : pv_(nullptr), size_(0) { }
        binary(void* pv, size_t size) : pv_(pv), size_(size) { }

        int name() const { return N; }
        const void* data() const { pv_; }
        void* data() { return pv_; }
        size_t size() const { size_; }
    };
} }

#endif // AZIOMQ_OPTION_HPP_
