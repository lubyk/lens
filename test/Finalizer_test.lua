--[[------------------------------------------------------

  lens.Finalizer test
  -----------------

  ...

--]]------------------------------------------------------
local lens   = require 'lens'
local lut    = require 'lut'
local should = lut.Test 'lens.Finalizer'

local Finalizer = lens.Finalizer

function should.autoload()
  assertType('table', Finalizer)
end

function should.setType()
  local f = Finalizer(function() end)
  assertEqual('lens.Finalizer', f.type)
end

function should.tostring()
  local f = Finalizer(function() end)
  assertMatch('lens.Finalizer: 0x.*', tostring(f))
end

function should.setFinalize()
  local t
  local f = Finalizer(function() 
    t = true
  end)
  f:finalize()
  assertTrue(t)
end

function should.triggerOnGc()
  local continue = false
  local fin = Finalizer(function()
    continue = true
  end)
  -- We make sure that we can set different finalizers at the same time.
  local fin2 = Finalizer(function()
    continue2 = true
  end)
  assertFalse(continue)
  assertFalse(continue2)
  collectgarbage('collect')
  assertFalse(continue)
  assertFalse(continue2)
  fin = nil
  collectgarbage('collect')
  assertTrue(continue)
  assertFalse(continue2)
  fin2 = nil
  collectgarbage('collect')
  assertTrue(continue)
  assertTrue(continue2)
end

should.ignore.deleted  = true
should.ignore.finalize = true

should:test()
