/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_DETAIL_CONTEXT_OPS_HPP__
#define AZIOMQ_DETAIL_CONTEXT_OPS_HPP__

#include "../error.hpp"
#include "../option.hpp"

#include <boost/assert.hpp>
#include <boost/system/error_code.hpp>

#include <memory>

namespace aziomq {
namespace detail {
    struct context_ops {
        using context_type = std::shared_ptr<void>;

        using io_threads = opt::integer<ZMQ_IO_THREADS>;
        using max_sockets = opt::integer<ZMQ_MAXMSGSIZE>;
        using ipv6 = opt::boolean<ZMQ_IPV6>;

        static context_type get_context(bool create_new = false);

        template<typename Option>
        static boost::system::error_code set_option(context_type & ctx,
                                                    Option const& option,
                                                    boost::system::error_code & ec) {
            BOOST_ASSERT_MSG(ctx, "context must not be null");
            auto rc = zmq_ctx_set(ctx.get(), option.name(), option.value());
            if (!rc)
                ec = make_error_code();
            return ec;
        }

        template<typename Option>
        static boost::system::error_code get_option(context_type & ctx,
                                                    Option & option,
                                                    boost::system::error_code & ec) {
            BOOST_ASSERT_MSG(ctx, "context must not be null");
            auto rc = zmq_ctx_get(ctx.get(), option.name());
            if (rc < 0)
                return ec = make_error_code();
            option.set(rc);
            return ec;
        }
    };
} // namespace detail
} // namespace aziomq

#endif // AZIOMQ_DETAIL_CONTEXT_OPS_HPP__

