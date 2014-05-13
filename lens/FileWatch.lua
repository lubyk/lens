--[[------------------------------------------------------

  # FileWatch

  Watches a file and executes the #changed callback if the
  content changes. Uses kevent or other OS optimized notification
  so that listening to files takes as few resources as
  possible.

  See [LiveCoding](examples.lens.LiveCoding.html) example.

--]]------------------------------------------------------
local lub  = require 'lub'
local lens = require 'lens'
local lib  = lub.class 'lens.FileWatch'

local File = lens.File
local Write, Delete = File.WriteEvent, File.DeleteEvent
local defaultCallback, watch

-- # Constructor
--
-- Start listening to the file provided by `path`. If `path`
-- is omitted, the default is the calling script's path. If `callback` is
-- omitted, the default is to reload the script.
function lib.new(path, callback)
  local self = { path = path or lub.path('&', 3) }
  setmetatable(self, lib)
  self.thread = lens.Thread(function()
    watch(self)
  end)
  self.changed = callback or defaultCallback
  return self
end

-- # Callback
--
-- The callback function is called when the watched file changes. The default
-- implementation is to reload the file (execute a dofile).
-- function lib:changed()

-- =================================== PRIVATE

function watch(self)
  local path = self.path
  local file = lens.File(path, File.Events)
  while true do
    -- We start with a first reload to allow writing lens.run at top of file.
    lens.Thread(function()
      -- Use a thread to protect in case of errors so that we do not crash
      -- our watching thread.
      self:changed()
    end)
    local ev = file:events(Delete + Write)
    if ev == Delete + Write or ev == Delete then
      -- Most editors move, write new, delete. We must get fd of new file.
      file:close()
      file = File(path, File.Events)
    end
  end
end

function defaultCallback(self)
  -- We cannot call plain `dofile` because of yield across metamethod/C-call
  -- boundary error.
  local func = loadfile(self.path)
  func()
end

return lib


