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
#include "lens/Popen.h"

#include "dub/dub.h"

#include <stdio.h>
#include <unistd.h> // close
#include <fcntl.h>  // fcntl
#include <errno.h>  // errno

#define MAX_BUFF_SIZE 8196

using namespace lens;

Popen::Popen(const char *program, int mode)
  : File(NULL, File::None)
  , pid_(0)
{

  if (mode != File::Read && mode != File::Write) {
      throw dub::Exception("Invalid mode %i (should be File.Read or File.Write).", mode);
  }

  setMode((File::Mode) mode);

  // Pipe file descriptors
  int pipes[2];
  // Prepare pipe
  if (pipe(pipes)) {
    throw dub::Exception("Could not create pipe (%s).", strerror(errno));
  }

  int child_fd, parent_fd;

  if (mode == Read) {
    parent_fd = pipes[0]; // read
    child_fd  = pipes[1]; // write
  } else {
    parent_fd = pipes[1]; // write
    child_fd  = pipes[0]; // read
  }
  
  int op;

  switch(pid_ = vfork()) {
    case -1: // error
      ::close(parent_fd);
      ::close(child_fd);
      throw dub::Exception("Could not fork (%s).", strerror(errno));
      break; // not reached
    case 0:  // child
      // With vfork, the parent thread is waiting until we call _exit or exec.
      // Memory is shared with parent.
      ::close(parent_fd);

      op = mode == Read ? STDOUT_FILENO : STDIN_FILENO;

      if (dup2(child_fd, op) == -1) {
        fprintf(stderr, "Error in child fd setup (%s).\n", strerror(errno));
        // Child from vfork can only call _exit to avoid (atexit calls) !
        ::close(child_fd);
        _exit(-1);
      }

      // We must close all unused file descriptors in child.
      ::close(child_fd);

      // Execute command and start parent thread
      execl("/bin/sh", "sh", "-c", program, NULL);
#ifdef __CYGWIN__
      // execl failed. On Cygwin, try to find 'sh' in PATH.
      execlp("sh", "sh", "-c", program, NULL);
#endif
      break; // not reached
    default: // parent
      // Child is running, we can start reading or writing to/from pipe.
      ::close(child_fd);

      // We only work with non-blocking fd.
      int flags = fcntl(parent_fd, F_GETFL, 0);
      fcntl(parent_fd, F_SETFL, flags | O_NONBLOCK);
      
      // All set.
      setFd(parent_fd);
  }
}

int Popen::waitpid() {
  close();
  int child_status;
  int ret = ::waitpid(pid_, &child_status, 0);
  if (ret == -1 || ret == pid_ - 1) {
    throw dub::Exception("Could not waitpid (%s).", strerror(errno));
  }
  if (WIFEXITED(child_status)) {
    return WEXITSTATUS(child_status);
  } else {
    // error
    return -1;
  }
}

