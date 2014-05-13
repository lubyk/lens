--[[--------------------
  # Lubyk networking and scheduling <a href="https://travis-ci.org/lubyk/lens"><img src="https://travis-ci.org/lubyk/lens.png" alt="Build Status"></a> 

  Single-threaded, fast and predictable scheduling with nanosecond precision.

  <html><a href="https://github.com/lubyk/lens"><img style="position: absolute; top: 0; right: 0; border: 0;" src="https://s3.amazonaws.com/github/ribbons/forkme_right_green_007200.png" alt="Fork me on GitHub"></a></html>
  
  *MIT license* &copy Gaspard Bucher 2014.

  ## Installation
  
  With [luarocks](http://luarocks.org):

    $ luarocks install lens

--]]--------------------
local lub  = require 'lub'
local lib  = lub.Autoload 'lens'
local core = require 'lens.core'

local private = {}
local yield            =
      coroutine.yield
local running, sched

-- ## Dependencies
--

-- Current version respecting [semantic versioning](http://semver.org).
lib.VERSION = '1.0.0'

lib.DEPENDS = { -- doc
  -- Compatible with Lua 5.1, 5.2 and LuaJIT
  "lua >= 5.1, < 5.3",
  -- Uses [Lubyk base library](http://doc.lubyk.org/lub.html)
  'lub >= 1.0.3, < 1.1',
}

-- # Scheduling
-- 
-- Note that all scheduling functions operate by using `coroutine.yield` and
-- it is possible to use these equivalent methods instead.

-- Enter event loop. This is equivalent to creating a lens.Scheduler and calling
-- run on it. If this function is called while the scheduler is already running,
-- it does nothing.
function lib.run(func)
  if not running then
    running = true
    if not sched then
      sched = lib.Scheduler()
    end
    sched:run(func)
    running = false
  end
end

-- Sleep for `sec` seconds (precision depends on OS). Using nanosecond for
-- precise sleep on linux and macosx. note that this should not be used to
-- create a precise timer: use lens.Timer for non-drifting timers.
--
-- Usage:
--
--   lens.sleep(0.5)
--   -- is the same as
--   coroutine.yield('sleep', sec)
function lib.sleep(sec)
  yield('sleep', sec)
end


-- Wait until the filedescriptor `fd` is ready for reading. Since some pollers
-- are edge based, make sure that fd is *not readable* before calling this.
--
-- this function simply does:
--
--   lens.waitRead(fd)
--   -- is the same as
--   coroutine.yield('read', fd)
function lib.waitRead(fd)
  yield('read', fd)
end

-- Wait until the filedescriptor `fd` is ready for writing. Since some pollers
-- are edge based, make sure that fd is *not writeable* before calling this.
--
-- this function simply does:
--
--   lens.waitWrite(fd)
--   -- is the same as
--   coroutine.yield('write', fd)
function lib.waitWrite(fd)
  yield('write', fd)
end

-- nodoc
lib.millisleep = core.millisleep

-- Get *monotonic* elapsed time since some arbitrary point in time. This timer
-- is as precise as the OS permits and does not jump forward or back. When a
-- computer is connected to atomic clocks with ntp protocol, this clock is
-- adjusted by altering it's speed and should be very precise but can have some
-- jitter.
--
-- Uses `mach_absolute_time` on macosx, `clock_gettime` (CLOCK_MONOTONIC) on
-- linux and `QueryPerformanceCounter` on windows.
--
-- function lib.elapsed()

-- nodoc
lib.elapsed = core.elapsed

-- Initialize library.
core.init()


-- # Classes

return lib
