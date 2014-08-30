#include <QtGui>
#include <QtDebug>

#import "cocoaappdelegate.h"

// See:
// http://qt.gitorious.org/qt/qt/blobs/4.8/src/gui/kernel/qcocoaapplicationdelegate_mac.mm
//
@implementation CocoaAppDelegate

- (id) init
{
    if (!(self = [super init]))
        return nil;

    _terminationPending = false;
    _mainWindow = NULL;

    return self;
}

- (void) registerForApplicationEvents
{
    // We can't just implement -applicationWillFinishLaunching: because
    // the Qt delegate won't forward those to its "reflection delegate"
    // (i.e. this object). :(
    //
    // We also can't register for this event in -init because NSApplication
    // has to first be ready (i.e. QApplication has to be instantiated).
    //
    [[NSNotificationCenter defaultCenter]
     addObserver:self
     selector:@selector(appDidFinishLaunching:)
     name:NSApplicationDidFinishLaunchingNotification
     object:[NSApplication sharedApplication]];
}

// This is called *after* the "open files" event:
- (void) appDidFinishLaunching:(NSNotification *)notification
{
    qDebug() << "App DidFinishLaunching !";
    _mainWindow->handleApplicationLaunched();
}


- (void) setMainWindow:(MainWindow *)aMainWindow
{
    _mainWindow = aMainWindow;
}


- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
{
    Q_UNUSED(sender);

    qDebug() << "applicationShouldTerminate";

    _terminationPending = true;
    _mainWindow->cocoaCommitDataHandler();
    qDebug() << "applicationShouldTerminate returned from cocoaCommitDataHandler().";

    return NSTerminateLater;
}

- (void) acceptPendingTermination
{
    qDebug() << "acceptPendingTermination. pending =" << _terminationPending;
    if (!_terminationPending)
        return;
    _terminationPending = false;
    [NSApp replyToApplicationShouldTerminate:YES];
}

- (void) cancelPendingTermination
{
    qDebug() << "cancelPendingTermination. pending =" << _terminationPending;
    if (!_terminationPending)
        return;
    _terminationPending = false;
    [NSApp replyToApplicationShouldTerminate:NO];
}


@end
