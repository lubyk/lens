#!/usr/bin/env lua
local lub = require 'lub'

local lib = require 'lens'

local def = {
  description = {
    summary = "Lubyk networking and scheduling.",
    detailed = [[
    lens.Scheduler: core scheduling class.

    lens.Poller: fast poller with nanosecond precision.

    lens.Thread: threading class to use with scheduler.

    lens.Timer: precise non-drifting timer.

    lens.Finalizer: run code on garbage collection.

    lens.Popen: pipe working with scheduler (non-blocking).
    ]],
    homepage = "http://doc.lubyk.org/"..lib.type..".html",
    author   = "Gaspard Bucher",
    license  = "MIT",
  },

  includes  = {'include', 'src/bind'},
  libraries = {'stdc++'},
  platlibs = {
    linux   = {'stdc++', 'rt'},
  },
}

-- Platform specific sources or link libraries
def.platspec = def.platspec or lub.keys(def.platlibs)

--- End configuration

local tmp = lub.Template(lub.content(lub.path '|rockspec.in'))
lub.writeall(lib.type..'-'..lib.VERSION..'-1.rockspec', tmp:run {lib = lib, def = def, lub = lub})

tmp = lub.Template(lub.content(lub.path '|dist.info.in'))
lub.writeall('dist.info', tmp:run {lib = lib, def = def, lub = lub})

tmp = lub.Template(lub.content(lub.path '|CMakeLists.txt.in'))
lub.writeall('CMakeLists.txt', tmp:run {lib = lib, def = def, lub = lub})


