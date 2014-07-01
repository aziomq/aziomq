/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include <aziomq/detail/context_ops.hpp>

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/system/error_code.hpp>

#include <string>
#include <iostream>
#include <exception>

#include "../assert.ipp"

void test_context() {
    auto ctx = aziomq::detail::context_ops::get_context();
    auto ctx2 = aziomq::detail::context_ops::get_context(true);
    BOOST_ASSERT_MSG(ctx != ctx2, "expecting ctx != ctx2");

    auto ctx3 = aziomq::detail::context_ops::get_context();
    BOOST_ASSERT_MSG(ctx == ctx3, "expecting ctx == ctx3");
}

void test_context_options() {
    auto ctx = aziomq::detail::context_ops::get_context();
    using io_threads = aziomq::detail::context_ops::io_threads;
    boost::system::error_code ec;
    aziomq::detail::context_ops::set_option(ctx, io_threads(2), ec);
    BOOST_ASSERT_MSG(!ec, "error setting io_threads option");

    io_threads res;
    aziomq::detail::context_ops::get_option(ctx, res, ec);
    BOOST_ASSERT_MSG(!ec, "error getting io_threads option");
    BOOST_ASSERT(res.value() == 2);
}

int main(int argc, char **argv) {
    std::cout << "Testing context operations...";
    try {
        test_context();
        test_context_options();
    } catch (std::exception const& e) {
        std::cout << "Failure\n" << e.what() << std::endl;
        return 1;
    }
    std::cout << "Success" << std::endl;
    return 0;
}
