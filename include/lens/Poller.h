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
#ifndef LUBYK_INCLUDE_LENS_POLLER_H_
#define LUBYK_INCLUDE_LENS_POLLER_H_

#include "lens/lens.h"

#include "dub/dub.h"

#include <poll.h>   // poll()
#include <errno.h>  // errno
#include <string.h> // strerror()

#include <stdlib.h> // rand()
#include <time.h>   // time()
#include <assert.h> // assert()
#include <signal.h> // signal(), SIG_DFL, ...

// #include "msgpack/msgpack.h"

namespace lens {

/** lens basic Poller.
 *
 * @dub string_format: %%f
 *      string_args: self->count()
 */
class Poller {
  typedef struct pollfd Pollitem;

  /** Contiguous array of poll items.
   */
  Pollitem *pollitems_;

  /** Size of the pollitems array.
   */
  int pollitems_size_;

  /** Resolve idx to actual position in pollitems.
   * (needed because pollitems can move due to array compaction).
   */
  int *idx_to_pos_;

  /** Resolve pos to idx.
   */
  int *pos_to_idx_;

  /** Number of slots used in the pollitems.
   */
  int used_count_;

  /** Event count (set during poll operation).
   */
  int event_count_;

  /** We set this to true when we receive a SIGINT. This enables
   * poll to return false.
   */
  bool interrupted_;

  /** Used to get back to the current poll in the interrupt handler.
   */
  static pthread_key_t sThisKey;
public:

  /** Create a poller and reserve free slots.
   */
  Poller(int reserve=8);

  ~Poller() {
    if (pollitems_)  free(pollitems_);
    if (idx_to_pos_) free(idx_to_pos_);
    if (pos_to_idx_) free(pos_to_idx_);
  }

  /** Polls for new events.
   * @return true on success, false on interruption.
   * @param wake time using monotonic clock in seconds.
   */
  bool poll(double wake_at) {
    double start = lens::elapsed();
    time_t timeout;
    
    if (wake_at < 0) {
      timeout = -1;
    } else {
      timeout = (wake_at - lens::elapsed()) * 1000;
      if (timeout < 0) {
        timeout = 0;
      }
    }

    // interruption can occur between poll operations
    if (interrupted_) return false;

    // poll expects milliseconds
    event_count_ = ::poll(pollitems_, used_count_, timeout);
    if (event_count_ < 0) {
      // error or interruption
      event_count_ = 0;
      if (!interrupted_) {
        throw dub::Exception("An error occured during poll (%s)", strerror(errno));
      } else {
        return false;
      }
    } else if (event_count_ == 0) {
      // timed out
      // remaining time to sleep in seconds
      double remaining = (wake_at - lens::elapsed()) * 1000.0;
      if (remaining > 0) lens::millisleep(remaining);    
    }

    return true;
  }

  /** Return a table with all event idx or nil.
   */
  LuaStackSize events(lua_State *L) {
    if (!event_count_) return 0;
    lua_newtable(L);
    // <table>
    int pos = 0;
    for(int i=0; i < used_count_; ++i) {
      Pollitem *item = pollitems_ + i;
      if (item->revents) {
        lua_pushnumber(L, pos_to_idx_[i]);
        // <table> <idx>
        lua_rawseti(L, -2, ++pos);
      }
      if (pos == event_count_) break;
    }
    event_count_ = 0;
    return 1;
  }

  int add(int fd, int events) {
    return addItem(fd, events);
  }

  /** Modify an item's events by its id.
   */
  void modify(int idx, int events, lua_State *L) {
    assert(events);
    assert(idx < pollitems_size_ && idx >= 0);
    Pollitem *item = pollitems_ + idx_to_pos_[idx];
    item->events = events;
    int top = lua_gettop(L);
    // <self> <idx> <events> <new_fd>
    if (top > 3) {
      int fd = dub::checkint(L, 4);
      item->fd = fd;
    }
  }

  /** Remove an item by its id.
   */
  void remove(int idx) {
    if (idx < 0 || idx >= pollitems_size_) {
      // Allready removed = bug.
      throw dub::Exception("Invalid index '%i'.", idx);
    }
    int last_pos = used_count_ - 1;
    int pos = idx_to_pos_[idx];
    if (pos == -1) {
      // Allready removed = bug.
      throw dub::Exception("Element '%i' removed twice.", idx);
    }
    idx_to_pos_[idx] = -1; // now free
    --used_count_;
    if (pos == last_pos) {
      // we removed the last element, we are done.
      return;
    }
    // move last item in the position where idx was
    int last_idx = pos_to_idx_[last_pos];
    pos_to_idx_[last_pos] = -1;
    idx_to_pos_[last_idx] = pos;
    pos_to_idx_[pos] = last_idx;
    // pollitems_[pos] <== pollitems_[last_pos];
    memcpy(pollitems_ + pos, pollitems_ + last_pos, sizeof(Pollitem));
  }

  int count() {
    return used_count_;
  }

  /** Used for testing only.
   * @return pos or nil
   */
  LuaStackSize idxToPos(int idx, lua_State *L) {
    if (idx >= pollitems_size_ || idx < 0) return 0;
    lua_pushnumber(L, idx_to_pos_[idx]);
    return 1;
  }

  /** Used for testing only.
   * @return pos or nil
   */
  LuaStackSize posToIdx(int pos, lua_State *L) {
    if (pos >= pollitems_size_ || pos < 0) return 0;
    lua_pushnumber(L, pos_to_idx_[pos]);
    return 1;
  }

  /** Used for testing only.
   * @return fd or nil
   */
  LuaStackSize posToFd(int pos, lua_State *L) {
    if (pos >= used_count_ || pos < 0) return 0;
    lua_pushnumber(L, pollitems_[pos].fd);
    return 1;
  }
  
  /** Used for testing only.
   * @return fd or nil
   */
  LuaStackSize posToEvent(int pos, lua_State *L) {
    if (pos >= used_count_ || pos < 0) return 0;
    lua_pushnumber(L, pollitems_[pos].events);
    return 1;
  }

private:
  int addItem(int fd, int events) {
    // printf("addItem fd:%i\n", fd);
    if (used_count_ >= pollitems_size_) {
      // we need more space: realloc
      int *sptr = (int*)realloc(idx_to_pos_, pollitems_size_ * 2 * sizeof(int));
      if (!sptr) {
        throw dub::Exception("Could not reallocate %i pollitems.", pollitems_size_ * 2);
      }
      idx_to_pos_ = sptr;
      sptr = NULL;
      sptr = (int*)realloc(pos_to_idx_, pollitems_size_ * 2 * sizeof(int));
      if (!sptr) {
        throw dub::Exception("Could not reallocate %i pollitems.", pollitems_size_ * 2);
      }
      pos_to_idx_ = sptr;
      Pollitem *ptr = (Pollitem*)realloc(pollitems_, pollitems_size_ * 2 * sizeof(Pollitem));
      if (!ptr) {
        throw dub::Exception("Could not reallocate %i pollitems.", pollitems_size_ * 2);
      }
      pollitems_ = ptr;
      // clear new space (same size as pollitems_size_ because we double).
      memset(idx_to_pos_+ used_count_, -1, pollitems_size_ * sizeof(int));
      memset(pos_to_idx_+ used_count_, -1, pollitems_size_ * sizeof(int));
      memset(pollitems_ + used_count_,  0, pollitems_size_ * sizeof(Pollitem));
      pollitems_size_ = 2 * pollitems_size_;
    }
    int pos = used_count_;
    ++used_count_;
    Pollitem *item = pollitems_ + pos;
    item->fd = fd;
    item->events = events;
    // Hasn't been moved yet: idx == position
    // find a free idx
    int idx;
    for(idx = 0; idx < pollitems_size_; ++idx) {
      if (idx_to_pos_[idx] < 0) break;
    }
    idx_to_pos_[idx] = pos;
    pos_to_idx_[pos] = idx;

    return idx;
  }

  static void sInterrupted(int i) {
    signal(i, SIG_DFL); // double interrupt == kill
    Poller *p = (Poller*)pthread_getspecific(sThisKey);
    p->interrupted_ = true;
    // continue
  }

  void setupInterruptHook() {
    Poller *p = (Poller*)pthread_getspecific(sThisKey);
    if (!p) {
      // only register first (main) Scheduler
      pthread_setspecific(sThisKey, (void*)this);
      signal(SIGINT, sInterrupted);
    }
  }
};
} // lens

#endif // LUBYK_INCLUDE_LENS_POLLER_H_
