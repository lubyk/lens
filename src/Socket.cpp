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
#include "lens/Socket.h"

/** Bind socket to a specific interface.
 * @return bound port
 */
int lens::Socket::bind(const char *localhost, int port) {
  char port_str[10];
  snprintf(port_str, 10, "%i", port);

  if (localhost) {
    local_host_ = localhost;
    if (local_host_ == "" || local_host_ == "*") {
      // bind to any interface
      localhost = NULL;
      local_host_ = "*";
    }
  }

  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));

  // we do not care if we get an IPv4 or IPv6 address
  hints.ai_family = AF_UNSPEC;
  // TCP
  // TODO: Use socket_type_ info
  if (socket_type_ == TCP) {
    hints.ai_socktype = SOCK_STREAM;
  } else {
    hints.ai_socktype = SOCK_DGRAM;
  }
  // fill our own IP
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, port_str, &hints, &res)) {
    throw dub::Exception("Could not getaddrinfo for %s:%i.", local_host_.c_str(), port);
  }

  if (socket_fd_ != -1) {
    ::close(socket_fd_);
    socket_fd_ = -1;
  }

  // we use getaddrinfo to stay IPv4/IPv6 agnostic
  socket_fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  // printf("socket --> %i\n", socket_fd_);
  if (socket_fd_ == -1) {
    freeaddrinfo(res);
    throw dub::Exception("Could not create socket for %s:%i (%s).", local_host_.c_str(), port, strerror(errno));
  }
  setNonBlocking();

  // bind to port
  if (::bind(socket_fd_, res->ai_addr, res->ai_addrlen)) {
    freeaddrinfo(res);
    throw dub::Exception("Could not bind socket to %s:%i (%s).", local_host_.c_str(), port, strerror(errno));
  }
  freeaddrinfo(res);

  if (port == 0) {
    local_port_ = get_port(socket_fd_);
  } else {
    local_port_ = port;
  }

  return local_port_;
}

bool lens::Socket::connect(const char *host, int port) {
  if (socket_fd_ != -1) {
    ::close(socket_fd_);
    socket_fd_ = -1;
  }

  remote_host_ = host;
  remote_port_ = port;

  if (socket_type_ == UDP) {
    // store remote info but only connect on send
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);

    // bind any port
    struct sockaddr_in addr;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);

    if (::bind(socket_fd_, (struct sockaddr *) &addr, sizeof(addr))) {
      throw dub::Exception("Could not bind local socket (%s)\n.", strerror(errno));
    }
    return true;
  }

  char port_str[10];
  snprintf(port_str, 10, "%i", port);
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));

  // we do not care if we get an IPv4 or IPv6 address
  hints.ai_family = AF_UNSPEC;
  // TCP
  hints.ai_socktype = SOCK_STREAM;

  int status;
  if ( (status = getaddrinfo(host, port_str, &hints, &res)) ) {
    throw dub::Exception("Could not getaddrinfo for %s:%i (%s).", host, port, gai_strerror(status));
  }


  // we use getaddrinfo to stay IPv4/IPv6 agnostic
  socket_fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  // printf("socket --> %i\n", socket_fd_);
  if (socket_fd_ == -1) {
    freeaddrinfo(res);
    throw dub::Exception("Could not create socket for %s:%i (%s).", host, port, strerror(errno));
  }
  setNonBlocking();

  // connect
  if (::connect(socket_fd_, res->ai_addr, res->ai_addrlen)) {
    freeaddrinfo(res);
    if (errno == EINPROGRESS) {
      return false; // wait for 'write' and try again later
    }
    throw dub::Exception("Could not connect socket to %s:%i (%s).", host, port, strerror(errno));
  }

  char info[INET6_ADDRSTRLEN];
  if (res->ai_family == AF_INET) {
    inet_ntop(res->ai_family, &(((struct sockaddr_in*)res)->sin_addr), info, sizeof(info));
  } else {
    inet_ntop(res->ai_family, &(((struct sockaddr_in6*)res)->sin6_addr), info, sizeof(info));
  }
  // printf("Connected to %s:%i (%s)\n", host, port, info);

  freeaddrinfo(res);

  remote_host_ = host;
  remote_port_ = port;
  return true;
}

void lens::Socket::connectFinish() {
  if (socket_fd_ == -1) {
    throw dub::Exception("Should only be called after 'connect' (no socket).");
  }
  // After select(2) indicates writability, use getsockopt(2) to read the 
  // SO_ERROR option at level SOL_SOCKET to determine whether connect() 
  // completed successfully (SO_ERROR is zero) or unsuccessfully 
  // (SO_ERROR is one of the usual error codes listed here, 
  // explaining the reason for the failure).

  int err;
  socklen_t len = sizeof(err);
  if (getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &err, &len)) {
    throw dub::Exception("Could not finalize connection (%s).", strerror(errno));
  }

  if (err) {
    throw dub::Exception("Could not finalize connection (%s).", strerror(err));
  }
}

/** Start listening for incoming connections.
 */
void lens::Socket::listen(int backlog) {
  if (socket_type_ == lens::Socket::UDP) {
    throw dub::Exception("not supported by UDP sockets.");
  }
  
  if (local_port_ == -1)
    throw dub::Exception("Listen called before bind.");

  if (::listen(socket_fd_, backlog)) {
    throw dub::Exception("Could not listen (%s).", strerror(errno));
  }
}

/** Accept a new incomming connection.
 * @return a new lens.Socket connected to the remote end.
 */
LuaStackSize lens::Socket::accept(lua_State *L) {
  if (socket_type_ == lens::Socket::UDP) {
    throw dub::Exception("not supported by UDP sockets.");
  }
  // <self>
  lua_pop(L, 1);

  if (local_port_ == -1)
    throw dub::Exception("Accept called before bind.");

  struct sockaddr sa;
  memset(&sa, 0, sizeof(struct sockaddr));
  // length has to be in a variable
  socklen_t sa_len = sizeof(sa);

  int fd;

  fd = ::accept(socket_fd_, &sa, &sa_len);
  // printf("[%p] accept(%i) --> %i\n", this, socket_fd_, fd);
  
  if (fd == -1) {
    if (errno == EAGAIN) {
      // return nil to indicate EAGAIN
      return 0;
    }
    throw dub::Exception("Error while accepting connection (%s).", strerror(errno));
  }

  // get remote name / port
  int remote_port;
  if (sa.sa_family == AF_INET) {
    remote_port = ntohs(((struct sockaddr_in *)&sa)->sin_port);
  } else {
    remote_port = ntohs(((struct sockaddr_in6 *)&sa)->sin6_port);
  }

  char remote_host[NI_MAXHOST];

  if (getnameinfo(&sa, sizeof(sa), remote_host, sizeof(remote_host), NULL, 0, 0)) {
    throw dub::Exception("Could not get remote host name (%s).", strerror(errno));
  }

  return pushNewSocket(L, socket_type_, fd, local_host_.c_str(), remote_host, remote_port);
}

/** Send raw bytes without encoding with msgpack.
 * param: string to send.
 */
int lens::Socket::send(lua_State *L) {
  // <string>
  size_t size;
  const char *data = luaL_checklstring(L, -1, &size);
  // printf("send string (%lu bytes)\n", size);
  // send raw bytes
  return sendBytes(data, size);
}

int lens::Socket::get_port(int fd) {
  // get bound port
  struct sockaddr sa;
  memset(&sa, 0, sizeof(struct sockaddr));
  // length has to be in a variable
  socklen_t sa_len = sizeof(sa);
  if (getsockname(fd, &sa, &sa_len)) {
    throw dub::Exception("Could not get bound port (%s).", strerror(errno));
  }
  if (sa.sa_family == AF_INET) {
    return ntohs(((struct sockaddr_in *)&sa)->sin_port);
  } else {
    return ntohs(((struct sockaddr_in6 *)&sa)->sin6_port);
  }
}

int lens::Socket::recvLine(lua_State *L) {
  luaL_Buffer buffer;
  luaL_buffinit(L, &buffer);

  while (true) {
    while (buffer_i_ < buffer_length_) {
      char c = buffer_[buffer_i_++];
      // printf("recvLine (%c)\n", c);
      if (c == '\n') {
        // found end of line
        // push string
        luaL_pushresult(&buffer);
        return 1;
      } else if (c != '\r') {
        // ignore \r
        // add char to lua buffer
        luaL_putchar(&buffer, c);
      }
    }
      
    // read more data
    buffer_length_ = ::recv(socket_fd_, buffer_, MAX_BUFF_SIZE, 0);
    if (buffer_length_ == 0) {
      // connection closed
      return 0;
    } else if (buffer_length_ < 0) {
      buffer_length_ = 0;
      if (errno == EAGAIN) {
        luaL_pushresult(&buffer);
        // indicate more to come
        lua_pushboolean(L, true);
        return 2;
      } else {
        throw dub::Exception("Could not receive (%s).", strerror(errno));
      }
    }
    buffer_i_ = 0;
  }                             
  return 0;
}

int lens::Socket::recvBytes(int sz, lua_State *L) {
  luaL_Buffer buffer;

  if (buffer_i_ < buffer_length_) {
    if (sz <= (buffer_length_-buffer_i_)) {
      // everything is in the buffer already
      lua_pushlstring(L, buffer_ + buffer_i_, sz);
      buffer_i_ += sz;
      return 1;
    }
    luaL_buffinit(L, &buffer);
    // add current content
    luaL_addlstring(&buffer, buffer_ + buffer_i_, buffer_length_ - buffer_i_);
    sz -= buffer_length_ - buffer_i_;
  } else {
    luaL_buffinit(L, &buffer);
  }

  while (sz > 0) {
    // continue receiving data until we have what we need, or we encounter EAGAIN.

    // we prefer not reading too much so that we might simplify read operation
    // with a single pushlstring
    buffer_length_ = ::recv(socket_fd_, buffer_, sz < MAX_BUFF_SIZE ? sz : MAX_BUFF_SIZE, 0);
    if (buffer_length_ == 0) {
      // connection closed
      return 0;
    } else if (buffer_length_ < 0) {
      buffer_length_ = 0;
      if (errno == EAGAIN) {
        luaL_pushresult(&buffer);
        // indicate more to come
        lua_pushnumber(L, sz);
        return 2;
      } else {
        throw dub::Exception("Could not receive (%s).", strerror(errno));
      }
    } else {
      // found buffer_length_ bytes
      luaL_addlstring(&buffer, buffer_, buffer_length_);
      sz -= buffer_length_;

      // mark buffer as empty
      buffer_i_ = buffer_length_;
    }
  }

  // found all data
  luaL_pushresult(&buffer);
  return 1;
}

LuaStackSize lens::Socket::recvMessage(lua_State *L) {
  if (socket_type_ != lens::Socket::UDP) {
    throw dub::Exception("recvMessage only works with UDP sockets.");
  }

  struct sockaddr_in fromAddr;
  socklen_t fromAddrLen = sizeof(fromAddr);

  // TODO: store latest fromAddr.
  buffer_length_ = recvfrom(socket_fd_,
                            buffer_, MAX_BUFF_SIZE, 0,
                            (struct sockaddr *) &fromAddr, &fromAddrLen);

  if (buffer_length_ == 0) {
    // connection closed
    return 0;
  } else if (buffer_length_ < 0) {
    buffer_length_ = 0;
    if (errno == EAGAIN) {
      lua_pushnil(L);
      lua_pushboolean(L, true);
      // <nil>, eagain
      return 2;
    } else {
      throw dub::Exception("Could not receive (%s).", strerror(errno));
    }
  } else {
    // found buffer_length_ bytes
    lua_pushlstring(L, buffer_, buffer_length_);

    // mark buffer as empty
    buffer_i_ = buffer_length_;
    return 1;
  }
}

