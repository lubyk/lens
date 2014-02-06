--[[------------------------------------------------------

  # lens test

--]]------------------------------------------------------
local lub    = require 'lub'
local lut    = require 'lut'

local lens   = require 'lens'
local core   = require 'lens.core'
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
  local t, op, l, line
  s:run(function()
    local p = lens.Popen('LC_ALL=C date -r 1391723000')
    -- The code below is approximately what File.readLine does. Please do not
    -- use __readLine. The direct binding is just there for testing.
    local fd = p:fd()
    line, op = lens.Popen.__readLine(p) -- __readLine used just for this test.
    while op == core.File.Wait do
      lens.waitRead(fd)
      l, op = core.Popen.__readLine(p)
      line = line .. l
    end
    if op == core.File.OK then
      t = line
    else
      t = nil -- EOF
    end
  end)
  assertMatch('Thu Feb  6 .* 2014', t)
end

function should.waitWrite()
  local s = lens.Scheduler()
  local path = lub.path '|tmp_waitWrite.txt'
  local str = 'Echo cowboy'
  s:run(function()
    local p = lens.Popen('cat > '..path, 'w')
    local wsz, op = lens.Popen.__write(p, str) -- __write used just for this test
    while op == Wait do
      str = string.sub(str, wsz+1)
      lens.waitWrite(p:fd())
      wsz, op = lens.File.__write(p, str)
    end
    p:waitpid() -- wait for child process to finish
  end)
  local t = lub.content(path)
  lub.rmFile(path)
  assertEqual(str, t)
end

-- It is hard to come with a test for waitWrite...
should.ignore.waitWrite = true

should:test()
