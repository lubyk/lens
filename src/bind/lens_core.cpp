/**
 *
 * MACHINE GENERATED FILE. DO NOT EDIT.
 *
 * Bindings for library lens
 *
 * This file has been generated by dub 2.2.1.
 */
#include "dub/dub.h"
#include "lens/File.h"
#include "lens/Finalizer.h"
#include "lens/Poller.h"
#include "lens/Popen.h"
#include "lens/Socket.h"
#include "lens/lens.h"

using namespace lens;

extern "C" {
int luaopen_lens_File(lua_State *L);
int luaopen_lens_Finalizer(lua_State *L);
int luaopen_lens_Poller(lua_State *L);
int luaopen_lens_Popen(lua_State *L);
int luaopen_lens_Socket(lua_State *L);
}

/** void lens::init()
 * include/lens/lens.h:63
 */
static int lens_init(lua_State *L) {
  try {
    lens::init();
    return 0;
  } catch (std::exception &e) {
    lua_pushfstring(L, "lens.init: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "lens.init: Unknown exception");
  }
  return lua_error(L);
}

/** double lens::elapsed()
 * include/lens/lens.h:82
 */
static int lens_elapsed(lua_State *L) {
  try {
    lua_pushnumber(L, lens::elapsed());
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "lens.elapsed: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "lens.elapsed: Unknown exception");
  }
  return lua_error(L);
}

/** double lens::millisleep(double ms)
 * include/lens/lens.h:100
 */
static int lens_millisleep(lua_State *L) {
  try {
    double ms = dub::checknumber(L, 1);
    lua_pushnumber(L, lens::millisleep(ms));
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "lens.millisleep: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "lens.millisleep: Unknown exception");
  }
  return lua_error(L);
}

// --=============================================== FUNCTIONS
static const struct luaL_Reg lens_functions[] = {
  { "init"         , lens_init            },
  { "elapsed"      , lens_elapsed         },
  { "millisleep"   , lens_millisleep      },
  { NULL, NULL},
};


extern "C" int luaopen_lens_core(lua_State *L) {
  lua_newtable(L);
  // <lib>
  dub::fregister(L, lens_functions);
  // <lib>

  luaopen_lens_File(L);
  // <lens.File>
  lua_setfield(L, -2, "File");
  
  luaopen_lens_Finalizer(L);
  // <lens.Finalizer>
  lua_setfield(L, -2, "Finalizer");
  
  luaopen_lens_Poller(L);
  // <lens.Poller>
  lua_setfield(L, -2, "Poller");
  
  luaopen_lens_Popen(L);
  // <lens.Popen>
  lua_setfield(L, -2, "Popen");
  
  luaopen_lens_Socket(L);
  // <lens.Socket>
  lua_setfield(L, -2, "Socket");
  
  // <lib>
  return 1;
}
