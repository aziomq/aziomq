/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_SOCKET_POLICY_HPP_
#define AZIOMQ_IO_SOCKET_POLICY_HPP_

#include "transport.hpp"

#include <boost/format.hpp>
#include <boost/mpl/for_each.hpp>

#include <string>
#include <ostream>
#include <type_traits>

namespace aziomq {
    namespace detail {
        struct socket_service;
    }

    struct endpoint {
        template<typename T>
        endpoint(T, std::string const& address)
            : value_(boost::str(boost::format("%1%://%2%") % T::prefix() % address)) {
            new (&impl_) model<T>();
        }

        endpoint(endpoint const& that)
            : value_(that.value_) {
            reinterpret_cast<concept const*>(&that.impl_)->clone_into(&impl_);
        }

        endpoint(endpoint && that)
            : value_(std::move(that.value_)) {
            reinterpret_cast<concept const*>(&that.impl_)->clone_into(&impl_);
        }

        endpoint & operator=(endpoint const& rhs) {
            value_ = rhs.value_;
            reinterpret_cast<concept const*>(&rhs.impl_)->clone_into(&impl_);
            return *this;
        }

        endpoint & operator=(endpoint && rhs) {
            value_ = std::move(rhs.value_);
            reinterpret_cast<concept const*>(&rhs.impl_)->clone_into(&impl_);
            return *this;
        }

        bool operator==(endpoint const& rhs) const { return value_ == rhs.value_; }
        bool operator!=(endpoint const& rhs) const { return value_ != rhs.value_; }
        bool operator<(endpoint const& rhs) const { return value_ < rhs.value_; }
        bool operator<=(endpoint const& rhs) const { return value_ <= rhs.value_; }
        bool operator>(endpoint const& rhs) const { return value_ > rhs.value_; }
        bool operator>=(endpoint const& rhs) const { return value_ >= rhs.value_; }

        template<typename T>
        bool supports_option() const {
            return reinterpret_cast<concept const*>(&impl_)->supports_option(T::static_name::value);
        }

        std::string to_string() const { return value_; }

        friend std::ostream& operator<<(std::ostream& stm, endpoint const& that) { return stm << that.to_string(); }

    private:
        std::string value_;

        struct concept {
            virtual ~concept() = default;
            virtual void clone_into(void*) const = 0;
            virtual char const* prefix() const = 0;
            virtual bool supports_option(int) const = 0;
        };
        std::aligned_storage<sizeof(concept)> impl_;

        struct visitor {
            int v_; bool r_;
            visitor(int v) : v_(v), r_(false) { }

            template<typename N>
            void operator()(N n) { r_ |= n == v_; };

            operator bool() const { return r_; }
        };

        template<typename T>
        struct model : concept {
            virtual void clone_into(void* pv) const { new (pv) model<T>(*this); }
            char const* prefix() const final { return T::prefix(); }
            bool supports_option(int v) const final {
                visitor vv(v);
                boost::mpl::for_each<T::allowable_options>(vv);
                return vv;
            }
        };
    };
} // namespace aziomq
#endif // AZIOMQ_IO_SOCKET_POLICY_HPP_

