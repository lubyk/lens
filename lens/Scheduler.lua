--[[------------------------------------------------------

  # Scheduler

--]]------------------------------------------------------
local lub     = require 'lub'
local lens    = require 'lens'
local lib     = lub.class 'lens.Scheduler'

local setmetatable, create,           resume,           status           =
      setmetatable, coroutine.create, coroutine.resume, coroutine.status
      
local format,        insert,       elapsed      =
      string.format, table.insert, lens.elapsed
      
local POLLIN,           POLLOUT,           VNODE =
      lens.Poller.Read, lens.Poller.Write, lens.Poller.VNode

local operations = {}
local scheduleAt, removeFd, runThread

-- Create a new Scheduler object.
function lib.new()
  local self = {
    -- Points to the next thread to run
    at_next  = nil,
    -- Counts number of filedescriptors watching
    fd_count = 0,
    -- Translates Poller ids to threads.
    idx_to_thread = {},
    -- Default pollser
    poller = lens.Poller(),
  }
  return setmetatable(self, lib)
end

-- Start running scheduler with a main function.
function lib:run(main)
  scheduleAt(self, nil, lens.Thread.make(main))
  -- self.at_next.name = 'main'
  self.should_run = true
  self:loop()
end

function lib:loop()
  local thread
  local idx_to_thread = self.idx_to_thread
  while self.should_run do
    -- Get next thread to run
    local thread = self.at_next
    local now    = elapsed()

    if not thread or thread.at > now then
      -- No events
    else
      -- Extract current elements from list so that newly added threads do not
      -- alter list (and we give fair chances for threads to run).

      -- Last element to be run now
      local last = thread
      -- Find element past now frame
      local past = thread.at_next
      while past and past.at <= now do
        last = past
        past = last.at_next
      end

      -- New head (can be nil)
      self.at_next = past
      
      if past then
        -- Cut link
        last.at_next = nil
      end

      -- Run scheduled threads in 'thread' linked list.
      while thread and self.should_run do
        -- Run thread
        -- Need to get next thread before in case the thread being run
        -- is rescheduled (and breaks at_next link).
        local ne = thread.at_next
        if runThread(self, thread) then
          -- run same thread again
        else

          thread = ne
        end
      end

      if not self.should_run then
        break
      end

    end

    -- Get timeout value
    local wake_at = -1

    thread = self.at_next
    if thread then
      wake_at = thread.at
    else
      print('no at next', wake_at)
    end

    if self.fd_count == 0 and wake_at == -1 then
      -- No more at events and no more fd
      self.should_run = false
      break
    end

    -- Poll
    if not self.poller:poll(wake_at) then
      -- interrupted
      self.should_run = false
      print('interrupted')
      break
    end

    -- First collect events so that running the threads (and
    -- possibly adding new file descriptors) does not alter
    -- the list.
    local events = self.poller:events()
    if events then
      -- Execute poll events.
      local i = 1
      local ev_idx = events[i]
      local thread = ev_idx and idx_to_thread[ev_idx]
      while thread and self.should_run do
        if thread.filter == VNODE then
          thread.retval = self.poller:fflags(ev_idx)
        end
        if runThread(self, thread) then
          -- run fd thread again
        else
          -- run next fd thread
          i = i + 1
          ev_idx = events[i]
          if ev_idx then
            thread = idx_to_thread[ev_idx]
            if not thread then
              error(string.format("Unknown thread idx '%i' in poller", ev_idx))
            end
          else
            thread = nil
          end
        end
      end
    end
  end -- while self.should_run
end

------------------------------------------------------ PRIVATE

function runThread(self, thread)
  local ok, a, b, c = resume(thread.co, thread.retval)
  if ok then
    if a then
      local func = operations[a]
      if func then
        if func(self, thread, b, c) then
          -- resume running thread immediately
          return true
        end
      else
        error(format("Invalid operation '%s'.", tostring(a)))
      end
    elseif status(thread.co) == 'dead' then
      -- Coroutine function finished
      -- Cleanup
      if thread.fd then
        removeFd(self, thread)
      end
      thread.co = nil
    end
  else
    -- Error
    if thread.error then
      -- Thread has an error handler, call it
      thread.error(a, debug.traceback(thread.co))
    else
      print('Error', a, thread.co, debug.traceback(thread.co))
      if thread.fd then
        removeFd(self, thread)
      end
      thread.co = nil
      -- error(a)
    end
  end
end  

-- Add a thread and schedule at thread.at
function scheduleAt(self, _, thread)
  local at = thread.at

  local prev = self
  local ne   = prev.at_next
  while true do
    if not ne or at < ne.at then
      prev.at_next   = thread
      thread.at_next = ne
      break
    else
      prev = ne
    end
    ne = prev.at_next
  end
  if self.at_next.at ~= pat then
    -- FIXME: give some time to GUI ?
    -- self.poller:resumeAt(self.at_next.at)
  end

  -- Return true to resume running thread immediately
  return true
end


function removeFd(self, thread)
  local fd = thread.fd
  if thread.idx then
    self.idx_to_thread[thread.idx] = nil
  end
  if fd then
    self.poller:remove(thread.idx)
    thread.fd  = nil
    self.fd_count = self.fd_count - 1
  end
  thread.idx = nil
end

function changeFdFilter(self, thread, fd, filter, flags)
  flags = flags or 0
  if thread.fd then
    if thread.fd == fd then
      -- same
      self.poller:modify(thread.idx, filter, fd, flags)
    else
      -- changed fd
      assert(fd, 'Missing fd value. Check waitRead calls.')
      self.poller:modify(thread.idx, filter, fd, flags)
    end
  else
    -- add fd
    thread.fd = fd
    self.fd_count = self.fd_count + 1

    thread.idx = self.poller:add(fd, filter, flags)

    self.idx_to_thread[thread.idx] = thread
  end

  -- We need this information in case we change poller and to retrieve filter
  -- flags.
  thread.filter = filter
end

------------------------------------------------------ OPERATIONS

operations.create = scheduleAt

function operations.read(self, thread, fd)
  changeFdFilter(self, thread, fd, POLLIN)
end

function operations.write(self, thread, fd)
  changeFdFilter(self, thread, fd, POLLOUT)
end

function operations.vnode(self, thread, fd, flags)
  flags = flags or 0
  changeFdFilter(self, thread, fd, VNODE, flags)
end

function operations.sleep(self, thread, duration)
  thread.at = elapsed() + duration
  if thread.fd then
    removeFd(self, thread)
  end
  scheduleAt(self, nil, thread)
end

function operations.wait(self, thread, duration)
  thread.at = thread.at + duration
  if thread.fd then
    removeFd(self, thread)
  end
  scheduleAt(self, nil, thread)
end

function operations.kill(self, thread, other)
  if other.fd then
    removeFd(self, other)
  end
  local p = self
  local t = self.at_next
  -- Remove from at list
  while t do
    if t == other then
      p.at_next = t.at_next
      break
    end
    p = t
    t = t.at_next
  end
  -- continue after kill
  scheduleAt(self, nil, thread)
end

function operations.poller(self, thread, new_poller)
  assert(false, 'Poller replacement not yet implemented')
end


return lib
