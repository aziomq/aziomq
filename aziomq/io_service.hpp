/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_SERVICE_HPP_
#define AZIOMQ_IO_SERVICE_HPP_

#include "detail/socket_service.hpp"
#include "option.hpp"
#include "error.hpp"

#include <boost/asio/io_service.hpp>
#include <zmq.h>

namespace aziomq {
namespace io_service {
    using  service_type = detail::socket_service;

    using io_threads = detail::context_ops::io_threads;
    using max_sockets = detail::context_ops::max_sockets;
    using ipv6 = detail::context_ops::ipv6;

    /** \brief set options on the zeromq context.
     *  \tparam Option option type
     *  \param option Option option to set
     *  \remark Must be called before any sockets are created
     */
    template<typename Option>
    boost::system::error_code set_option(boost::asio::io_service & io_service,
                                         const Option & option,
                                         boost::system::error_code & ec) {

        return boost::asio::use_service<service_type>(io_service).set_option(option, ec);
    }

    /** \brief set options on the zeromq context.
     *  \tparam Option option type
     *  \param option Option option to set
     *  \remark Must be called before any sockets are created
     */
    template<typename Option>
    void set_option(boost::asio::io_service & io_service, const Option & option) {
        boost::system::error_code ec;
        if (set_option(io_service, option, ec))
            throw boost::system::system_error(ec);
    }

    /** \brief get option from zeromq context
     *  \tparam Option option type
     *  \param option Option te get
     *  \param ec boost::system::error_code
     */
    template<typename Option>
    boost::system::error_code get_option(boost::asio::io_service & io_service,
                                         Option & option,
                                         boost::system::error_code & ec) {
        return boost::asio::use_service<service_type>(io_service).get_option(option, ec);
    }

    /** \brief get option from zeromq context
     *  \tparam Option option type
     *  \param option Option te get
     *  \param ec boost::system::error_code
     */
    template<typename Option>
    void get_option(boost::asio::io_service & io_service, Option & option) {
        boost::system::error_code ec;
        if (get_option(io_service, option))
            throw boost::system::system_error(ec);
    }
} // namespace io_service
} // namespace aziomq
#endif // AZIOMQ_IO_SERVICE_HPP_
