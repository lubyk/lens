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

// Maximum return event count
#define MAX_REVENT_COUNT 20
// Use kevent
#define LUBYK_POLLER_KEVENT
#define DEBUG 0

#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%12s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#include <errno.h>  // errno
#include <string.h> // strerror()

#include <stdlib.h> // rand()
#include <time.h>   // time()
#include <assert.h> // assert()
#include <signal.h> // signal(), SIG_DFL, ...



#ifdef LUBYK_POLLER_KEVENT
#include <sys/event.h>
#else
#include <poll.h>   // poll()
#endif

namespace lens {

/** lens basic Poller.
 *
 * @dub string_format: %%f
 *      string_args: self->count()
 *      push: dub_pushobject
 *      ignore: resume, backPoll
 */
class Poller : public dub::Thread {

#ifdef LUBYK_POLLER_KEVENT
  typedef struct kevent Pollitem;

  /** Kevent queue id.
   */
  int kqueue_;

  Pollitem events_data_[MAX_REVENT_COUNT];
#else
  typedef struct pollfd Pollitem;
#endif

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

  /** This is used to pass 'wake_at' value from main thread to ext kevent
   * thread when running GUI.
   */
  double wake_at_;

  /** Return value from poll function used in external thread.
   */
  bool retval_;

  /** Used to get back to the current poll in the interrupt handler.
   */
  static pthread_key_t sThisKey;
public:

  enum Filters {
    //  compatible with zmq
    Read  = 1, // POLLIN
    Write = 2, // POLLOUT
    VNode = 3,
  };

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
    double timeout;
    debug_print("Wake at:%.2f\n", wake_at);
    
    if (wake_at < 0) {
      timeout = -1;
    } else {
      timeout = wake_at - lens::elapsed();
      if (timeout < 0) {
        timeout = 0;
      }
    }

    debug_print("poll timeout:%f used:%i.\n", timeout, used_count_);

    // interruption can occur between poll operations
    if (interrupted_) return false;

#ifdef LUBYK_POLLER_KEVENT
    if (timeout >= 0) {
      int timeout_sec  = timeout;
      int timeout_nsec = (timeout - timeout_sec) * 1000000000; // nanosec
      struct timespec ttimeout = { timeout_sec, timeout_nsec};

      // Get new events.
      // kevent expects a timespec
      event_count_ = ::kevent(kqueue_, NULL, 0, events_data_, MAX_REVENT_COUNT, &ttimeout);
    } else {
      // negative timeout == wait forever
      event_count_ = ::kevent(kqueue_, NULL, 0, events_data_, MAX_REVENT_COUNT, NULL);
    }
#else
    // poll expects milliseconds
    // negative timeout == wait forever
    event_count_ = ::poll(pollitems_, used_count_, timeout * 1000);
#endif
    if (event_count_ < 0 || events_data_[0].flags == EV_ERROR) {
      // error or interruption
      event_count_ = 0;
      if (!interrupted_) {
#ifdef LUBYK_POLLER_KEVENT
        throw dub::Exception("An error occured during kevent (%s)", strerror(errno));
#else
        throw dub::Exception("An error occured during poll (%s)", strerror(errno));
#endif
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

  /** Moving kevent and polling to an external thread. This is required to
   * run OS event loop on main thread. This function never returns.
   */
  void runGUI(double wake_at);

  /** Called from background thread.
   */
  bool backPoll() {
    retval_ = poll(wake_at_);
    // wait for 'resume' to end.
    return retval_;
  }

  /** Called from main thread when external thread 'backPoll' returns.
   */
  bool resume() {
    bool res = true;
    if (!dub_pushcallback("resume")) return false;

    lua_pushboolean(dub_L, retval_);
    // <func> <self> <retval>
    dub_call(2, 1);

    // return value => wake_at or nil to halt.
    if (lua_isnumber(dub_L, -1)) {
      wake_at_ = lua_tonumber(dub_L, -1);
    } else {
      res = false;
    }

    lua_pop(dub_L, 1);

    return res;
  }

  /** Return a table with all event idx or nil.
   */
  LuaStackSize events(lua_State *L) {
    if (!event_count_) return 0;
    lua_newtable(L);
    // <table>
    int pos = 0;
#ifdef LUBYK_POLLER_KEVENT
    for(int i=0; i < event_count_; ++i) {
      Pollitem *item = &events_data_[i];
      // udata contains idx
      lua_pushnumber(L, (intptr_t)item->udata);
      // <table> <idx>
      lua_rawseti(L, -2, ++pos);
    }
#else
    for(int i=0; i < used_count_; ++i) {
      Pollitem *item = pollitems_ + i;
      if (item->revents) {
        lua_pushnumber(L, pos_to_idx_[i]);
        // <table> <idx>
        lua_rawseti(L, -2, ++pos);
      }
      if (pos == event_count_) break;
    }
#endif
    event_count_ = 0;
    return 1;
  }

  // This must be called for the given thread before poll is called again.
  int fflags(int idx) {
#ifdef LUBYK_POLLER_KEVENT
    assert(idx < pollitems_size_ && idx >= 0);
    int pos = idx_to_pos_[idx];
    return pollitems_[pos].fflags;
#else
    return 0;
#endif
  }

  // Translate event fflags to a table
  static LuaStackSize eventMap(int fflags, lua_State *L) {
#ifdef LUBYK_POLLER_KEVENT
    lua_newtable(L);
    if (fflags & NOTE_DELETE) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_DELETE);
    }
    if (fflags & NOTE_WRITE) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_WRITE);
    }
    if (fflags & NOTE_EXTEND) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_EXTEND);
    }
    if (fflags & NOTE_ATTRIB) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_ATTRIB);
    }
    if (fflags & NOTE_LINK) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_LINK);
    }
    if (fflags & NOTE_RENAME) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_RENAME);
    }
    if (fflags & NOTE_REVOKE) {
      lua_pushboolean(L, true);
      lua_rawseti(L, -2, NOTE_REVOKE);
    }
    // <table>
    return 1;
#else
    return 0;
#endif
  }

  int add(int fd, int filter, int fflags = 0) {
    debug_print("add fd:%i, filter:%i, flags:%i.\n", fd, filter, fflags);
    return addItem(fd, filter, fflags);
  }

  /** Modify an item's filter by its id.
   */
  void modify(int idx, int filter, lua_State *L) {
    debug_print("modify idx:%i, filter:%i.\n", idx, filter);
    assert(filter);
    assert(idx < pollitems_size_ && idx >= 0);
    Pollitem *item = pollitems_ + idx_to_pos_[idx];
    int fd     = -1;
    int fflags = 0;
    int top    = lua_gettop(L);
    // <self> <idx> <filter> <new_fd> (<flags>)
    if (top > 3) {
      fd = dub::checkint(L, 4);
      if (top > 4) {
        fflags = dub::checkint(L, 5);
      }
    }
#ifdef LUBYK_POLLER_KEVENT
    if (fd != -1) {
      // changed fd or identifier
      item->ident = fd;
    }
	
    switch(filter) {
      case Read:
        item->filter = EVFILT_READ;
        item->flags  = EV_ADD | EV_ENABLE | EV_CLEAR;
        break;
      case Write:
        item->filter = EVFILT_WRITE;
        item->flags  = EV_ADD | EV_ENABLE | EV_CLEAR;

        break;
      case VNode:
        item->filter = EVFILT_VNODE;
        item->flags  = EV_ADD | EV_ENABLE | EV_CLEAR;
        if (fflags == 0) {
          // default File flags
          fflags = NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE;
        }

        item->fflags = fflags;
       break;
      default:
        throw dub::Exception("Invalid filter value %i.", filter);
    }
    setKEvent(item);
#else
    // change kevent
    item->events = filter;
    if (fd != -1) {
      // changed fd
      item->fd = fd;
    }
#endif
  }

  /** Remove an item by its id.
   */
  void remove(int idx) {
    debug_print("remove idx:%i.\n", idx);
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
#ifdef LUBYK_POLLER_KEVENT
    Pollitem *item = pollitems_ + pos;
    item->flags = EV_DELETE;
    setKEvent(item);
#endif

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
#ifdef LUBYK_POLLER_KEVENT
    lua_pushnumber(L, pollitems_[pos].ident);
#else
    lua_pushnumber(L, pollitems_[pos].fd);
#endif
    return 1;
  }
  
  /** Used for testing only.
   * @return fd or nil
   */
  LuaStackSize posToEvent(int pos, lua_State *L) {
    if (pos >= used_count_ || pos < 0) return 0;
#ifdef LUBYK_POLLER_KEVENT
    lua_pushnumber(L, pollitems_[pos].filter);
#else
    lua_pushnumber(L, pollitems_[pos].events);
#endif
    return 1;
  }

private:
  int addItem(int fd, int filter, int fflags) {
    debug_print("addItem fd:%i\n", fd);
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
    // Hasn't been moved yet: idx == position
    // find a free idx
    intptr_t idx;
    for(idx = 0; idx < pollitems_size_; ++idx) {
      if (idx_to_pos_[idx] < 0) break;
    }
    idx_to_pos_[idx] = pos;
    pos_to_idx_[pos] = idx;

#ifdef LUBYK_POLLER_KEVENT
    switch(filter) {
      case Read:
        EV_SET(item, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR,
              fflags, 0, (void*)idx);

        break;
      case Write:
        EV_SET(item, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR,
              fflags, 0, (void*)idx);
        item->flags  = EVFILT_WRITE;

        break;
      case VNode:
        if (fflags == 0) {
          // default File flags
          fflags = NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE;
        }
        EV_SET(item, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
              fflags, 0, (void*)idx);

        break;
      default:
        throw dub::Exception("Invalid filter value %i.", filter);
    }                            
    setKEvent(item);
#else
    item->fd = fd;
    item->events = events;
#endif
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

#ifdef LUBYK_POLLER_KEVENT
  void setKEvent(Pollitem *item) {
    struct timespec timeout = {0, 0};
    struct kevent event_data;
    int res = kevent(kqueue_, item, 1, &event_data, 1, &timeout);
    if (res < 0 || event_data.flags == EV_ERROR) {
      throw dub::Exception("An error occured during set kevent (%s).", strerror(errno));
    }
  }
#endif
};
} // lens

#endif // LUBYK_INCLUDE_LENS_POLLER_H_
