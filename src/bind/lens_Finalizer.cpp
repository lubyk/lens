/**
 *
 * MACHINE GENERATED FILE. DO NOT EDIT.
 *
 * Bindings for class Finalizer
 *
 * This file has been generated by dub 2.2.1.
 */
#include "dub/dub.h"
#include "lens/Finalizer.h"

using namespace lens;

/** lens::Finalizer::Finalizer()
 * include/lens/Finalizer.h:45
 */
static int Finalizer_Finalizer(lua_State *L) {
  try {
    Finalizer *retval__ = new Finalizer();
    retval__->dub_pushobject(L, retval__, "lens.Finalizer", true);
    return 1;
  } catch (std::exception &e) {
    lua_pushfstring(L, "new: %s", e.what());
  } catch (...) {
    lua_pushfstring(L, "new: Unknown exception");
  }
  return dub::error(L);
}

/** virtual lens::Finalizer::~Finalizer()
 * include/lens/Finalizer.h:47
 */
static int Finalizer__Finalizer(lua_State *L) {
  try {
    DubUserdata *userdata = ((DubUserdata*)dub::checksdata_d(L, 1, "lens.Finalizer"));
    if (userdata->gc) {
      Finalizer *self = (Finalizer *)userdata->ptr;
      self->finalize();
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



// --=============================================== __tostring
static int Finalizer___tostring(lua_State *L) {
  Finalizer *self = *((Finalizer **)dub::checksdata_n(L, 1, "lens.Finalizer"));
  lua_pushfstring(L, "lens.Finalizer: %p", self);
  
  return 1;
}

// --=============================================== METHODS

static const struct luaL_Reg Finalizer_member_methods[] = {
  { "new"          , Finalizer_Finalizer  },
  { "__gc"         , Finalizer__Finalizer },
  { "__tostring"   , Finalizer___tostring },
  { "deleted"      , dub::isDeleted       },
  { NULL, NULL},
};


extern "C" int luaopen_lens_Finalizer(lua_State *L)
{
  // Create the metatable which will contain all the member methods
  luaL_newmetatable(L, "lens.Finalizer");
  // <mt>

  // register member methods
  dub::fregister(L, Finalizer_member_methods);
  // setup meta-table
  dub::setup(L, "lens.Finalizer");
  // <mt>
  return 1;
}
