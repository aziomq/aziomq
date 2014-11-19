/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include <aziomq/socket.hpp>
#include <aziomq/util/scope_guard.hpp>

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/asio/buffer.hpp>

#include <array>
#include <thread>

#include "../assert.ipp"

std::array<boost::asio::const_buffer, 2> snd_bufs = {{
    boost::asio::buffer("A"),
    boost::asio::buffer("B")
}};

std::string subj(const char* name) {
    return std::string("inproc://") + name;
}

void test_set_get_options() {
    boost::asio::io_service ios;

    aziomq::socket s(ios, ZMQ_ROUTER);

    // set/get_option are generic, works for one and all...
    aziomq::socket::rcv_hwm in_hwm(42);
    s.set_option(in_hwm);

    aziomq::socket::rcv_hwm out_hwm;
    s.get_option(out_hwm);
    BOOST_ASSERT_MSG(in_hwm.value() == out_hwm.value(), "in_hwm != out_hwm");
}

void test_send_receive_sync() {
    boost::asio::io_service ios;

    aziomq::socket sb(ios, ZMQ_ROUTER);
    sb.bind(subj(__PRETTY_FUNCTION__));

    aziomq::socket sc(ios, ZMQ_DEALER);
    sc.connect(subj(__PRETTY_FUNCTION__));

    sc.send(snd_bufs, ZMQ_SNDMORE);

    aziomq::message msg;
    auto size = sb.receive(msg);

    BOOST_ASSERT_MSG(msg.more(), "more");

    size = sb.receive(msg, 0);
    BOOST_ASSERT_MSG(size == boost::asio::buffer_size(snd_bufs[0]), "buffer size");
    BOOST_ASSERT_MSG(msg.more(), "more");

    size = sb.receive(msg, 0);
    BOOST_ASSERT_MSG(size == boost::asio::buffer_size(snd_bufs[1]), "buffer size");
    BOOST_ASSERT_MSG(!msg.more(), "!more");

    sc.send(snd_bufs, ZMQ_SNDMORE);

    std::array<char, 5> ident;
    std::array<char, 2> a;
    std::array<char, 2> b;

    std::array<boost::asio::mutable_buffer, 3> rcv_bufs = {{
        boost::asio::buffer(ident),
        boost::asio::buffer(a),
        boost::asio::buffer(b)
    }};

    size = sb.receive(rcv_bufs, ZMQ_RCVMORE);
    BOOST_ASSERT_MSG(size == 9, "buffer size");
}

void test_send_receive_async(bool is_speculative) {
    boost::asio::io_service ios_b;
    boost::asio::io_service ios_c;

    aziomq::socket sb(ios_b, ZMQ_ROUTER);
    sb.set_option(aziomq::socket::allow_speculative(is_speculative));
    sb.bind(subj(__PRETTY_FUNCTION__));

    aziomq::socket sc(ios_c, ZMQ_DEALER);
    sc.set_option(aziomq::socket::allow_speculative(is_speculative));
    sc.connect(subj(__PRETTY_FUNCTION__));

    boost::system::error_code ecc;
    size_t btc = 0;
    sc.async_send(snd_bufs, [&] (boost::system::error_code const& ec, size_t bytes_transferred) {
        ecc = ec;
        btc = bytes_transferred;
        ios_c.stop();
    }, ZMQ_SNDMORE);

    std::array<char, 5> ident;
    std::array<char, 2> a;
    std::array<char, 2> b;

    std::array<boost::asio::mutable_buffer, 3> rcv_bufs = {{
        boost::asio::buffer(ident),
        boost::asio::buffer(a),
        boost::asio::buffer(b)
    }};

    boost::system::error_code ecb;
    size_t btb = 0;
    sb.async_receive(rcv_bufs, [&](boost::system::error_code const& ec, size_t bytes_transferred) {
        ecb = ec;
        btb = bytes_transferred;
        ios_b.stop();
    }, ZMQ_RCVMORE);

    ios_c.run();
    ios_b.run();

    BOOST_ASSERT_MSG(!ecc, "!ecc");
    BOOST_ASSERT_MSG(btc == 4, "btc != 4");
    BOOST_ASSERT_MSG(!ecb, "!ecb");
    BOOST_ASSERT_MSG(btb == 9, "btb != 9");
}

void test_send_receive_async_threads(bool optimize_single_threaded) {
    boost::asio::io_service ios_b;
    aziomq::socket sb(ios_b, ZMQ_ROUTER, optimize_single_threaded);
    sb.bind(subj(__PRETTY_FUNCTION__));

    boost::asio::io_service ios_c;
    aziomq::socket sc(ios_c, ZMQ_DEALER, optimize_single_threaded);
    sc.connect(subj(__PRETTY_FUNCTION__));

    boost::system::error_code ecc;
    size_t btc = 0;
    std::thread tc([&] {
        sc.async_send(snd_bufs, [&] (boost::system::error_code const& ec, size_t bytes_transferred) {
            ecc = ec;
            btc = bytes_transferred;
            ios_c.stop();
        }, ZMQ_SNDMORE);
        ios_c.run();
    });

    boost::system::error_code ecb;
    size_t btb = 0;
    std::thread tb([&] {
        std::array<char, 5> ident;
        std::array<char, 2> a;
        std::array<char, 2> b;

        std::array<boost::asio::mutable_buffer, 3> rcv_bufs = {{
            boost::asio::buffer(ident),
            boost::asio::buffer(a),
            boost::asio::buffer(b)
        }};

        sb.async_receive(rcv_bufs, [&](boost::system::error_code const& ec, size_t bytes_transferred) {
            ecb = ec;
            btb = bytes_transferred;
            ios_b.stop();
        }, ZMQ_RCVMORE);
        ios_b.run();
    });

    tc.join();
    tb.join();
    BOOST_ASSERT_MSG(!ecc, "!ecc");
    BOOST_ASSERT_MSG(btc == 4, "btc != 4");
    BOOST_ASSERT_MSG(!ecb, "!ecb");
    BOOST_ASSERT_MSG(btb == 9, "btb != 9");
}

void test_send_receive_message_async() {
    boost::asio::io_service ios_b;
    boost::asio::io_service ios_c;

    aziomq::socket sb(ios_b, ZMQ_ROUTER);
    sb.bind(subj(__PRETTY_FUNCTION__));

    aziomq::socket sc(ios_c, ZMQ_DEALER);
    sc.connect(subj(__PRETTY_FUNCTION__));

    boost::system::error_code ecc;
    size_t btc = 0;
    sc.async_send(snd_bufs, [&] (boost::system::error_code const& ec, size_t bytes_transferred) {
        ecc = ec;
        btc = bytes_transferred;
        ios_c.stop();
    }, ZMQ_SNDMORE);

    std::array<char, 5> ident;
    std::array<char, 2> a;
    std::array<char, 2> b;

    boost::system::error_code ecb;
    size_t btb = 0;
    sb.async_receive([&](boost::system::error_code const& ec, aziomq::message & msg, size_t bytes_transferred) {
        SCOPE_EXIT { ios_b.stop(); };
        ecb = ec;
        if (ecb)
            return;
        btb += bytes_transferred;
        msg.buffer_copy(boost::asio::buffer(ident));

        if (msg.more()) {
            btb += sb.receive(msg, ZMQ_RCVMORE, ecb);
            if (ecb)
                return;
            msg.buffer_copy(boost::asio::buffer(a));
        }

        if (msg.more()) {
            btb += sb.receive(msg, 0, ecb);
            if (ecb)
                return;
            msg.buffer_copy(boost::asio::buffer(b));
        }
    });

    ios_c.run();
    ios_b.run();

    BOOST_ASSERT_MSG(!ecc, "!ecc");
    BOOST_ASSERT_MSG(btc == 4, "btc != 4");
    BOOST_ASSERT_MSG(!ecb, "!ecb");
    BOOST_ASSERT_MSG(btb == 9, "btb != 9");
}

void test_send_receive_message_more_async() {
    boost::asio::io_service ios_b;
    boost::asio::io_service ios_c;

    aziomq::socket sb(ios_b, ZMQ_ROUTER);
    sb.bind(subj(__PRETTY_FUNCTION__));

    aziomq::socket sc(ios_c, ZMQ_DEALER);
    sc.connect(subj(__PRETTY_FUNCTION__));

    boost::system::error_code ecc;
    size_t btc = 0;
    sc.async_send(snd_bufs, [&] (boost::system::error_code const& ec, size_t bytes_transferred) {
        ecc = ec;
        btc = bytes_transferred;
        ios_c.stop();
    }, ZMQ_SNDMORE);

    std::array<char, 5> ident;
    std::array<char, 2> a;
    std::array<char, 2> b;

    std::array<boost::asio::mutable_buffer, 2> rcv_bufs = {{
        boost::asio::buffer(a),
        boost::asio::buffer(b)
    }};

    boost::system::error_code ecb;
    size_t btb = 0;
    sb.async_receive([&](boost::system::error_code const& ec, aziomq::message & msg, size_t bytes_transferred) {
        SCOPE_EXIT { ios_b.stop(); };
        ecb = ec;
        if (ecb)
            return;
        btb += bytes_transferred;
        msg.buffer_copy(boost::asio::buffer(ident));

        if (!msg.more())
            return;

        aziomq::message_vector v;
        btb += sb.receive_more(v, 0, ecb);
        if (ecb)
            return;
        auto it = std::begin(v);
        for (auto&& buf : rcv_bufs)
            (*it++).buffer_copy(buf);
    });

    ios_c.run();
    ios_b.run();

    BOOST_ASSERT_MSG(!ecc, "!ecc");
    BOOST_ASSERT_MSG(btc == 4, "btc != 4");
    BOOST_ASSERT_MSG(!ecb, "!ecb");
    BOOST_ASSERT_MSG(btb == 9, "btb != 9");
}

int main(int argc, char **argv) {
    std::cout << "Testing socket operations...";
    try {
        test_set_get_options();
        test_send_receive_sync();
        test_send_receive_async(true);
        test_send_receive_async(false);
        for (auto i = 0; i < 100; i++)
            test_send_receive_async_threads(true);
        for (auto i = 0; i < 100; i++)
            test_send_receive_async_threads(false);
        test_send_receive_message_async();
        test_send_receive_message_more_async();
    } catch (std::exception const& e) {
        std::cout << "Failure\n" << e.what() << std::endl;
        return 1;
    }
    std::cout << "Success" << std::endl;
    return 0;
}

