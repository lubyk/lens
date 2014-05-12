/**
 *
 * MACHINE GENERATED FILE. DO NOT EDIT.
 *
 * Bindings for class Popen
 *
 * This file has been generated by dub 2.2.0.
 */
#include "dub/dub.h"
#include "lens/Popen.h"

using namespace lens;

/** Cast (class_name)
 * 
 */
static int Popen__cast_(lua_State *L) {

  Popen *self = *((Popen **)dub::checksdata_n(L, 1, "lens.Popen"));
  const char *key = luaL_checkstring(L, 2);
  void **retval__ = (void**)lua_newuserdata(L, sizeof(void*));
  int key_h = dub::hash(key, 2);
  switch(key_h) {
    case 0: {
      if (DUB_ASSERT_KEY(key, "lens.File")) break;
      *retval__ = static_cast<File *>(self);
      return 1;
    }
  }
  return 0;
}

/** lens::Popen::Popen(const char *program, lua_State *L)
 * include/lens/Popen.h:47
 */
static int Popen_Popen(lua_State *L) {
  try {
    const char *program = dub::checkstring(L, 1);
    Popen *retval__ = new Popen(program, L);
    dub::pushudata(L, retval__, "lens.Popen", true);
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "new: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "new: Unknown exception");
  }
  return dub::error(L);
}

/** lens::Popen::~Popen()
 * include/lens/Popen.h:49
 */
static int Popen__Popen(lua_State *L) {
  try {
    DubUserdata *userdata = ((DubUserdata*)dub::checksdata_d(L, 1, "lens.Popen"));
    if (userdata->gc) {
      Popen *self = (Popen *)userdata->ptr;
      delete self;
    }
    userdata->gc = false;
    return 0;
  } catch (std::exception &e) {
    lua_pushfstring(L, "__gc: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "__gc: Unknown exception");
  }
  return dub::error(L);
}

/** int lens::Popen::pid()
 * include/lens/Popen.h:51
 */
static int Popen_pid(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    lua_pushnumber(L, self->pid());
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "pid: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "pid: Unknown exception");
  }
  return dub::error(L);
}

/** int lens::Popen::waitpid()
 * include/lens/Popen.h:56
 */
static int Popen_waitpid(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    lua_pushnumber(L, self->waitpid());
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "waitpid: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "waitpid: Unknown exception");
  }
  return dub::error(L);
}

/** int lens::File::fd()
 * include/lens/File.h:93
 */
static int Popen_fd(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    lua_pushnumber(L, self->fd());
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "fd: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "fd: Unknown exception");
  }
  return dub::error(L);
}

/** void lens::File::close()
 * include/lens/File.h:97
 */
static int Popen_close(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    self->close();
    return 0;
  } catch (std::exception &e) {
    lua_pushfstring(L, "close: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "close: Unknown exception");
  }
  return dub::error(L);
}

/** LuaStackSize lens::File::read(size_t sz, lua_State *L)
 * include/lens/File.h:106
 */
static int Popen_read(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    size_t sz = dub::checkint(L, 2);
    return self->read(sz, L);
  } catch (std::exception &e) {
    lua_pushfstring(L, "read: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "read: Unknown exception");
  }
  return dub::error(L);
}

/** LuaStackSize lens::File::readLine(lua_State *L)
 * include/lens/File.h:109
 */
static int Popen_readLine(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    return self->readLine(L);
  } catch (std::exception &e) {
    lua_pushfstring(L, "readLine: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "readLine: Unknown exception");
  }
  return dub::error(L);
}

/** LuaStackSize lens::File::readAll(lua_State *L)
 * include/lens/File.h:112
 */
static int Popen_readAll(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    return self->readAll(L);
  } catch (std::exception &e) {
    lua_pushfstring(L, "readAll: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "readAll: Unknown exception");
  }
  return dub::error(L);
}

/** LuaStackSize lens::File::write(lua_State *L)
 * include/lens/File.h:115
 */
static int Popen_write(lua_State *L) {
  try {
    Popen *self = *((Popen **)dub::checksdata(L, 1, "lens.Popen"));
    return self->write(L);
  } catch (std::exception &e) {
    lua_pushfstring(L, "write: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "write: Unknown exception");
  }
  return dub::error(L);
}



// --=============================================== __tostring
static int Popen___tostring(lua_State *L) {
  Popen *self = *((Popen **)dub::checksdata_n(L, 1, "lens.Popen"));
  lua_pushfstring(L, "lens.Popen: %p (%d)", self, self-> fd());
  
  return 1;
}

// --=============================================== METHODS

static const struct luaL_Reg Popen_member_methods[] = {
  { "_cast_"       , Popen__cast_         },
  { "new"          , Popen_Popen          },
  { "__gc"         , Popen__Popen         },
  { "pid"          , Popen_pid            },
  { "waitpid"      , Popen_waitpid        },
  { "fd"           , Popen_fd             },
  { "close"        , Popen_close          },
  { "read"         , Popen_read           },
  { "readLine"     , Popen_readLine       },
  { "readAll"      , Popen_readAll        },
  { "write"        , Popen_write          },
  { "__tostring"   , Popen___tostring     },
  { "deleted"      , dub::isDeleted       },
  { NULL, NULL},
};


extern "C" int luaopen_lens_Popen(lua_State *L)
{
  // Create the metatable which will contain all the member methods
  luaL_newmetatable(L, "lens.Popen");
  // <mt>

  // register member methods
  dub::fregister(L, Popen_member_methods);
  // setup meta-table
  dub::setup(L, "lens.Popen");
  // <mt>
  return 1;
}