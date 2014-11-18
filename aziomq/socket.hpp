/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_SOCKET_HPP_
#define AZIOMQ_IO_SOCKET_HPP_

#include "error.hpp"
#include "option.hpp"
#include "io_service.hpp"
#include "message.hpp"
#include "detail/send_op.hpp"
#include "detail/receive_op.hpp"

#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include <type_traits>

namespace aziomq {

/** \brief Implement an asio-like socket over a zeromq socket
 *  \remark sockets are movable, but not copyable
 */
class socket :
    public boost::asio::basic_io_object<io_service::service_type> {

public:
    using native_handle_type = service_type::native_handle_type;
    using endpoint_type = service_type::endpoint_type;
    using flags_type = service_type::flags_type;
    using more_result_type = service_type::more_result_type;
    using shutdown_type = service_type::shutdown_type;

    // socket options
    using allow_speculative = service_type::allow_speculative;
    using type = opt::integer<ZMQ_TYPE>;
    using rcv_more = opt::integer<ZMQ_RCVMORE>;
    using rcv_hwm = opt::integer<ZMQ_RCVHWM>;
    using snd_hwm = opt::integer<ZMQ_SNDHWM>;
    using affinity = opt::ulong_integer<ZMQ_AFFINITY>;
    using subscribe = opt::binary<ZMQ_SUBSCRIBE>;
    using unsubscribe = opt::binary<ZMQ_UNSUBSCRIBE>;
    using identity = opt::binary<ZMQ_IDENTITY>;
    using rate = opt::integer<ZMQ_RATE>;
    using recovery_ivl = opt::integer<ZMQ_RECOVERY_IVL>;
    using snd_buf = opt::integer<ZMQ_SNDBUF>;
    using rcv_buf = opt::integer<ZMQ_RCVBUF>;
    using linger = opt::integer<ZMQ_LINGER>;
    using reconnect_ivl = opt::integer<ZMQ_RECONNECT_IVL>;
    using reconnect_ivl_max = opt::integer<ZMQ_RECONNECT_IVL_MAX>;
    using backlog = opt::integer<ZMQ_BACKLOG>;
    using max_msgsize = opt::integer<ZMQ_MAXMSGSIZE>;
    using multicast_hops = opt::integer<ZMQ_MULTICAST_HOPS>;
    using rcv_timeo = opt::integer<ZMQ_RCVTIMEO>;
    using snd_timeo = opt::integer<ZMQ_SNDTIMEO>;
    using ipv6 = opt::boolean<ZMQ_IPV6>;
    using immediate = opt::boolean<ZMQ_IMMEDIATE>;
    using router_mandatory = opt::boolean<ZMQ_ROUTER_MANDATORY>;
    using router_raw = opt::boolean<ZMQ_ROUTER_RAW>;
    using probe_router = opt::boolean<ZMQ_PROBE_ROUTER>;
    using xpub_verbose = opt::boolean<ZMQ_XPUB_VERBOSE>;
    using req_correlate = opt::boolean<ZMQ_REQ_CORRELATE>;
    using req_relaxed = opt::boolean<ZMQ_REQ_RELAXED>;
    using last_endpoint = opt::binary<ZMQ_LAST_ENDPOINT>;
    using tcp_keepalive = opt::integer<ZMQ_TCP_KEEPALIVE>;
    using tcp_keepalive_idle = opt::integer<ZMQ_TCP_KEEPALIVE_IDLE>;
    using tcp_keepalive_cnt = opt::integer<ZMQ_TCP_KEEPALIVE_CNT>;
    using tcp_keepalive_intvl = opt::integer<ZMQ_TCP_KEEPALIVE_INTVL>;
    using tcp_accept_filter = opt::binary<ZMQ_TCP_ACCEPT_FILTER>;
    using plain_server = opt::integer<ZMQ_PLAIN_SERVER>;
    using plain_username = opt::binary<ZMQ_PLAIN_USERNAME>;
    using plain_password = opt::binary<ZMQ_PLAIN_PASSWORD>;
    using curve_server = opt::boolean<ZMQ_CURVE_SERVER>;
    using curve_publickey = opt::binary<ZMQ_CURVE_PUBLICKEY>;
    using curve_privatekey = opt::binary<ZMQ_CURVE_SECRETKEY>;
    using zap_domain = opt::binary<ZMQ_ZAP_DOMAIN>;
    using conflate = opt::boolean<ZMQ_CONFLATE>;

    /** \brief socket constructor
     *  \param ios reference to an asio::io_service
     *  \param s_type int socket type
     *      For socket types see the zeromq documentation
     *  \param optimize_single_threaded bool
     *      Defaults to false - socket is not optimized for a single
     *      threaded io_service
     *  \remarks
     *      ZeroMQ's socket types are not thread safe. Because there is no
     *      guarantee that the supplied io_service is running in a single
     *      thread, Aziomq by default wraps all calls to ZeroMQ APIs with
     *      a mutex. If you can guarantee that a single thread has called
     *      io_service.run() you may bypass the mutex by passing true for
     *      optimize_single_threaded.
     */
    explicit socket(boost::asio::io_service& ios,
                    int type,
                    bool optimize_single_threaded = false) :
        boost::asio::basic_io_object<io_service::service_type>(ios) {
        boost::system::error_code ec;
        if (get_service().do_open(implementation, type, optimize_single_threaded, ec))
            throw boost::system::system_error(ec);
    }

    socket(const socket &) = delete;
    socket & operator=(const socket &) = delete;

    /** \brief Accept incoming connections on this socket
     *  \param addr std::string zeromq URI to bind
     *  \param ec error_code to capture error
     *  \see http://api.zeromq.org/4-1:zmq-bind
     */
    boost::system::error_code bind(std::string addr,
                                   boost::system::error_code & ec) {
        return get_service().bind(implementation, std::move(addr), ec);
    }

    /** \brief Accept incoming connections on this socket
     *  \param addr std::string zeromq URI to bind
     *  \throw boost::system::system_error
     *  \see http://api.zeromq.org/4-1:zmq-bind
     */
    void bind(std::string addr) {
        boost::system::error_code ec;
        if (bind(std::move(addr), ec))
            throw boost::system::system_error(ec);
    }

    /** \brief Create outgoing connection from this socket
     *  \param addr std::string zeromq URI of endpoint
     *  \param ec error_code to capture error
     *  \see http://api.zeromq.org/4-1:zmq-connect
     */
    boost::system::error_code connect(std::string addr,
                                      boost::system::error_code & ec) {
        return get_service().connect(implementation, std::move(addr), ec);
    }

    /** \brief Create outgoing connection from this socket
     *  \param addr std::string zeromq URI of endpoint
     *  \throw boost::system::system_error
     *  \see http://api.zeromq.org/4-1:zmq-connect
     */
    void connect(std::string addr) {
        boost::system::error_code ec;
        if (connect(addr, ec))
            throw boost::system::system_error(ec);
    }

    /** \brief return endpoint addr supplied to bind or connect
     *  \returns std::string
     *  \remarks Return value will be empty if bind or connect has
     *  not yet been called/succeeded.  If multiple calls to connect
     *  or bind have occured, this call wil return only the most recent
     */
    endpoint_type endpoint(boost::system::error_code & ec) const {
        return get_service().endpoint(implementation, ec);
    }

    /** \brief Set an option on a socket
     *  \tparam Option type which must conform the asio SettableSocketOption concept
     *  \param ec error_code to capture error
     *  \param opt T option to set
     */
    template<typename Option>
    boost::system::error_code set_option(Option const& opt,
                                         boost::system::error_code & ec) {
        return get_service().set_option(implementation, opt, ec);
    }

    /** \brief Set an option on a socket
     *  \tparam T type which must conform the asio SettableSocketOption concept
     *  \param opt T option to set
     *  \throw boost::system::system_error
     */
    template<typename Option>
    void set_option(Option const& opt) {
        boost::system::error_code ec;
        if (set_option(opt, ec))
            throw boost::system::system_error(ec);
    }

    /** \brief Get an option from a socket
     *  \tparam T must conform to the asio GettableSocketOption concept
     *  \param opt T option to get
     *  \param ec error_code to capture error
     */
    template<typename Option>
    boost::system::error_code get_option(Option & opt,
                                         boost::system::error_code & ec) {
        return get_service().get_option(implementation, opt, ec);
    }

    /** \brief Get an option from a socket
     *  \tparam T must conform to the asio GettableSocketOption concept
     *  \param opt T option to get
     *  \throw boost::system::system_error
     */
    template<typename Option>
    void get_option(Option & opt) {
        boost::system::error_code ec;
        if (get_option(opt, ec))
            throw boost::system::system_error(ec);
    }

    /** \brief Receive some data from the socket
     *  \tparam MutableBufferSequence
     *  \param buffers buffer(s) to fill on receive
     *  \param flags specifying how the receive call is to be made
     *  \param ec set to indicate what error, if any, occurred
     *  \remark
     *  If buffers is a sequence of buffers, and flags has ZMQ_RCVMORE
     *  set, this call will fill the supplied sequence with message
     *  parts from a multipart message. It is possible that there are
     *  more message parts than supplied buffers, or that an individual
     *  message part's size may exceed an individual buffer in the
     *  sequence. In either case, the call will return with ec set to
     *  no_buffer_space. It is the callers responsibility to issue
     *  additional receive calls to collect the remaining message parts.
     *
     * \remark
     * If flags does not have ZMQ_RCVMORE set, this call will synchronously
     * receive a message for each buffer in the supplied sequence
     * before returning. This will work for multi-part messages as well, but
     * will not verify that the number of buffers supplied is sufficient to
     * receive all message parts.
     */
    template<typename MutableBufferSequence>
    std::size_t receive(MutableBufferSequence const& buffers,
                        flags_type flags,
                        boost::system::error_code & ec) {
        return get_service().receive(implementation, buffers, flags, ec);
    }

    /** \brief Receive some data from the socket
     *  \tparam MutableBufferSequence
     *  \param buffers buffer(s) to fill on receive
     *  \param flags flags specifying how the receive call is to be made
     *  \throw boost::system::system_error
     *  \remark
     *  If buffers is a sequence of buffers, and flags has ZMQ_RCVMORE
     *  set, this call will fill the supplied sequence with message
     *  parts from a multipart message. It is possible that there are
     *  more message parts than supplied buffers, or that an individual
     *  message part's size may exceed an individual buffer in the
     *  sequence. In either case, the call will return with ec set to
     *  no_buffer_space. It is the callers responsibility to issue
     *  additional receive calls to collect the remaining message parts.
     *
     * \remark
     * If flags does not have ZMQ_RCVMORE set, this call will synchronously
     * receive a message for each buffer in the supplied sequence
     * before returning.
     */
    template<typename MutableBufferSequence>
    std::size_t receive(const MutableBufferSequence & buffers,
                        flags_type flags = 0) {
        boost::system::error_code ec;
        auto res = receive(buffers, flags, ec);
        if (ec)
            throw boost::system::system_error(ec);
        return res;
    }

    /** \brief Receive some data from the socket
     *  \param msg raw_message to fill on receive
     *  \param flags specifying how the receive call is to be made
     *  \param rebuild_message bool
     *  \returns byte's received
     *  \remarks
     *      This variant provides access to a type that thinly wraps the underlying
     *      libzmq message type.  The rebuild_message flag indicates whether the
     *      message provided should be closed and rebuilt.  This is useful when
     *      reusing the same message instance across multiple receive operations.
     */
    std::size_t receive(message & msg,
                        flags_type flags,
                        boost::system::error_code & ec,
                        bool rebuild_message = false) {
        if (rebuild_message)
            msg.rebuild();
        return get_service().receive(implementation, msg, flags, ec);
    }

    /** \brief Receive some data from the socket
     *  \param msg message to fill on receive
     *  \param flags specifying how the receive call is to be made
     *  \param ec set to indicate what error, if any, occurred
     *  \param rebuild_message bool
     *  \remarks
     *      This variant provides access to a type that thinly wraps the underlying
     *      libzmq message type.  The rebuild_message flag indicates whether the
     *      message provided should be closed and rebuilt.  This is useful when
     *      reusing the same message instance across multiple receive operations.
     */
    std::size_t receive(message & msg,
                        flags_type flags = 0,
                        bool rebuild_message = false) {
        boost::system::error_code ec;
        auto res = receive(msg, flags, ec, rebuild_message);
        if (ec)
            throw boost::system::system_error(ec);
        return res;
    }

    /** \brief Receive some data as part of a multipart message from the socket
     *  \tparam MutableBufferSequence
     *  \param buffers buffer(s) to fill on receive
     *  \param flags specifying how the receive call is to be made
     *  \param ec set to indicate what error, if any, occurred
     *  \return pair<size_t, bool>
     *  \remark
     *  Works as for receive() with flags containing ZMQ_RCVMORE but returns
     *  a pair containing the number of bytes transferred and a boolean flag
     *  which if true, indicates more message parts are available on the
     *  socket.
     */
    template<typename MutableBufferSequence>
    more_result_type receive_more(MutableBufferSequence const& buffers,
                                  flags_type flags,
                                  boost::system::error_code & ec) {
        return get_service().receive_more(implementation, buffers, flags, ec);
    }

    /** \brief Receive some data as part of a multipart message from the socket
     *  \tparam MutableBufferSequence
     *  \param buffers buffer(s) to fill on receive
     *  \param flags specifying how the receive call is to be made
     *  \return pair<size_t, bool>
     *  \throw boost::system::system_error
     *  \remark
     *  Works as for receive() with flags containing ZMQ_RCV_MORE but returns
     *  a pair containing the number of bytes transferred and a boolean flag
     *  which if true, indicates more message parts are available on the
     *  socket.
     */
    template<typename MutableBufferSequence>
    more_result_type receive_more(MutableBufferSequence const& buffers,
                                  flags_type flags = 0) {
        boost::system::error_code ec;
        auto res = receive_more(buffers, flags, ec);
        if (ec)
            throw boost::system::system_error(ec);
        return res;
    }

    /** \brief Send some data from the socket
     *  \tparam ConstBufferSequence
     *  \param buffers buffer(s) to send
     *  \param flags specifying how the send call is to be made
     *  \param ec set to indicate what, if any, error occurred
     *  \remark
     *  If buffers is a sequence of buffers, and flags has ZMQ_SNDMORE
     *  set, this call will construct a multipart message from the supplied
     *  buffer sequence.
     *
     * \remark
     * If flags does not have ZMQ_RCVMORE set, this call will synchronously
     * send an individual message for each buffer in the supplied sequence before
     * returning.
     */
    template<typename ConstBufferSequence>
    std::size_t send(ConstBufferSequence const& buffers,
                     flags_type flags,
                     boost::system::error_code & ec) {
        return get_service().send(implementation, buffers, flags, ec);
    }

    /** \brief Send some data to the socket
     *  \tparam ConstBufferSequence
     *  \param buffers buffer(s) to send
     *  \param flags specifying how the send call is to be made
     *  \throw boost::system::system_error
     *  \remark
     *  If buffers is a sequence of buffers, and flags has ZMQ_SNDMORE
     *  set, this call will construct a multipart message from the supplied
     *  buffer sequence.
     *
     * \remark
     * If flags does not have ZMQ_RCVMORE set, this call will synchronously
     * send a message for each buffer in the supplied sequence before
     * returning.
     */
    template<typename ConstBufferSequence>
    std::size_t send(ConstBufferSequence const& buffers,
                     flags_type flags = 0) {
        boost::system::error_code ec;
        auto res = send(buffers, flags, ec);
        if (ec)
            throw boost::system::system_error(ec);
        return res;
    }

    /** \brief Send some data from the socket
     *  \param msg raw_message to send
     *  \param flags specifying how the send call is to be made
     *  \param ec set to indicate what, if any, error occurred
     *  \remarks
     *      This variant provides access to a type that thinly wraps the underlying
     *      libzmq message type.
     */
    std::size_t send(message const& msg,
                     flags_type flags,
                     boost::system::error_code & ec) {
        return get_service().send(implementation, msg, flags, ec);
    }

    /** \brief Send some data from the socket
     *  \param msg raw_message to send
     *  \param flags specifying how the send call is to be made
     *  \remarks
     *      This variant provides access to a type that thinly wraps the underlying
     *      libzmq message type.
     */
    std::size_t send(message const& msg,
                     flags_type flags = 0) {
        boost::system::error_code ec;
        auto res = get_service().send(implementation, msg, flags, ec);
        if (res)
            throw boost::system::error_code(ec);
        return res;
    }

    /** \brief Initiate an async receive operation.
     *  \tparam MutableBufferSequence
     *  \tparam ReadHandler must conform to the asio ReadHandler concept
     *  \param buffers buffer(s) to fill on receive
     *  \param handler ReadHandler
     *  \remark
     *  If buffers is a sequence of buffers, and flags has ZMQ_RCVMORE
     *  set, this call will fill the supplied sequence with message
     *  parts from a multipart message. It is possible that there are
     *  more message parts than supplied buffers, or that an individual
     *  message part's size may exceed an individual buffer in the
     *  sequence. In either case, the handler will be called with ec set
     *  to no_buffer_space. It is the callers responsibility to issue
     *  additional receive calls to collect the remaining message parts.
     *  If any message parts remain after the call to the completion
     *  handler returns, the socket handler will throw an exception to
     *  the io_service forcing this socket to be removed from the poll
     *  set. The socket is largely unusable after this, in particular
     *  any subsequent call to (async_)send/receive will raise an exception.
     *
     * \remark
     *  If flags does not have ZMQ_RCVMORE set, this call will asynchronously
     *  receive a message for each buffer in the supplied sequence before
     *  calling the supplied handler.
     */
    template<typename MutableBufferSequence,
             typename ReadHandler>
    void async_receive(MutableBufferSequence const& buffers,
                       ReadHandler handler,
                       flags_type flags = 0) {
        using type = detail::receive_buffer_op<MutableBufferSequence, ReadHandler>;
        get_service().enqueue<type>(implementation, service_type::op_type::read_op,
                                    buffers, std::forward<ReadHandler>(handler), flags);
    }

    /** \brief Initiate an async receive operation.
     *  \tparam MutableBufferSequence
     *  \tparam ReadMoreHandler must conform to the ReadMoreHandler concept
     *  \param buffers buffer(s) to fill on receive
     *  \param handler ReadMoreHandler
     *  \remark
     *  The ReadMoreHandler concept has the following interface
     *      struct ReadMoreHandler {
     *          void operator()(const boost::system::error_code & ec,
     *                          more_result result);
     *      }
     *  \remark
     *  Works as for async_receive() with flags containing ZMQ_RCV_MORE but
     *  does not error if more parts remain than buffers supplied.  The
     *  completion handler will be called with a more_result indicating the
     *  number of bytes transferred thus far, and flag indicating whether
     *  more message parts remain. The handler may then make synchronous
     *  receive_more() calls to collect the remaining message parts.
     */
    template<typename MutableBufferSequence,
             typename ReadMoreHandler>
    void async_receive_more(MutableBufferSequence const& buffers,
                            ReadMoreHandler handler,
                            flags_type flags = 0) {
        using type = detail::receive_more_buffer_op<MutableBufferSequence, ReadMoreHandler>;
        get_service().enqueue<type>(implementation, service_type::op_type::read_op,
                                    buffers, std::forward<ReadMoreHandler>(handler), flags);
    }

    /** \brief Initate an async receive operation
     *  \tparam MessageReadHandler must conform to the MessageReadHandler concept
     *  \param handler ReadHandler
     *  \param flags int flags
     *  \remark
     *  The MessageReadHandler concept has the following interface
     *  struct MessageReadHandler {
     *      void operator()(const boost::system::error_code & ec,
     *                      message & msg,
     *                      size_t bytes_transferred);
     *  }
     *  \remark
     *  Multipart messages can be handled by checking the status of more() on the
     *  supplied message, and calling synchronous receive() to retrieve subsequent
     *  message parts. If a handler wishes to retain the supplied message after the
     *  MessageReadHandler returns, it must make an explicit copy or move of
     *  the message.
     */
    template<typename MessageReadHandler>
    void async_receive(MessageReadHandler handler,
                       flags_type flags = 0) {
        using type = detail::receive_op<MessageReadHandler>;
        get_service().enqueue<type>(implementation, service_type::op_type::read_op,
                                    std::forward<MessageReadHandler>(handler), flags);
    }

    /** \brief Initiate an async send operation
     *  \tparam ConstBufferSequence must conform to the asio
     *          ConstBufferSequence concept
     *  \tparam WriteHandler must conform to the asio
     *          WriteHandler concept
     *  \param flags specifying how the send call is to be made
     *  \remark
     *  If buffers is a sequence of buffers, and flags has ZMQ_SNDMORE
     *  set, this call will construct a multipart message from the supplied
     *  buffer sequence.
     *
     *  \remark
     *  If flags does not specify ZMQ_SNDMORE this call will asynchronously
     *  send each buffer in the sequence as an individual message.
     */
    template<typename ConstBufferSequence,
             typename WriteHandler>
    void async_send(ConstBufferSequence const& buffers,
                    WriteHandler handler,
                    flags_type flags = 0) {
        using type = detail::send_buffer_op<ConstBufferSequence, WriteHandler>;
        get_service().enqueue<type>(implementation, service_type::op_type::write_op,
                                    buffers, std::forward<WriteHandler>(handler), flags);
    }

    /** \brief Initate an async send operation
     *  \tparam WriteHandler must conform to the asio ReadHandler concept
     *  \param msg message reference
     *  \param handler ReadHandler
     *  \param flags int flags
     *  \remarks
     *      This variant provides access to a type that thinly wraps the underlying
     *      libzmq message type.
     */
    template<typename WriteHandler>
    void async_send(message const& msg,
                    WriteHandler handler,
                    flags_type flags = 0) {
        using type = detail::send_op<WriteHandler>;
        get_service().enqueue<type>(implementation, service_type::op_type::write_op,
                                    msg, std::forward<WriteHandler>(handler), flags);
    }

    /** \brief Initiate shutdown of socket
     *  \param what shutdown_type
     */
    boost::system::error_code shutdown(shutdown_type what,
                                       boost::system::error_code & ec) {
        return get_service().shutdown(implementation, what, ec);
    }

    void shutdown(shutdown_type what) {
        boost::system::error_code ec;
        if (shutdown(what, ec))
            throw boost::system::system_error(ec);
    }

    /** \brief Allows the caller to associate an instance which conforms to the
     *  following protocol:
     *      struct handler {
     *          void on_install(io_service& ios, void* socket);
     *          void on_remove();
     *
     *          template<typename Option>
     *          error_code set_option(Option const&, error_code &);
     *
     *          template<typename Option>
     *          error_code get_option(Option &, error_code &);
     *      };
     *  with the supplied socket.
     *  \remark set/get_option allows the caller to interract with the handler
     *  from the socket interface. If the handler does not support the supplied,
     *  option, the handler should return errc::not_supported.  Handler options
     *  should satisfy the following protocol:
     *      struct option {
     *          int name() const;
     *          const void* data() const;
     *          void* data();
     *          size_t size() const;
     *          void resize(size_t);
     *      };
     *  \returns true if the handler was installed, false if a handler is already
     *  associated.
     */
    template<typename T>
    bool associate_ext(T ext) {
        return get_service().associate_ext<T>(implementation, std::move(ext));
    }

    template<typename T>
    bool remove_ext() {
        return get_service().remove_ext<T>(implementation);
    }

    /** \brief as for set_option, but only for an associated extension
     *  \tparam Extension to set the option on
     *  \tparam Option type which must conform the asio SettableSocketOption concept
     *  \param ec error_code to capture error
     *  \param opt T option to set
     */
    template<typename Extension, typename Option>
    friend boost::system::error_code set_socket_option(socket & s, Option const& opt,
                                                       boost::system::error_code & ec) {
        return s.get_service().set_option<Extension>(s.implementation, opt, ec);
    }

    /** \brief as for get_option bu only for an associated extension
     *  \tparam Extension to get option from
     *  \tparam T must conform to the asio GettableSocketOption concept
     *  \param opt T option to get
     *  \param ec error_code to capture error
     */
    template<typename Extension, typename Option>
    boost::system::error_code get_socket_option(socket & s, Option & opt,
                                                boost::system::error_code & ec) {
        return s.get_service().get_option<Extension>(s.implementation, opt, ec);
    }
};

/** \brief as for set_option, but only for an associated extension
*  \tparam Extension to set option on
*  \tparam T type which must conform the asio SettableSocketOption concept
*  \param opt T option to set
*  \throw boost::system::system_error
*/
template<typename Extension, typename Option>
void set_socket_option(socket & s, Option const& opt) {
    boost::system::error_code ec;
    if (set_socket_option<Extension>(s, opt, ec))
        throw boost::system::system_error(ec);
}

/** \brief as for get_option but only for an associated extension
*  \tparam Extension to get option from
*  \tparam T must conform to the asio GettableSocketOption concept
*  \param opt T option to get
*  \throw boost::system::system_error
*/
template<typename Extension, typename Option>
void get_socket_option(socket & s, Option & opt) {
    boost::system::error_code ec;
    if (get_socket_option<Extension>(s, opt, ec))
        throw boost::system::system_error(ec);
}

} // namespace aziomq
#endif // AZIOMQ_IO_SOCKET_HPP_

