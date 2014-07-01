/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include <aziomq/detail/socket_service.hpp>

namespace aziomq {
namespace detail {
boost::asio::io_service::id socket_service::id;

void socket_service::shutdown_service() {
    ctx_.reset();
}
} // namespace detail
} // namespace aziomq

