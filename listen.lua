local lens = require 'lens'
local File = lens.File
local Write, Delete = File.WriteEvent, File.DeleteEvent

lens.run(function()
-- redo = redo or lens.FileRedo()
  -- This is only run once, even if the code is reloaded because 'lens.run'
  -- ignores calls when running.
  local thread = lens.Thread(function()
    local file = lens.File('listen.lua', File.Events)
    while true do
      local ev = file:events(Delete + Write)
      if ev == Delete + Write or ev == Delete then
        -- Most editors move, write new, delete. We must get fd of new file.
        file:close()
        file = File('listen.lua', File.Events)
      end
      -- protect in case of errors
      lens.Thread(function()
        local func = loadfile('listen.lua')
        -- Avoids yield across metamethod/C-call boundary that appears when
        -- C function 'dofile' is called.
        func()
      end)
    end
  end)
end)

tim = tim or lens.Timer(0.2)

i = i or 0
function tim:timeout()
  i = i + 1
  print('hop', i)
end

-- lens.run(function()
--   -- Listen for file changes and reload current script if needed.
--   lens.FileRedo()
-- end)

