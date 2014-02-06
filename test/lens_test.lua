--[[------------------------------------------------------

  # lens test

--]]------------------------------------------------------
local lens   = require 'lens'
local lut    = require 'lut'
local should = lut.Test 'lens'

function should.haveType()
  assertEqual('lens', lens.type)
end

function should.returnElapsedTime()
  local elapsed = lens.elapsed()
  assertType('number', elapsed)
  assertTrue(elapsed > 0)
end

function should.millisleep()
  local unslept = lens.millisleep(1)
  assertEqual(0, unslept)
end

function should.sleep()
  local s = lens.Scheduler()
  local t
  s:run(function()
    lens.sleep(0.01)
    t = true
  end)
  assertTrue(t)
end

function should.waitRead()
  local s = lens.Scheduler()
  local t
  s:run(function()
    local p = lens.Popen('LC_ALL=C date -r 182674800')
    t = p:readLine()
  end)
  assertMatch('Thu Oct 16 .* 1975', t)
end

function should.waitWrite()
  local s = lens.Scheduler()
  local path = lub.path '|tmp_waitWrite.txt'
  s:run(function()
    local p = lens.Popen('cat > '..path, 'w')
    p:write('Echo echo')
  end)
  local t = lub.content(path)
  lub.rmFile(path)
  assertEqual('Echo echo', t)
end

should:test()
