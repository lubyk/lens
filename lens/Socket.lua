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

function lib.new(sock_type, func)
  if type(sock_type) == 'function' then
    func = sock_type
    sock_type = lib.TCP
  end
  local self = new(sock_type or lib.TCP)

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

local recvMessage = lib.recvMessage
-- Receive all there is to receive. This method yields if the data is not yet
-- available.
function lib:recvMessage()
  local super  = self.super

  while true do
    local data, eagain = recvMessage(super)
    if eagain then
      -- EAGAIN
      -- no data
    elseif not data then
      -- closed
      error('Connection closed while reading.')
    else
      return data
    end
    yield('read', self.sock_fd)
  end
end

local recvBytes = lib.recvBytes
-- Receive `len` count of bytes. This method yields if the data is not yet
-- available.
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
    cli.thread = lens.Thread(function()
      func(cli)
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
  else
    error('no thread running. Cannot join.')
  end
end

return lib
