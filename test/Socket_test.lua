--[[------------------------------------------------------

  lens.Socket test
  --------------


--]]------------------------------------------------------
local lens   = require 'lens'
local lut    = require 'lut'
local should = lut.Test 'lens.Socket'

local Scheduler,      Socket,      sleep,      elapsed =
      lens.Scheduler, lens.Socket, lens.sleep, lens.elapsed

should.ignore.deleted = true

local function run(func)
  local sc = Scheduler()
  sc.willTerminate = function() end
  sc:run(func)
end  

function should.tostring()
  local s = Socket()
  assertMatch('lens.Socket:', tostring(s))
end

function should.getLocalInfo()
  local s = Socket()
  s:bind('*', 9938)
  assertEqual('*',  s:localHost())
  assertEqual(9938, s:localPort())
end

function should.kill()
  run(function()
    local s = Socket(function()
      while true do
        sleep(0.1)
      end
    end)
    assertPass(function()
      s:kill()
    end)
  end)
end

function should.join()
  local t
  run(function()
    local s = Socket(function()
      t = true
    end)
    assertFalse(t)
    s:join()
    assertTrue(t)
  end)
end

function should.getRemoteInfo()
  run(function()
    local s = Socket()
    s:connect('google.com', 80)
    assertEqual('google.com', s:remoteHost())
    assertEqual(80, s:remotePort())
  end)
end

function should.runMultithreaded(t)
  run(function()
    t.sequence = {}
    function log(msg)
      table.insert(t.sequence, msg)
    end
    t.server = Socket(function()
      -- will start as soon as we join
      local client = t.server:accept()
      log('srv:accept')
      t.received = client:recvLine()
      log('srvi:recv')
      client:close()
      log('srvi:close')
    end)
    t.server:bind('*', 0)
    t.server:listen()
    t.port = t.server.port

    t.client = Socket()
    t.client:connect('127.0.0.1', t.port)
    log('cli:connect')
    -- 'send' does not yield
    t.client:send('Hello Lubyk!\n')
    log('cli:send')
    -- The server thread finally has some time to run
    t.server:join()
    -- should give us control back on 'accept'
    assertValueEqual('Hello Lubyk!', t.received)
    -- sometimes 'cli:connect' returns EAGAIN and then 'srv:accept' runs first
    if t.sequence[1] == 'srv:accept' then
      assertValueEqual({
        'srv:accept',
        'cli:connect',
        'cli:send',
        'srvi:recv',
        'srvi:close',
      }, t.sequence)
    else
      assertValueEqual({
        'cli:connect',
        'cli:send',
        'srv:accept',
        'srvi:recv',
        'srvi:close',
      }, t.sequence)
    end
  end)
end

function should.send(t)
  run(function()
    t.server = Socket(function()
      -- print('run2')
      -- run server in new thread
      t.server:bind('*', 0)
      t.server:listen()
      t.port = t.server.port
      -- will start as soon as we yield
      t.srv_client = t.server:accept()
      t.received1 = t.srv_client:recvLine()
      t.received2 = t.srv_client:recvLine()
      t.srv_client:close()
    end)

    -- print('run1')

    t.client = Socket(function()
      t.client:connect('127.0.0.1', t.port)
      t.client:send('Hello Lubyk!\nOne two\n')
    end)

    -- print('run1.2')

    -- The server thread finally has some time to run
    t.server:join()
    t.client:close()
    -- should give us control back on 'accept'
    assertEqual('Hello Lubyk!', t.received1)
    assertEqual('One two', t.received2)
  end)
end

function should.recvBytes(t)
  run(function()
    t.server = Socket()
    t.server:bind('*', 0)
    t.server:listen()
    t.port = t.server.port
    -- run server in new thread
    t.thread = lens.Thread(function()
      -- will start as soon as we yield
      local client = t.server:accept()
      t.received1 = client:recvBytes(6)
      t.received2 = client:recvBytes(4)
      client:close()
    end)
    t.client = Socket()
    t.client:connect('127.0.0.1', t.port)
    -- 'send' does not yield
    t.client:send('0123456789')
    -- The server thread finally has some time to run
    t.thread:join()
    -- should give us control back on 'accept'
    assertValueEqual('012345', t.received1)
    assertValueEqual('6789', t.received2)
  end)
end


should:test()
