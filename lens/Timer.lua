--[[------------------------------------------------------

  # lens.Timer

  The Timer contains a callback to execute a function at
  regular intervals. This timer does not drift (uses OS monotonic
  clock).

  Usage:

    local i = 0
    local tim = lens.Timer(0.5, function()
      print('Hello', lens.elapsed())
      i = i + 1
      if i == 10 then
        -- Use return value to set new interval. A
        -- return value of 0 stops the timer.
        return 0
      end
    end)

--]]------------------------------------------------------
local lub     = require 'lub'
local lens    = require 'lens'
local lib     = lub.class 'lens.Timer'
local assert, setmetatable, running,           yield,           floor,      elapsed   = 
      assert, setmetatable, coroutine.running, coroutine.yield, math.floor, lens.elapsed

local Thread  = lens.Thread.new
local run

-- # Constructor

-- Create a new timer with a give `interval` in seconds and a callback function.
-- The callback can also be set as #timeout method on the returned object.
function lib.new(interval, callback)
  if not running() then
    error('Cannot create timer outside of running scheduler.')
  end
  local self = {
    interval = interval,
  }
  setmetatable(self, lib)
  self.cb = function() run(self) end
  self.timeout = callback or function() end
  self.sched = yield('sched')
  self:start()
  return self
end

-- # Start, stop, phase

-- Start timer. The `start_in_seconds` parameter is the delay to start the
-- timer. For precise phase synchronization with other timers, use #startAt.
-- For irregular timers, you should use #timeout return value instead.
function lib:start(start_in_seconds)
  assert(self.interval > 0, 'Cannot run timer with negative or zero interval.')
  if self.thread then
    -- reschedule
    self.thread:kill()
    self.thread = nil
  end

  if not start_in_seconds then
    -- start or reschedule right away
    self.thread = Thread(self.cb, nil, self.sched)
  else
    self.thread = Thread(self.cb, start_in_seconds + elapsed(), self.sched)
  end

  local restart_func
  restart_func = function(at, sched)
    self.thread = Thread(self.cb, at + self.interval, sched)
    self.thread.restart = restart_func
  end
  self.thread.restart = restart_func
end


-- Start timer with a precise starting time. This can be used to set precision 
-- phase between timers.
function lib:startAt(at)
  assert(self.interval > 0, 'Cannot run timer with negative or zero interval.')
  if self.thread then
    -- reschedule
    self.thread:kill()
    self.thread = nil
  end

  self.thread = Thread(self.cb, at)
end

-- Stop the timer.
function lib:stop()
  if self.thread then
    self.ref = self.thread.at
    self.thread:kill()
    self.thread = nil
  end
end

-- # Methods

-- Return true if the timmer is running.
function lib:running()
  return self.thread and true or false
end

-- Change interval but do not restart timer (effect on next trigger).
function lib:setInterval(interval)
  self.interval = interval
end

-- # Callback

-- Method called when the timer fires. If you return a number from this
-- method, it will be used as the next interval but it does not alter the
-- timer interval.
--
-- Returning a number can be used for irregular timers or to change phase. A
-- returned value of `0` stops the timer.
-- function lib:timeout()

----------------------------------------------- PRIVATE


-- nodoc
function run(self)
  if self.interval > 0 then
    while self.thread do
      local interval = self:timeout()
      if interval then
        if interval <= 0 then
          self:stop()
          break
        else
          -- do not change interval but set next trigger, keeping thread.at
          -- offset.
          yield('wait', interval)
        end
      else
        yield('wait', self.interval)
      end
    end
  end
end  

return lib
