/*
  ==============================================================================

   This file is part of the LUBYK project (http://lubyk.org)
   Copyright (c) 2007-2011 by Gaspard Bucher (http://teti.ch).

  ------------------------------------------------------------------------------

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

  ==============================================================================
*/
#ifndef LUBYK_INCLUDE_LENS_SOCKET_H_
#define LUBYK_INCLUDE_LENS_SOCKET_H_

#include "dub/dub.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // close
#include <netdb.h>  // getaddrinfo
#include <arpa/inet.h> // inet_ntop

#include <errno.h>  // errno
#include <string.h> // strerror
#include <fcntl.h>

#include <string>

// How many pending connections should wait for 'accept'.
#define BACKLOG 10
// recv buffer size (must be at least SIZEOF_SIZE)
#define MAX_BUFF_SIZE 8196
// 4 = size needed to encode SIZE_MAX
#define SIZEOF_SIZE 4

namespace lens {

/** Listen for incoming messages on a given port.
 *
 * @dub push: dub_pushobject
 *      string_format:'%%s:%%d --> %%s:%%d'
 *      string_args:'self->localHost(), self->localPort(), self->remoteHost(), self->remotePort()'
 */
class Socket : public dub::Thread {
  int socket_fd_;
  int socket_type_;
  std::string local_host_;
  int local_port_;
  std::string remote_host_;
  int remote_port_;

  // buffer management
  /** Number of bytes already received in the buffer.
   */
  int buffer_length_;

  /** Bytes already used in the buffer.
   * If buffer_i_ == buffer_length_: there is no more data in the buffer.
   */
  int buffer_i_;

  /** Buffer that contains received data not yet used by Lua.
   */
  char buffer_[MAX_BUFF_SIZE];

  /** Non-blocking flag when setNonBlocking is called before creating the
   * socket.
   */
  bool non_blocking_;
public:

  enum SocketType {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
  };

  Socket(int socket_type)
      // When changing this, also change the private
      // constructor.
      : socket_fd_(-1)
      , socket_type_(socket_type)
      , local_host_("*")
      , local_port_(-1)
      , remote_host_("?")
      , remote_port_(-1)
      , buffer_length_(0)
      , buffer_i_(0)
      , non_blocking_(false) {
  }

  virtual ~Socket() {
    close();
  }

  void close() {
    if (socket_fd_ != -1) {
      ::close(socket_fd_);
      socket_fd_ = -1;
    }
  }

  /** Bind socket to a specific interface.
   * @return bound port
   */
  int bind(const char *localhost = NULL, int port = 0);

  /** Connect to a remote socket.
   * @return false if the socket is not ready and we should waitWrite and 'connectFinish'.
   */
  bool connect(const char *host, int port);

  /** Finish connecting for NON-BLOCKING sockets.
   */
  void connectFinish();

  /** Start listening for incoming connections.
   * @param backlog number of accepted connections in queue before refusing.
   */
  void listen(int backlog = BACKLOG);

  /** Accept a new incomming connection.
   * @return a new lens.Socket connected to the remote end.
   */
  LuaStackSize accept(lua_State *L);

  void setRecvTimeout(int timeout) {
    setTimeout(timeout, SO_RCVTIMEO);
  }
    
  void setSendTimeout(int timeout) {
    setTimeout(timeout, SO_SNDTIMEO);
  }

  void setNonBlocking() {
    non_blocking_ = true;
    if (socket_fd_ != -1) {
      int x;
      x = fcntl(socket_fd_, F_GETFL, 0);
      if (-1 == fcntl(socket_fd_, F_SETFL, x | O_NONBLOCK)) {
        throw dub::Exception("Could not set non-blocking (%s).", strerror(errno));
      }
    }
  }
  
  /** Receive a raw string.
   * This IO call blocks.
   */
  LuaStackSize recv(lua_State *L);

  /** Send raw bytes.
   * param: string to send.
   * @return number of bytes sent.
   */
  int send(lua_State *L);

  /** Return the hostname of the local host.
   */
  const char *localHost() const {
    return local_host_.c_str();
  }

  /** Return the port number of the local end.
   */
  int localPort() const {
    return local_port_;
  }

  /** Return the hostname of the remote host.
   */
  const char *remoteHost() const {
    return remote_host_.c_str();
  }

  /** Return the port number of the remote end.
   */
  int remotePort() const {
    return remote_port_;
  }

  /** Same as localPort.
   */
  int port() const {
    return local_port_;
  }

  /** File descriptor used by scheduler.
   */
  int fd() const {
    return socket_fd_;
  }

  /** This is the same as recv("*a").
   */
  LuaStackSize recvAll(lua_State *L);

protected:

  /** Send raw bytes from C++.
   */
  inline int sendBytes(const char *bytes, size_t sz) {
    if (socket_type_ == UDP) {
      struct addrinfo hints, *res;

      memset(&hints, 0, sizeof(hints));

      // we do not care if we get an IPv4 or IPv6 address
      hints.ai_family = AF_UNSPEC;
      // UDP
      hints.ai_socktype = SOCK_DGRAM;

      // TODO: performance save port string.
      char port_str[10];
      snprintf(port_str, 10, "%i", remote_port_);

      int status;
      if ( (status = getaddrinfo(remote_host_.c_str(), port_str, &hints, &res)) ) {
        throw dub::Exception("Could not getaddrinfo for %s:%i (%s).", remote_host_.c_str(), remote_port_, gai_strerror(status));
      }


      int sent = sendto(socket_fd_, bytes, sz, 0, res->ai_addr, res->ai_addrlen);
      freeaddrinfo(res);
      return sent;
    } else {
      int sent = ::send(socket_fd_, bytes, sz, 0);
      if (sent == -1) {
        if (errno == EAGAIN) {
          sent = 0;
        } else {
          throw dub::Exception("Could not send message (%s).", strerror(errno));
        };
      }
      return sent;
    }
  }


  /** Create a socket with an existing file descriptor.
   * This is used as the result of an 'accept()' call.
   */
  Socket(int type, int fd, const char *local_host, const char *remote_host, int remote_port)
      : socket_fd_(fd)
      , socket_type_(type)
      , local_host_(local_host)
      , local_port_(get_port(fd))
      , remote_host_(remote_host)
      , remote_port_(remote_port)
      , buffer_length_(0)
      , buffer_i_(0)
      , non_blocking_(false) {
      }

private:
  static int get_port(int fd);

  int recvLine(lua_State *L);

  int recvBytes(lua_State *L, int sz);


  void setTimeout(int timeout, int opt_name);

  virtual int pushNewSocket(lua_State *L, int type, int fd, const char *local_host, const char *remote_host, int remote_port) {
    Socket *new_socket = new Socket(type, fd, local_host, remote_host, remote_port);

    new_socket->dub_pushobject(L, new_socket, "lens.Socket", true);
    return 1;
  }
};
} // lens

#endif // LUBYK_INCLUDE_LENS_SOCKET_H_

