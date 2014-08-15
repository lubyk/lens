/*
  ==============================================================================

   This file is part of the LUBYK project (http://lubyk.org)
   Copyright (c) 2007-2014 by Gaspard Bucher (http://teti.ch).

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
#ifndef LUBYK_INCLUDE_LENS_FILE_H_
#define LUBYK_INCLUDE_LENS_FILE_H_

#include "lens/lens.h"

#include "dub/dub.h"

#include <sys/fcntl.h> // O_READ
#include <errno.h>     // errno

// 8 Ko
// Maybe we could pass an argument to make this size smaller/bigger
#define MAX_BUFF_SIZE 8196

namespace lens {

/** OS popen wrapper.
 *
 * @dub string_format: %%i
 *      string_args: self->fd()
 */
class File {
  // File descriptor for reading or writing.
  int fd_;

  // File mode OR from File::Mode
  int mode_;

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
  
public:
  enum Mode {
    None   = -1,
    Read   = O_RDONLY,
    Write  = O_WRONLY,
    Append = O_APPEND,
#ifdef O_EVTONLY
    Events = O_EVTONLY,
#else
    Events = 32768,
#endif
  };

  
  enum Events {
    DeleteEvent  = 0x00000001,    /* vnode was removed */
    WriteEvent   = 0x00000002,    /* data contents changed */
    ExtendEvent  = 0x00000004,    /* size increased */
    AttribEvent  = 0x00000008,    /* attributes changed */
    LinkEvent    = 0x00000010,    /* link count changed */
    RenameEvent  = 0x00000020,    /* vnode was renamed */
    RevokeEvent  = 0x00000040,    /* vnode access was revoked */
    NoneEvent    = 0x00000080,    /* No specific vnode event: to test for EVFILT_READ activation*/
  };

  enum IOCode {
    OK = 0,
    Wait,
    End,
  };

  File(const char *path, Mode mode)
    : fd_(0)
    , mode_(mode)
    , buffer_length_(0)
    , buffer_i_(0)
  {
    if (mode != None) {
      fd_ = open(path, (int)mode);
      if (fd_ < 0) {
        throw dub::Exception("Could not open file '%s' (%s).", path, strerror(errno));
      }
    }
  }

  virtual ~File() {
    close();
  }

  int fd() {
    return fd_;
  }

  void close() {
    if (fd_) {
      ::close(fd_);
      fd_ = 0;
    }
  }


  // Return string and op
  LuaStackSize read(size_t sz, lua_State *L);

  // Return string and op
  LuaStackSize readLine(lua_State *L);

  // Read everyting until EOF is reached.
  LuaStackSize readAll(lua_State *L);

  // Return written size and op
  LuaStackSize write(lua_State *L);

protected:
  void setFd(int fd) {
    fd_ = fd;
  }

  void setMode(Mode m) {
    mode_ = m;
  }
};
} // lens

#endif // LUBYK_INCLUDE_LENS_FILE_H_

