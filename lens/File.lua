--[[------------------------------------------------------

  # File

--]]------------------------------------------------------
local core  = require 'lens.core'
local lib   = core.File

local     OK,     Wait, None,     new =
      lib.OK, lib.Wait, lib.None, lib.new

local           yield,     readLine,     write,        len,        sub =
      coroutine.yield, lib.readLine, lib.write, string.len, string.sub

-- These lua helpers must be copied in each sub-class in order to avoid casting
-- resolution overhead. :-(

-- Create a new file with the given mode. Mode values are:
--
-- + Read:   prepare file for reading
-- + Write:  prepare file for writing
-- + Events: listen to file changes
--
function lib.new(path, mode)
  local self = {
    super = new(path, mode),
    path  = path,
  }
  return setmetatable(self, lib)
end

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

-- # Events

-- Listen for OS notifications on file changes. The flags determine which
-- notifications are watched. You can combine flags by adding them.
-- The function returns the notified event as a number.
--
-- + DeleteEvent:  Vnode was removed.
-- + WriteEvent:   Data contents changed.
-- + ExtendEvent:  Size increased.
-- + AttribEvent:  Attributes changed.
-- + LinkEvent:    Link count changed.
-- + RenameEvent:  Vnode was renamed.
-- + RevokeEvent:  Vnode access was revoked.
-- + NoneEvent:    No specific vnode event: to test for EVFILT_READ activation.
--
-- Usage example:
--
--   local lens = require 'lens'
--   local File = lens.File
--   local DELETE, WRITE = File.DeleteEvent, File.WriteEvent
--   f = lens.File('foo.lua', File.Events)
--   while true do
--     local ev = f:events(DELETE + WRITE)
--     if ev == DELETE then
--       print(f.path, 'deleted')
--       break
--     else
--       print(f.path, File.EventName[ev])
--     end
--   end
function lib:events(flags)
  return yield('vnode', self:fd(), flags)
end

-- Translate an event number to a table with keys representing active flags.
-- function lib.eventMap

-- nodoc
lib.eventMap = core.Poller.eventMap


return lib

