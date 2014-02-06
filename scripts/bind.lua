--[[------------------------------------------------------

  lens bindings generator
  ------------------------

  This uses the 'dub' tool and Doxygen to generate the
  bindings for mimas.

  Input:  headers in 'include/lens'
  Output: cpp files in 'src/bind'

--]]------------------------------------------------------
local lub = require 'lub'
local dub = require 'dub'

local ins = dub.Inspector {
  INPUT    = {
    lub.path '|../include/lens',
  },
  --doc_dir = base .. '/tmp',
  --html = true,
  --keep_xml = true,
}

local binder = dub.LuaBinder()

binder:bind(ins, {
  output_directory = lub.path '|../src/bind',
  -- Remove this part in included headers
  header_base = lub.path '|../include',
  -- Create a single library.
  single_lib = 'lens',
  -- Open the library with require 'lens.core' (not 'lens')
  luaopen    = 'lens_core',
})
