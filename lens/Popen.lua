--[[------------------------------------------------------

  # Popen

--]]------------------------------------------------------
local lens  = require 'lens'

local core  = require 'lens.core'
local lib   = core.Popen
local new   = core.Popen.new

local           OK,           Wait =
      lens.File.OK, lens.File.Wait

local           yield,     readLine,     write,        len,        sub =
      coroutine.yield, lib.readLine, lib.write, string.len, string.sub


--nodoc (used for testing)
lib.__readLine = readLine

--nodoc (used for testing)
lib.__write = write

-- Create a new pipe with the given cmd executed in another process. Mode can
-- be either 'r' (read) or 'w' (write). Default is 'r'.
--
-- TODO: implement bidirectional pipes.
function lib.new(cmd, mode)
  -- Optimize fd ? Implies using a table for self...
  return new(cmd, mode or 'r')
end

-- These lua helpers must be copied from File in each sub-class in order to
-- avoid casting resolution overhead. :-(

-- Read a line. Returns a string or nil on EOF.
function lib:readLine()
  local line, op = readLine(self)
  while op == Wait do
    local l
    yield('read', self:fd())
    l, op = readLine(self)
    line = line .. l
  end
  if op == OK then
    return line
  else
    -- EOF
    return nil
  end
end

-- Write a string to a file.
function lib:write(str)
  local wsz, op = write(self, str)
  while op == Wait do
    str = sub(str, wsz+1)
    yield('write', self:fd())
    wsz, op = write(self, str)
  end
  -- done
end

return lib

