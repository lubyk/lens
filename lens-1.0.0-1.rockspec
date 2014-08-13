package = "lens"
version = "1.0.0-1"
source = {
  url = 'https://github.com/lubyk/lens/archive/REL-1.0.0.tar.gz',
  dir = 'lens-REL-1.0.0',
}
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
  homepage = "http://doc.lubyk.org/lens.html",
  license = "MIT"
}

dependencies = {
  "lua >= 5.1, < 5.3",
  "lub >= 1.0.3, < 2.0",
}
build = {
  type = 'builtin',
  modules = {
    -- Plain Lua files
    ['lens'           ] = 'lens/init.lua',
    ['lens.File'      ] = 'lens/File.lua',
    ['lens.FileWatch' ] = 'lens/FileWatch.lua',
    ['lens.Finalizer' ] = 'lens/Finalizer.lua',
    ['lens.Poller'    ] = 'lens/Poller.lua',
    ['lens.Popen'     ] = 'lens/Popen.lua',
    ['lens.Scheduler' ] = 'lens/Scheduler.lua',
    ['lens.Thread'    ] = 'lens/Thread.lua',
    ['lens.Timer'     ] = 'lens/Timer.lua',
    -- C module
    ['lens.core'      ] = {
      sources = {
        'src/file.cpp',
        'src/lens.cpp',
        'src/poller.cpp',
        'src/popen.cpp',
        'src/bind/dub/dub.cpp',
        'src/bind/lens_core.cpp',
        'src/bind/lens_File.cpp',
        'src/bind/lens_Finalizer.cpp',
        'src/bind/lens_Poller.cpp',
        'src/bind/lens_Popen.cpp',
      },
      incdirs   = {'include', 'src/bind'},
      libraries = {'stdc++'},
    },
  },
  platforms = {
    linux = {
      modules = {
        ['lens.core'] = {
          libraries = {'stdc++', 'rt'},
        },
      },
    },
    macosx = {
      modules = {
        ['lens.core'] = {
          sources = {
            [11] = 'src/macosx/poller.mm',
          },
          libraries = {'stdc++', '-framework Foundation', '-framework Cocoa', 'objc'},
        },
      },
    },
  },
}

