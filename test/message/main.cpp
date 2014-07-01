/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include <iostream>
#include <aziomq/message.hpp>

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/asio/buffer.hpp>

#include <string>
#include <algorithm>
#include <array>
#include <iterator>

#include "../assert.ipp"

void test_message_constructors() {
    // default init range has 0 size
    aziomq::message m;
    BOOST_ASSERT(m.size() == 0);

    // pre-sized message construction
    aziomq::message mm(42);
    BOOST_ASSERT(mm.size() == 42);

    // implicit construction from asio::const_buffer
    std::string s("This is a test");
    aziomq::message mstr(boost::asio::buffer(s));
    BOOST_ASSERT(s.size() == mstr.size());
    BOOST_ASSERT(s ==  mstr.string());

    // construction from string
    aziomq::message mmstr(s);
    BOOST_ASSERT(s == mmstr.string());
}

void test_message_buffer_operations() {
    aziomq::message mm(42);
    // implicit cast to const_buffer
    boost::asio::const_buffer b = mm;
    BOOST_ASSERT(boost::asio::buffer_size(b) == mm.size());

    // implicit cast to mutable_buffer
    boost::asio::mutable_buffer bb = mm;
    BOOST_ASSERT(boost::asio::buffer_size(bb) == mm.size());
}

void test_message_copy_operations() {
    aziomq::message m(42);
    aziomq::message mm(m);
    BOOST_ASSERT(m.size() == mm.size() && mm.size() == 42);

    aziomq::message mmm = m;
    BOOST_ASSERT(m.size() == mmm.size() && mmm.size() == 42);
}

void test_message_move_operations() {
    aziomq::message m;
    aziomq::message mm(42);

    // move assignment
    m = std::move(mm);
    BOOST_ASSERT(m.size() == 42);
    BOOST_ASSERT(mm.size() == 0);

    // move construction
    aziomq::message mmm(std::move(m));
    BOOST_ASSERT(m.size() == 0);
    BOOST_ASSERT(mmm.size() == 42);
}

void test_write_through_mutable_buffer() {
    aziomq::message m("This is a test");

    aziomq::message mm(m);
    boost::asio::mutable_buffer bb = mm;
    auto pstr = boost::asio::buffer_cast<char*>(bb);
    pstr[0] = 't';

    auto s = mm.string();
    BOOST_ASSERT(std::string("this is a test") == s);

    auto ss = m.string();
    BOOST_ASSERT(s != ss);
}

void test_message_sequence() {
    std::string foo("foo");
    std::string bar("bar");

    std::array<boost::asio::const_buffer, 2> bufs {{
        boost::asio::buffer(foo),
        boost::asio::buffer(bar)
    }};

    // make a message_vector from a range
    auto res = aziomq::to_message_vector(bufs);
    BOOST_ASSERT(res.size() == bufs.size());
    BOOST_ASSERT(foo == res[0].string());
    BOOST_ASSERT(bar == res[1].string());

    // implicit conversion
    res.push_back(boost::asio::buffer("BAZ"));
    BOOST_ASSERT(res.size() == bufs.size() + 1);

    // range of const_buffer -> range of message
    auto range = aziomq::const_message_range(bufs);
    BOOST_ASSERT(std::distance(std::begin(bufs), std::end(bufs)) ==
                 std::distance(std::begin(range), std::end(range)));

    auto it = std::begin(range);
    for(auto& buf : bufs) {
        BOOST_ASSERT(aziomq::message(buf) == *it++);
    }
}

int main(int argc, char **argv) {
    std::cout << "Testing message operations...";
    try {
        test_message_constructors();
        test_message_copy_operations();
        test_message_move_operations();
        test_message_buffer_operations();
        test_write_through_mutable_buffer();
        test_message_sequence();
    } catch (std::exception const& e) {
        std::cout << "Failure\n" << e.what() << std::endl;
        return 1;
    }
    std::cout << "Success" << std::endl;
    return 0;
}

