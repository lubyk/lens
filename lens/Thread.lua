--[[------------------------------------------------------

  # Thread

  ## Thread garbage collection

  As long as a thread is scheduled to run (active loop, listening for, it will be kept in memory.

--]]------------------------------------------------------
local lub     = require 'lub'
local lib     = lub.class 'lens.Thread'
local assert, setmetatable, yield,           create,           running   = 
      assert, setmetatable, coroutine.yield, coroutine.create, coroutine.running

-- Use a 'make' function because the Scheduler needs to create threads without
-- yielding to add them to the queue.
local function make(func, at)
  assert(func, 'Cannot create thread without function.')
  local self = {
    at = at or 0,
    co = create(func)
  }
  return setmetatable(self, lib)
end

-- nodoc
lib.make = make

-- Create a new Thread object and insert it inside the
-- currently running scheduler's event queue.
function lib.new(func, at)
  if not running() then
    error('Cannot create thread outside of running scheduler.')
  end
  local self = make(func, at)
  yield('create', self)
  return self
end


function lib:kill()
  yield('kill', self)
end
-- PRIVATE
--

return lib

