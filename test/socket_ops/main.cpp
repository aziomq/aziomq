#include <aziomq/detail/context_ops.hpp>
#include <aziomq/detail/socket_ops.hpp>

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/asio/buffer.hpp>

#include <array>
#include <iostream>

#include "../assert.ipp"

auto ctx = aziomq::detail::context_ops::get_context();

std::array<boost::asio::const_buffer, 2> snd_bufs = {{
    boost::asio::buffer("A"),
    boost::asio::buffer("B")
}};

std::string subj(const char* name) {
    return std::string("inproc://") + name;
}

void test_send_receive_inproc_discrete_calls() {
    boost::system::error_code ec;
    auto sb = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_ROUTER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::bind(sb, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    auto sc = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_DEALER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::connect(sc, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    // Send and receive one at a time
    aziomq::detail::socket_ops::send(snd_bufs, sc, ZMQ_SNDMORE, ec);
    BOOST_ASSERT(!ec);

    aziomq::message msg;
    // Identity comes first
    aziomq::detail::socket_ops::receive(msg, sb, 0, ec);
    BOOST_ASSERT(!ec);
    BOOST_ASSERT(msg.more());

    // Then first part
    aziomq::detail::socket_ops::receive(msg, sb, 0, ec);
    BOOST_ASSERT(!ec);
    BOOST_ASSERT(msg.more());

    // Finally second part
    aziomq::detail::socket_ops::receive(msg, sb, 0, ec);
    BOOST_ASSERT(!ec);
    BOOST_ASSERT(!msg.more());
}

void test_send_receive_inproc_mutable_bufseq() {
    boost::system::error_code ec;
    auto sb = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_ROUTER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::bind(sb, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    auto sc = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_DEALER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::connect(sc, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    // Send and receive all message parts as a mutable buffer sequence
    aziomq::detail::socket_ops::send(snd_bufs, sc, ZMQ_SNDMORE, ec);
    BOOST_ASSERT(!ec);

    std::array<char, 5> ident;
    std::array<char, 2> part_A;
    std::array<char, 2> part_B;

    std::array<boost::asio::mutable_buffer, 3> rcv_msg_seq = {{
        boost::asio::buffer(ident),
        boost::asio::buffer(part_A),
        boost::asio::buffer(part_B)
    }};
    aziomq::detail::socket_ops::receive(rcv_msg_seq, sb, 0, ec);
    BOOST_ASSERT(!ec);
    BOOST_ASSERT('A' == part_A[0]);
    BOOST_ASSERT('B' == part_B[0]);
}

void test_send_receive_inproc_msg_vect() {
    boost::system::error_code ec;
    auto sb = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_ROUTER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::bind(sb, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    auto sc = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_DEALER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::connect(sc, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    // Send and receive all message parts as a vector
    aziomq::detail::socket_ops::send(snd_bufs, sc, ZMQ_SNDMORE, ec);
    BOOST_ASSERT(!ec);

    aziomq::message_vector rcv_msgs;
    aziomq::detail::socket_ops::receive(rcv_msgs, sb, 0, ec);
    BOOST_ASSERT(!ec);
    BOOST_ASSERT(rcv_msgs.size() == 3);
}

void test_send_receive_inproc_not_enough_bufs() {
    boost::system::error_code ec;
    auto sb = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_ROUTER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::bind(sb, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);

    auto sc = aziomq::detail::socket_ops::create_socket(ctx, ZMQ_DEALER, ec);
    BOOST_ASSERT(!ec);
    aziomq::detail::socket_ops::connect(sc, subj(__PRETTY_FUNCTION__), ec);
    BOOST_ASSERT(!ec);
    // Verify that we get an error on multipart with too few bufs in seq
    aziomq::detail::socket_ops::send(snd_bufs, sc, ZMQ_SNDMORE, ec);
    BOOST_ASSERT(!ec);

    std::array<char, 5> ident;
    std::array<char, 2> part_A;

    std::array<boost::asio::mutable_buffer, 2> rcv_msg_seq_2 = {{
        boost::asio::buffer(ident),
        boost::asio::buffer(part_A)
    }};
    aziomq::detail::socket_ops::receive(rcv_msg_seq_2, sb, ZMQ_RCVMORE, ec);
    BOOST_ASSERT(ec);
}

int main(int argc, char **argv) {
    std::cout << "Testing basic socket operations...";
    try {
        test_send_receive_inproc_discrete_calls();
        test_send_receive_inproc_mutable_bufseq();
        test_send_receive_inproc_msg_vect();
        test_send_receive_inproc_not_enough_bufs();
    } catch (std::exception const& e) {
        std::cout << "Failure\n" << e.what() << std::endl;
        return 1;
    }
    std::cout << "Success" << std::endl;
    return 0;
}
