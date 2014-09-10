#include "lens/Poller.h"

using namespace lens;

// ============================================== Poller
void Poller::runGUI(double wake_at, lua_State *L) {
  if (gui_running_) return;
  throw dub::Exception("Poller::runGUI is not implemented on linux yet...");
}
