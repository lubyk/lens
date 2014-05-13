--[[------------------------------------------------------

  lens.Timer
  --------

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

function lib.new(interval, func)
  if not running() then
    error('Cannot create timer outside of running scheduler.')
  end
  local self = {
    interval = interval,
  }
  self.cb = function() self:run() end
  if func then
    self.timeout = func
  end
  return setmetatable(self, lib)
end

-- The callback function. Note that first argument is the timer
-- itself.
function lib:timeout() end

-- Start timer. The `start_in_seconds` parameter is the delay to start the
-- timer. This delay can be used for precise phase synchronization with other
-- timers or to use an irregular timer.
function lib:start(start_in_seconds)
  assert(self.interval > 0, 'Cannot run timer with negative or zero interval.')
  if self.thread then
    print('kill')
    -- reschedule
    self.thread:kill()
    self.thread = nil
  end
  print('starting again')

  if not start_in_seconds then
    -- start or reschedule right away
    self.thread = Thread(self.cb)
  else
    print('create new thread', start_in_seconds + elapsed())
    self.thread = Thread(self.cb, start_in_seconds + elapsed())
  end
end

function lib:run()
  if self.interval > 0 then
    while self.thread do
      local interval = self:timeout()
      if interval then
        if interval <= 0 then
          self:setInterval(0)
          break
        else
          self.interval = interval
          yield('wait', interval)
        end
      else
        yield('wait', self.interval)
      end
    end
  end
end  

function lib:setInterval(interval)
  self.interval = interval

  if self.thread then
    -- Running timer: remove from scheduler.
    local ref = self.thread.at
    if interval == 0 then
      -- Stop.
      self:stop()
    else
      -- Change interval and restart with same phase.
      local diff = elapsed() - ref
      local c = floor(diff / self.interval) + 1
      -- FIXME: we should use 'wakeAt' so that we can specify absolute time and
      -- avoid drift by calls to elapsed().
      -- self:wakeAt(ref + interval * c)
      self:start(self.interval * c)
    end
  else
    -- Not running.
    self.interval = interval
  end
end

function lib:join()
  if self.thread then
    self.thread:join()
  end
end

function lib:stop()
  if self.thread then
    self.ref = self.thread.at
    self.thread:kill()
    self.thread = nil
  end
end


return lib
