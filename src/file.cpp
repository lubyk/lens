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
#include "lens/File.h"

#include <errno.h>
using namespace lens;

// Read 'sz' bytes from fd. Returns string or false and error message.
// local sz = 1024
// local op, data = f:read(sz)
// while File.op == File.EAGAIN
//   local l
//   yield('read', f:fd())
//   op, l = f:read(sz - string.len(data))
//   data = data .. l
// end
// if op == File.OK then
//   return line
// else
//   return false, File.Message[op]
// end
LuaStackSize File::read(size_t sz, lua_State *L) {
  // This should not happen. Error.
  if (fd_ == 0) throw dub::Exception("Cannot read from a closed file.");
  if (!(mode_ & Read)) throw dub::Exception("File mode not compatible with read operation.");
  // TODO
  return 0;
}

/*
int lk::Socket::recvBytes(lua_State *L, int sz) {
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

  while (sz) {
    // read more data
    // we prefer not reading too much so that we might simplify read operation
    // with a single pushlstring
    buffer_length_ = ::recv(socket_fd_, buffer_, sz < MAX_BUFF_SIZE ? sz : MAX_BUFF_SIZE, 0);
    if (buffer_length_ == 0) {
      // connection closed
      // abort
      return 0;
    } else if (buffer_length_ < 0) {
      buffer_length_ = 0;
      throw dub::Exception("Could not receive (%s).", strerror(errno));
    }
    
    luaL_addlstring(&buffer, buffer_, buffer_length_);

    sz -= buffer_length_;
    buffer_i_ = buffer_length_;
  }                             
  
  luaL_pushresult(&buffer);
  return 1;
}
*/

// Read a line from fd. Returns string or nil on EOF
// function readLine()
//    local op, line = f:readLine()
//    while op == File.EAGAIN
//      local l
//      yield('read', f:fd())
//      op, l = f:readLine()
//      line = line .. l
//    end
//    if op == File.OK then
//      return line
//    else
//      -- eof
//      return nil
//    end
// end
// Return op code and string.
LuaStackSize File::readLine(lua_State *L) {
  if (fd_ == 0) throw dub::Exception("Cannot read from a closed file.");
  if (!(mode_ & Read)) throw dub::Exception("File mode not compatible with read operation.");

  bool has_data = buffer_i_ < buffer_length_;
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
        lua_pushnumber(L, (int)File::OK);
        return 2;
      } else if (c != '\r') {
        // ignore \r
        // add char to lua buffer
        luaL_putchar(&buffer, c);
      }
    }
      
    // Need to fill buffer
    buffer_i_      = 0;
    buffer_length_ = 0;

    // read more data
    buffer_length_ = ::read(fd_, buffer_, MAX_BUFF_SIZE);
    if (buffer_length_ == 0) {
      if (has_data) {
        // EOF == same as end of line
        luaL_pushresult(&buffer);
        lua_pushnumber(L, (int)File::OK);
        return 2;
      } else {
        // Reading past end of file
        // empty string
        luaL_pushresult(&buffer);
        lua_pushnumber(L, (int)File::End);
        return 2;
      }
    } else if (buffer_length_ < 0) {
      buffer_length_ = 0;
      int err = errno;
      switch(err) {
        case EINTR: // on interruption, just redo
        case EAGAIN:
          luaL_pushresult(&buffer);
          lua_pushnumber(L, (int)File::Wait);
          return 2;
        default:
          throw dub::Exception("Could not read (%s).", strerror(err));
      }
    } else {
      // All good, we have some more data to process
      has_data = true;
    }
  }                             
  return 0;
}

// Read everyting until EOF is reached.
LuaStackSize File::readAll(lua_State *L) {
  // TODO
  return 0;
}

/*
LuaStackSize lk::Socket::recvAll(lua_State *L) {
  if (socket_type_ == lk::Socket::UDP) {

    struct sockaddr_in fromAddr;
    socklen_t fromAddrLen = sizeof(fromAddr);

    // TODO: store latest fromAddr.
    buffer_length_ = recvfrom(socket_fd_, buffer_, MAX_BUFF_SIZE, 0,
		 (struct sockaddr *) &fromAddr, &fromAddrLen);
    if (buffer_length_ < 0) return 0;

    // Single packet: do not wait for disconnection.
    lua_pushlstring(L, buffer_, buffer_length_);
    return 1;
  } else {
    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);

    if (buffer_i_ < buffer_length_) {
      luaL_addlstring(&buffer, buffer_ + buffer_i_, buffer_length_ - buffer_i_);
    }

    while (true) {
      // read more data
      buffer_length_ = ::recv(socket_fd_, buffer_, MAX_BUFF_SIZE, 0);
      if (buffer_length_ == 0) {
        // connection closed
        // done
        return 0;
      } else if (buffer_length_ < 0) {
        buffer_length_ = 0;
        if (errno == EAGAIN) {
          break;
        } else {
          throw dub::Exception("Could not receive (%s).", strerror(errno));
        }
      }
      luaL_addlstring(&buffer, buffer_, buffer_length_);
    }                             
    
    luaL_pushresult(&buffer);
    return 1;
  }
}
*/


// Return op and written size
LuaStackSize File::write(lua_State *L) {
  if (fd_ == 0) throw dub::Exception("Cannot write to a closed file.");
  if (!(mode_ & Write)) throw dub::Exception("File mode not compatible with write operation.");

  size_t sz;
  const char *str = dub::checklstring(L, 2, &sz);

  ssize_t todo = sz;

  for (ssize_t n; todo; ) {
    // keep trying to write until we either get EAGAIN, an error or finish
    n = ::write(fd_, str, todo);
    if (n == -1 && errno != EINTR) {
      // error
      break;
    }
    str += n;
    todo -= n;
  }

  if (todo != 0) {
    switch(errno) {
      case EAGAIN:
        // Wait to write more.
        lua_pushnumber(L, sz - todo);
        lua_pushnumber(L, (int)File::Wait);
        return 2;
      default:
        throw dub::Exception("Could not write (%s).", strerror(errno));
    }
  } else {
    // OK
    lua_pushnumber(L, sz);
    lua_pushnumber(L, (int)File::OK);
    return 2;
  }
}
