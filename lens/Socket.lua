--[[------------------------------------------------------

  lens.Socket
  ---------

  BSD Socket that uses msgpack to send Lua values.

--]]------------------------------------------------------
local lens = require 'lens'
local core = require 'lens.core'
local lib  = core.Socket
local new  = lib.new

local           yield,       slen,       ssub = 
      coroutine.yield, string.len, string.sub

-- Rewrite new
function lib.new(func)
  local self = new(lib.TCP)
  self.clients = {}
  if func then
    self.thread = lens.Thread(function()
      func(self)
    end)
  end
  return self
end

local bind = lib.bind
function lib:bind(host, port)
  self.host    = host
  self.port    = bind(self.super, host, port)
  self.sock_fd = self:fd()
  return self.port
end

local connect = lib.connect
function lib:connect(...)
  local super = self.super
  local ok = connect(super, ...)
  self.sock_fd = super:fd()
  if not ok then
    yield('write', self.sock_fd)
    super:connectFinish()
  end
end

local recvBytes = lib.recvBytes
function lib:recvBytes(len)
  local super  = self.super
  local buffer

  while true do
    local data, eagain, sz = recvBytes(super, len)
    if not data then
      -- closed
      error('Connection closed while reading.')
    elseif eagain then
      -- eagain contains the remaining size
      len = eagain
      if buffer then
        buffer = buffer .. data
      else
        buffer = data
      end
    else
      if buffer then
        return buffer .. data
      else
        return data
      end
    end
    yield('read', self.sock_fd)
  end
end

local recvLine = lib.recvLine
function lib:recvLine()
  local super  = self.super
  local buffer

  while true do
    local data, eagain = recvLine(super)
    if not data then
      -- closed
      error('Connection closed while reading.')
    elseif eagain then
      -- EAGAIN
      if buffer then
        buffer = buffer .. data
      else
        buffer = data
      end
    else
      if buffer then
        return buffer .. data
      else
        return data
      end
    end
    yield('read', self.sock_fd)
  end
end

local send = lib.send
function lib:send(data)
  local buffer = data
  while true do
    local sent = send(self.super, data)
    if sent == slen(data) then
      break
    else
      data = ssub(data, sent + 1)
    end
    yield('write', self.sock_fd)
  end
end

local accept = lib.accept
function lib:accept(func)
  local cli = accept(self.super)
  while not cli do
    yield('read', self.sock_fd)
    cli = accept(self.super)
  end

  cli.sock_fd = cli:fd()
  if func then
    -- start new thread
    -- protect from GC
    self.clients[cli.sock_fd] = cli
    cli.thread = lens.Thread(function()
      func(cli)
      -- forget about the client
      self.clients[cli.sock_fd] = nil
    end)
  end
  return cli
end

function lib:kill()
  if self.thread then
    self.thread:kill()
  end
end

function lib:join()
  if self.thread then
    self.thread:join()
  end
end

return lib
