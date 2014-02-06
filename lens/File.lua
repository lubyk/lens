--[[------------------------------------------------------

  # File

--]]------------------------------------------------------
local core  = require 'lens.core'
local lib   = core.File

local     OK,     Wait =
      lib.OK, lib.Wait

local           yield,     readLine,     write,        len,        sub =
      coroutine.yield, lib.readLine, lib.write, string.len, string.sub

-- These lua helpers must be copied in each sub-class in order to avoid casting
-- resolution overhead. :-(

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

