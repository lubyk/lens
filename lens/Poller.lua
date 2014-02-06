--[[------------------------------------------------------

  # default Poller

  Normally, end users do not interact directly with the poller.
  It is used internally by lens.Scheduler to wait on file
  descriptors and sleep. This poller uses fast poll API and nanosleep
  for precise operation.

-- # Poller API
-- TODO  
--

--]]------------------------------------------------------
local core  = require 'lens.core'
local lib   = core.Poller

-- Create a new poller. Optional `reserve` argument is used to reserve slots
-- in memory for items to poll on (default = 8).
-- function lib.new(reserve)

-- Polls for new events with a maximal waiting time of `timeout`. Returns `true`
-- on success and `false` on interruption.
-- function lib.poll(timeout)

-- Return a table with all event idx or nil. Used after a call to #poll.
-- function lib.events()

return lib
