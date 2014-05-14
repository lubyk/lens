#include "lens/Poller.h"

#import <Cocoa/Cocoa.h>

using namespace lens;

// ============================================== LFileHandle (objC)

@interface LPoller : NSObject {
  Poller * master_;
  NSThread * thread_;
  bool should_run_;
}

- (id)initWithMaster:(Poller*)master;
- (void)start;
- (void)resume;
- (void)run;

@end

@implementation LPoller

- (id)initWithMaster:(Poller*)master {
  master_  = master;
  
	thread_ = [[NSThread alloc] initWithTarget:self selector:@selector(run) object:nil];
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

- (void)dealloc {
  [thread_ cancel];
  [thread_ release];
  [super dealloc];
}

@end

// ============================================== Poller
void Poller::runGUI(double wake_at) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [NSApplication sharedApplication];

  LPoller *lpoller = [[LPoller alloc] initWithMaster:this];

  // initial wake is immediately since we do not know what the previous value
  // was.
  wake_at_ = wake_at;
  [lpoller start];
  [NSApp run];

  [lpoller release];
  [pool release];
}

