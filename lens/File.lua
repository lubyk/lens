--[[------------------------------------------------------

  # File

--]]------------------------------------------------------
local core  = require 'lens.core'
local lib   = core.File

local yield,           readLine =
      coroutine.yield, core.Popen.readLine
local OK, Wait = core.File.OK, core.File.Wait

-- Read a line. Returns a string or nil on EOF.
function lib:readLine()
  local op, line = readLine(self)
  while op == Wait do
    local l
    yield('read', self:fd())
    op, l = readLine(self)
    line = line .. l
  end
  if op == OK then
    return line
  else
    -- EOF
    return nil
  end
end

return lib

