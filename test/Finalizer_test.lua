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
  assertType('function', Finalizer)
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

should:test()
