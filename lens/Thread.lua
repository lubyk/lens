--[[------------------------------------------------------

  # Thread

  ## Thread garbage collection

  As long as a thread is scheduled to run (active loop, listening for, it will be kept in memory.

--]]------------------------------------------------------
local lub     = require 'lub'
local lib     = lub.class 'lens.Thread'
local assert, setmetatable, yield,           create,           running   = 
      assert, setmetatable, coroutine.yield, coroutine.create, coroutine.running

-- We need a 'make' function so the Scheduler can create threads without
-- yielding to add them to the queue.
local function make(func, at)
  assert(func, 'Cannot create thread without function.')
  local self = {
    at = at or 0,
    -- In case we must restart thread on error.
    func = func,
    co = create(func)
  }
  return setmetatable(self, lib)
end

-- nodoc
lib.make = make

-- Create a new Thread object and insert it inside the
-- currently running scheduler's event queue. The thread is retained in the
-- scheduler as long as it is alive (function has not reached the end). If
-- `sched` is provided, this scheduler will be used instead of using yield.
function lib.new(func, at, sched)
  if not running() then
    error('Cannot create thread outside of running scheduler.')
  end
  local self = make(func, at)
  if sched then
    sched:createThread(self)
  else
    yield('create', self)
  end
  return self
end


function lib:kill(sched)
  self.sched:killThread(nil, self)
  -- We avoid using 'yield' in case the kill happens from a C callback.
end

-- PRIVATE
--

return lib

