/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_TRANSPORT_HPP_
#define AZIOMQ_IO_TRANSPORT_HPP_

#include "option.hpp"

#include <boost/mpl/set.hpp>
#include <boost/mpl/joint_view.hpp>

namespace aziomq {
namespace transport {
    struct connection_oriented {
        using reconnect_ivl = opt::reconnect_ivl;
        using reconnect_ivl_max = opt::reconnect_ivl_max;
        using backlog = opt::backlog;
        using immediate = opt::immediate;

        using base_options = boost::mpl::set<reconnect_ivl, reconnect_ivl_max, backlog, immediate>;
    };

    struct tcp : connection_oriented {
        using tcp_keepalive = opt::tcp_keepalive;
        using tcp_keepalive_idle = opt::tcp_keepalive_idle;
        using tcp_keepalive_cnt = opt::tcp_keepalive_cnt;
        using tcp_keepalive_intvl = opt::tcp_keepalive_intvl;
        using tcp_accept_filter = opt::tcp_accept_filter;
        using ipv6 = opt::ipv6;
        using ipv4_only = opt::ipv4_only;
        using plain_server = opt::plain_server;
        using plain_username = opt::plain_username;
        using plain_password = opt::plain_password;
        using curve_server = opt::curve_server;
        using curve_publickey = opt::curve_publickey;
        using curve_privatekey = opt::curve_privatekey;
        using zap_domain = opt::zap_domain;

        using protocol_options = boost::mpl::set<tcp_keepalive, tcp_keepalive_idle, tcp_keepalive_cnt,
                                                 tcp_keepalive_intvl, tcp_accept_filter, ipv6, ipv4_only,
                                                 plain_server, plain_username, plain_password,
                                                 curve_server, curve_publickey, curve_privatekey,
                                                 zap_domain>;

        using allowable_options = boost::mpl::joint_view<base_options, protocol_options>;

        static constexpr char const* prefix() { return "tcp"; }
    };

    struct inproc {
        using allowable_options = boost::mpl::set<>;

        static constexpr char const* prefix() { return "inproc"; }
    };

    struct ipc : connection_oriented {
        using allowable_options = base_options;
        static constexpr char const* prefix() { return "ipc"; }
    };

    struct multicast {
        using rate = opt::rate;
        using recovery_ivl = opt::recovery_ivl;
        using multicast_hops = opt::multicast_hops;

        using base_options = boost::mpl::set<rate, recovery_ivl, multicast_hops>;
    };

    struct pgm : multicast {
        using allowable_options = base_options;
        static constexpr char const* prefix() { return "pgm"; }
    };

    struct epgm : multicast {
        using allowable_options = base_options;
        static constexpr char const* prefix() { return "epgm"; }
    };

    using all_transports = boost::mpl::set<tcp, ipc, inproc, pgm, epgm>;
} // namespace transport
} // namespace aziomq
#endif // AZIOMQ_IO_TRANSPORT_HPP_

