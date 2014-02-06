--[[------------------------------------------------------

  # Popen

--]]------------------------------------------------------
local core  = require 'lens.core'
local lib   = core.Popen
local new   = core.Popen.new

-- Create a new pipe with the given cmd executed in another process. Mode can
-- be either 'r' (read) or 'w' (write). Default is 'r'.
--
-- TODO: implement bidirectional pipes.
function lib.new(cmd, mode)
  -- Optimize fd ? Implies using a table for self...
  return new(cmd, mode or 'r')
end

-- Read a line. Returns a string or nil on EOF.
-- function lub.readLine()

-- nodoc
lub.readLine = lub.File.readLine

return lib

