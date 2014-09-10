--[[------------------------------------------------------

  # Thread

  ## Thread garbage collection

  As long as a thread is scheduled to run (active loop, listening for, it will be kept in memory.


  ## Error handling

  When an error occurs in a thread, the related coroutine dies. If the thread has a `restart` function
  set, this function is called with the thread's time reference and the scheduler. Here is the example used
  to restart the Timer thread in case of errors in the timeout function:

    local restart_func
    restart_func = function(at, sched)
      self.thread = Thread(self.cb, at + self.interval, sched)
      self.thread.restart = restart_func
    end
    self.thread.restart = restart_func

  Error handling can be personalized by the thread by setting an `error` function.

--]]------------------------------------------------------
local lub     = require 'lub'
local lens    = require 'lens'
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
-- `sched` is provided, this scheduler will be used instead of using yield. If
-- the yield call fails due to a C-call boundary, the default #lens.sched
-- scheduler will be used.
function lib.new(func, at, sched)
  local self = make(func, at)
  if sched then
    sched:createThread(self)
  elseif running() then
    local ok, err = pcall(function()
      yield('create', self)
    end)
    if not ok then
      -- Trying to yield across c boundary. Using default scheduler.
      lens.sched():createThread(self)
    end
  else
    lens.sched():createThread(self)
  end
  return self
end


function lib:kill()
  self.sched:killThread(nil, self)
  -- We avoid using 'yield' in case the kill happens from a C callback.
end

-- Running thread will wait for this thread to finish.
function lib:join()
  if self.co then
    -- ignore if thread is already dead
    yield('join', self)
  end
end

-- PRIVATE
--

return lib

