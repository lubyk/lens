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
--local core = {init = function() end}
local core = require 'lens.core'

local private = {}
local yield            =
      coroutine.yield
local running

-- ## Dependencies
--

-- Current version respecting [semantic versioning](http://semver.org).
lib.VERSION = '1.0.0'

lib.DEPENDS = { -- doc
  -- Compatible with Lua 5.1, 5.2 and LuaJIT
  "lua >= 5.1, < 5.3",
  -- Uses [Lubyk base library](http://doc.lubyk.org/lub.html)
  'lub >= 1.0.3, < 2.0',
}

-- nodoc
lib.DESCRIPTION = {
  summary = "Lubyk networking and scheduling.",
  detailed = [[
  lens.Scheduler: core scheduling class.

  lens.Poller: fast poller with nanosecond precision.

  lens.Thread: threading class to use with scheduler.

  lens.Timer: precise non-drifting timer.

  lens.Finalizer: run code on garbage collection.

  lens.Popen: pipe working with scheduler (non-blocking).
  ]],
  homepage = "http://doc.lubyk.org/"..lib.type..".html",
  author   = "Gaspard Bucher",
  license  = "MIT",
}

-- nodoc
lib.BUILD = {
  github    = 'lubyk',
  includes  = {'include', 'src/bind'},
  libraries = {'stdc++'},
  platlibs  = {
    linux   = {'stdc++', 'rt'},
    macosx  = {
      'stdc++',
      '-framework Foundation',
      '-framework Cocoa',
      'objc',
    },
  },
}

-- # Scheduling
-- 
-- Note that all scheduling functions operate by using `coroutine.yield` and
-- it is possible to use these equivalent methods instead.

-- Enter event loop. This is equivalent to creating a lens.Scheduler and calling
-- run on it with some differences: 
--
-- * The initial call to `lens.run` never returns
-- * Repeated calls to this function are ignored which this means that code
--   after `lens.run` is executed.
--
-- This is typically used for [live coding](examples.lens.file_redo.html).
function lib.run(func)
  if not running then
    running = true
    lib.sched():run(func)
    -- Make sure no code is executed below 'lens.run'
    os.exit(0)
  end
end

-- Stop scheduler and exit.
-- Usage:
--
--   lens.halt()
--   -- is the same as
--   coroutine.yield('halt')
function lib.halt()
  yield('halt')
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

-- # Helpers

local classes = {}

-- This is like lub.class but with class reloading support. Should only be used
-- during development. If `skip_lub` is true, do not call lub.class and alter
-- table setup (this is needed when wrapping class objects as library items).
function lib.class(name, tbl, skip_lub)
  if classes[name] then return classes[name] end
  local class
  if skip_lub then
    class = tbl
  else
    class = lub.class(name, tbl)
  end
  classes[name] = class
  -- Install file watch
  lib.FileWatch(lub.path('&', 3))
  
  return class
end

local sched

-- Return default scheduler (the one used in lens.run and by lens.FileWatch or
-- lens.Thread when called without a coroutine running).
function lib.sched()
  if not sched then
    sched = lib.Scheduler()
  end
  return sched
end

-- # Classes

return lib
