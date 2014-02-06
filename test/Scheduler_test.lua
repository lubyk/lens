--[[------------------------------------------------------

  lens.Scheduler
  ------------


--]]------------------------------------------------------
local lens   = require 'lens'
local lut    = require 'lut'
local should = lut.Test 'lens.Scheduler'

local Scheduler = lens.Scheduler

function should.create()
  local s = Scheduler()
  assertEqual('lens.Scheduler', s.type)
end

function should.runMainLoop()
  local x
  local s = Scheduler()
  s:run(function()
    x = 1
  end)
  assertEqual(1, x)
end

function should.restart()
  local x
  local s = Scheduler()
  s:run(function()
    x = 1
  end)

  s:run(function()
    x = 2
  end)
  assertEqual(2, x)
end

should:test()
