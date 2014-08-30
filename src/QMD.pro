QT += network webkit widgets # webkitwidgets

mac {
    OSX_FILES.files = gfx/icon.icns gfx/document.icns
    OSX_FILES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += OSX_FILES
    QMAKE_INFO_PLIST = cfg/Info.plist
    RESOURCES += compilers-osx.qrc
    CONFIG += cocoa
    LIBS += -framework Cocoa
    QMAKE_POST_LINK += ../fixmakefile.sh
}

mac:cocoa {
    OBJECTIVE_SOURCES += \
        mac/main.mm \
        mac/mainwindow.mm \
        mac/cocoaappdelegate.mm
    HEADERS += \
        mac/cocoaappdelegate.h
}
#} else {
#    SOURCES += \
#        main.cpp \
#        mainwindow.cpp
#}

RESOURCES += \
    styles.qrc \
    misc.qrc

CONFIG(release, debug|release) {
    DEFINES += BUILD_RELEASE
}
CONFIG(debug, debug|release) {
    DEFINES += BUILD_DEBUG
}


HEADERS += \
    defines.h\
    QMDapplication.h \
    mainwindow.h \
    preferencesdialog.h \
    peg-markdown-highlight/pmh_parser.h \
    peg-markdown-highlight/pmh_definitions.h \
    peg-markdown-highlight/highlighter.h \
    editor/QMDtextedit.h \
    editor/linenumberingplaintextedit.h \
    peg-markdown-highlight/pmh_styleparser.h \
    markdowncompiler.h \
    logger.h \
#    updatecheck/updatecheck.h \
#    updatecheck/hgupdateinfodialog.h \
    filesearchdialog.h
SOURCES += \
    QMDapplication.cpp \
    main.cpp \
    mainwindow.cpp \
    preferencesdialog.cpp \
    peg-markdown-highlight/pmh_parser.c\
    peg-markdown-highlight/highlighter.cpp \
    editor/QMDtextedit.cpp \
    editor/linenumberingplaintextedit.cpp \
    peg-markdown-highlight/pmh_styleparser.c \
    markdowncompiler.cpp \
    logger.cpp \
#    updatecheck/updatecheck.cpp \
#    updatecheck/hgupdateinfodialog.cpp \
    filesearchdialog.cpp

FORMS += \
    preferencesdialog.ui \
#    updatecheck/hgupdateinfodialog.ui \
    filesearchdialog.ui

OTHER_FILES += \
    cfg/Info.plist \
    template.html












