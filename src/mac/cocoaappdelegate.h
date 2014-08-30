#import <Cocoa/Cocoa.h>

#include "../mainwindow.h"

@interface CocoaAppDelegate : NSObject<NSApplicationDelegate>
{
    bool _terminationPending;
    MainWindow *_mainWindow;
}

- (void) registerForApplicationEvents;
- (void) setMainWindow:(MainWindow *)aMainWindow;

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender;
- (void) acceptPendingTermination;
- (void) cancelPendingTermination;

@end
