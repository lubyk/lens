#include "lens/Poller.h"

#import <Cocoa/Cocoa.h>

using namespace lens;

// ============================================== LFileHandle (objC)

@interface LPoller : NSObject {
  Poller * master_;
  NSThread * thread_;
  bool should_run_;
  lua_State *lua_;
}

- (id)initWithMaster:(Poller*)master lua:(lua_State*)L;
- (void)start;
- (void)resume;
- (void)run;
- (void)applicationWillTerminate:(NSNotification *)aNotification;

@end

@implementation LPoller

- (id)initWithMaster:(Poller*)master lua:(lua_State*)L {
  master_  = master;
  lua_     = L;
  
	thread_ = [[NSThread alloc] initWithTarget:self selector:@selector(run) object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillTerminate:)
                                                 name:NSApplicationWillTerminateNotification
                                               object:nil];
  return self;
}

- (void)start {
  [thread_ start];
}

// This is posted on main thread when poller:poll() returns.
- (void)resume {
  should_run_ = master_->resume();
}

- (void)run {
  should_run_ = true;
  while(should_run_ && master_->backPoll()) {
    // we must waitUntilDone to make sure the poll() call is only executed after
    // lua threads have finished running.
    [self performSelectorOnMainThread:@selector(resume) withObject:nil waitUntilDone:YES];
  }
  // if this thread ends, we quit application
  [NSApp terminate:nil];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
  // Close lua state
  lua_close(lua_);
}

- (void)dealloc {
  [thread_ cancel];
  [thread_ release];
  [super dealloc];
}

@end

// ============================================== Poller
void Poller::runGUI(double wake_at, lua_State *L) {
  if (gui_running_) return;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [NSApplication sharedApplication];

  LPoller *lpoller = [[LPoller alloc] initWithMaster:this lua:L];
  impl_ptr_ = lpoller;
  

  // initial wake is immediately since we do not know what the previous value
  // was.
  wake_at_ = wake_at;
  [lpoller start];
  gui_running_ = true;
  [NSApp run];

  // NEVER REACHED

  [lpoller release];
  [pool release];
}

void Poller::interrupted() {
  interrupted_ = true;
  if (gui_running_) {
    LPoller *lpoller = (LPoller *)impl_ptr_;
    retval_ = false; // inform about interruption
    [lpoller performSelectorOnMainThread:@selector(resume) withObject:nil waitUntilDone:YES];
    [NSApp terminate:nil];
  }
}
