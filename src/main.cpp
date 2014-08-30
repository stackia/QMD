#include <QtWidgets/QApplication>

#include "mainwindow.h"
#include "QMDapplication.h"
#include "logger.h"
#include <QtCore/QDebug>

#ifdef QT_MAC_USE_COCOA
#import "mac/cocoaappdelegate.h"
#endif

int main(int argc, char *argv[])
{
#ifdef QT_MAC_USE_COCOA
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    CocoaAppDelegate *cocoaAppDelegate = [[CocoaAppDelegate alloc] init];
    [[NSApplication sharedApplication] setDelegate:cocoaAppDelegate];
    [pool release];
#endif

    QMDApplication app(argc, argv);

#ifdef QT_MAC_USE_COCOA
    [cocoaAppDelegate registerForApplicationEvents];
#endif

    for (int i = 0; i < argc; i++)
    {
        if (strcmp("-d", argv[i]) == 0) {
            Logger::setAllLevelsEnabled(true);
            break;
        }
    }

    MainWindow window;

#ifdef QT_MAC_USE_COCOA
    [cocoaAppDelegate setMainWindow:&window];
#endif

    window.show();
    app.mainWindow = &window;
    if (argc > 1)
        window.openFile(argv[1]);

#ifndef QT_MAC_USE_COCOA
    window.handleApplicationLaunched();
#endif

    int ret = app.exec();
    return ret;
}
