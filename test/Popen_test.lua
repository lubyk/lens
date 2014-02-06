--[[------------------------------------------------------

  # lens.Popen test

--]]------------------------------------------------------
local lub    = require 'lub'
local lut    = require 'lut'

local lens   = require 'lens'
local core   = require 'lens.core'
local should = lut.Test 'lens.Popen'

function should.haveType()
  local p = lens.Popen('date')
  assertEqual('lens.Popen', p.type)
end

function should.tostring()
  local p = lens.Popen('date')
  assertMatch('lens.Popen: 0x.*', tostring(p))
end

function should.cast()
  -- cast could be used outside of dub if we set metatable on userdata.
  local p = lens.Popen('date')
  local f = p:_cast_('lens.File')
  assertType('userdata', f)
end

function should.close()
  -- cast could be used outside of dub if we set metatable on userdata.
  local p = lens.Popen('date')
  local f = p:close()
  assertError('Cannot read from a closed file', function()
    p:readLine()
  end)
end

function should.pid()
  -- cast could be used outside of dub if we set metatable on userdata.
  local p = lens.Popen('date')
  assertType('number', p:pid())
end

function should.readLine()
  local s = lens.Scheduler()
  local t, t2
  s:run(function()
    local p = lens.Popen('LC_ALL=C date -r 1391723000')
    t = p:readLine()
    t2 = p:readLine()
  end)
  assertMatch('Thu Feb  6 .* 2014', t)
  assertEqual(nil, t2)
end

function should.waitWrite()
  local s = lens.Scheduler()
  local path = lub.path '|tmp_waitWrite.txt'
  local str = {'Hello cowboy', ', how is life ?\n', 'OK?'}
  s:run(function()
    local p = lens.Popen('cat > '..path, 'w')
    for _, s in ipairs(str) do
      p:write(s)
    end
    p:waitpid() -- wait for child process to finish
  end)
  local t = lub.content(path)
  lub.rmFile(path)
  assertEqual("Hello cowboy, how is life ?\nOK?", t)
end

should.ignore.__readLine = true
should.ignore.__write    = true
should.ignore.deleted    = true

should:test()
