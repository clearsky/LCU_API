/*
 * Copyright (c) 2017 Darren Smith
 *
 * wampcc is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef WAMPCC_PROTOCOL_H
#define WAMPCC_PROTOCOL_H

#include "wampcc/types.h"

#include <vector>
#include <cstddef>
#include <memory>
#include <stdexcept>

namespace wampcc {

class kernel;
struct logger;
class tcp_socket;

constexpr inline int operator & (int lhs, serialiser_type rhs) {
  return lhs & static_cast<int>(rhs);
}
constexpr inline int operator | (serialiser_type lhs, serialiser_type rhs) {
  return (int) (static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr inline int operator | (protocol_type lhs, protocol_type rhs) {
  return (int) (static_cast<int>(lhs) | static_cast<int>(rhs));
}

class buffer
{
public:

  struct read_pointer
  {
    read_pointer(char *, size_t);

    char  operator[](size_t i) const { return m_ptr[i]; }
    char& operator[](size_t i)       { return m_ptr[i]; }

    void advance(size_t i) { m_ptr += i; m_avail -= i; }

    size_t avail() const { return m_avail; }

    const char * ptr() const { return m_ptr; }
          char * ptr()       { return m_ptr; }

  private:
    char * m_ptr;
    size_t m_avail;
  };


  buffer(size_t initial_size, size_t max_size);

  /** amount of actual data present */
  size_t data_size() const { return m_bytes_avail; }

  /** current space for new data */
  size_t space() const { return m_mem.size() - m_bytes_avail; }

  /** current buffer capacity */
  size_t capacity() const { return m_mem.size(); }

  /** pointer to data */
  char* data() { return m_mem.data(); }

  /** copy in new bytes, growing internal space if necessary */
  size_t consume(const char* src, size_t len);

  /** obtain a read pointer */
  read_pointer read_ptr() { return {m_mem.data(), m_bytes_avail}; }

  /** Update buffer to remove bytes that have be read via the read pointer. */
  void discard_read(read_pointer);

  void update_max_size(size_t);

private:

  void grow_by(size_t len);

  std::vector<char> m_mem;
  size_t m_max_size;
  size_t m_bytes_avail;
};


/** Exception thrown during IO processing of a new connection. This is an
 * unrecoverable error that prevents the correct construction of a protocol
 * encoder/decoder object, and so prevents creationg of a WAMP session from a
 * new socket connection.  Will lead to connection drop without any attempt to
 * send a final WAMP message (although protocol level messages maybe sent before
 * disconnect). */
class handshake_error : public std::runtime_error
{
public:
  handshake_error(const std::string& msg)
    : std::runtime_error(msg.c_str())
  {}
};


/** Exception thrown due to any sort of malformed WAMP message.  Generally this
 * error is thrown when the peer has failed to respect the terms of the WAMP
 * message-level protocol. Some examples: missinng mandatory arguments in WAMP
 * messages; values that have incorrect primitive type or container type (eg an
 * object where an array was expected).  This exception can be thrown during
 * initial message processing on the IO thread, or later during deferred
 * processing on the EV thread.  Shall result in a connection drop. */
class protocol_error : public std::runtime_error
{
public:
  protocol_error(const std::string& msg)
    : std::runtime_error(msg.c_str())
  {}

};


class codec
{
public:
  virtual ~codec() {}
  virtual json_value decode(const char* ptr, size_t msglen) = 0;
  virtual std::vector<char> encode(const json_array&) = 0;
  virtual serialiser_type type() const = 0;
  virtual const char* name() const = 0;
};

namespace protocol_constants {
  /* Keep default interval under 1 minute, which is a typical timeout period
     chosen by load balancers etc. */
  static const int default_ping_interval_ms = 30000;

  static const int default_pong_min_interval_ms = 1000;

  static const int default_max_missed_pings = 2;
}

/* Base class for encoding & decoding of bytes on the wire. */
class protocol
{
public:

  struct options
  {
    int serialisers; /* mask of enum serialiser_type bits */

    std::chrono::milliseconds ping_interval; /* 0 for no heartbeats */

    /* minimum allowed interval between replies to ping */
    std::chrono::milliseconds pong_min_interval;

    /* Maximum number of missed pings.  A connection that reaches this mumber of
     * missed pings will be dropped.  A missed ping is one that is not answered
     * by a pong, nor by any other received data. It is expected that a peer
     * will reply to a ping with a pong message, or, will send application data
     * or other control frame instead of a pong. */
    int max_missed_pings;

    options()
      : serialisers(wampcc::all_serialisers),
        ping_interval(protocol_constants::default_ping_interval_ms),
        pong_min_interval(protocol_constants::default_pong_min_interval_ms),
        max_missed_pings(protocol_constants::default_max_missed_pings)
    {
      if (ping_interval.count() == 0 && max_missed_pings != 0)
        throw std::runtime_error("cannot have non-zero max_missed_pings with zero ping_interval");
    }
  };

  struct protocol_callbacks
  {
    std::function<void(std::unique_ptr<protocol>&)> upgrade_protocol;
    std::function<void(std::chrono::milliseconds)>  request_timer;
    std::function<void(std::chrono::milliseconds)> protocol_closed;
  };

  typedef std::function<void(json_array msg,  json_uint_t msgtype)> t_msg_cb;
  typedef std::function<void()> t_initiate_cb;

  protocol(kernel*, tcp_socket*, t_msg_cb, protocol_callbacks, connect_mode m,
           size_t buf_initial_size=1, size_t buf_max_size=1024);

  virtual ~protocol() = default;

  /* Initiate the protocol closure handshake.  Returns false if the protocol
   * state doesn't support closure handshake or if already closed, so allowing
   * the caller to proceed with wamp session closure.  Otherwise returns true,
   * to indicate that closure handshake will commence and will be completed
   * later.  */
  virtual bool initiate_close() = 0;

  virtual void on_timer() {}
  virtual void io_on_read(char*, size_t) = 0;
  virtual void initiate(t_initiate_cb) = 0;
  virtual const char* name() const = 0;

  virtual void send_msg(const json_array& j) = 0;

  connect_mode mode() const { return m_mode; }

protected:

  bool have_codec() const { return (bool) m_codec; }
  void create_codec(int choices);

  std::string fd() const;

  void decode(const char* ptr, size_t msglen);
  std::vector<char> encode(const json_array&);

  kernel* m_kernel;
  logger& __logger;
  tcp_socket * m_socket; /* non owning */
  t_msg_cb    m_msg_processor;
  protocol_callbacks m_callbacks;
  buffer m_buf;
  std::shared_ptr<codec> m_codec;

private:
  connect_mode m_mode;
};

typedef std::function< std::unique_ptr<protocol> (tcp_socket*, protocol::t_msg_cb,
                                                  protocol::protocol_callbacks) > protocol_builder_fn;


class selector_protocol : public protocol
{
public:
  static constexpr const char* NAME = "selector";

  struct options : public protocol::options
  {
    int protocols;
    options() :
      protocol::options(),
      protocols(protocol_type::websocket|protocol_type::rawsocket){}
  };

  selector_protocol(kernel*, tcp_socket*, t_msg_cb,
                    protocol::protocol_callbacks, options = {});

  void io_on_read(char*, size_t) override;

  void initiate(t_initiate_cb) override
  {
    throw std::runtime_error("selector_protocol cannot initiate");
  }

  const char* name() const override { return NAME; }

  void send_msg(const json_array&) override
  {
    throw std::runtime_error("selector_protocol cannot send");
  }
  bool initiate_close() override { return false; }

  static size_t buffer_size_required();

private:
  options m_opts;
};

}


#endif
