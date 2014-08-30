#include "QMDapplication.h"
#include "logger.h"
#include <QtGui/QFileOpenEvent>
#include <QtGui/QDesktopServices>
#include <QTranslator>


struct applicationVersion
{
    int major;
    int minor;
    int tiny;
} appVersion = {0, 1, 0};

#define kCopyrightYearStr "2014"
#define kWebsiteURL "http://jiasaiqi.com"

QMDApplication::QMDApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
    setQuitOnLastWindowClosed(true);
    mainWindow = NULL;
    QCoreApplication::setApplicationName("QMD");
    QCoreApplication::setApplicationVersion(QString().sprintf("%i.%i.%i", appVersion.major, appVersion.minor, appVersion.tiny));
}

QMDApplication::~QMDApplication()
{
}

QString QMDApplication::copyrightYear()
{
    return kCopyrightYearStr;
}

QString QMDApplication::websiteURL()
{
    return kWebsiteURL;
}

QString QMDApplication::applicationStoragePath()
{
    QString appName = QCoreApplication::applicationName();
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    // DataLocation should be defined on all but embedded platforms but just
    // to be safe we do this:
    if (path.isEmpty())
        path = QDir::homePath() + "/." + appName;

    // Make sure the path exists
    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }

    return path;
}

bool QMDApplication::copyResourceToFile(QString resourcePath, QString targetFilePath)
{
    QFile source(resourcePath);

    if (QFile::exists(targetFilePath))
    {
        if (!QFile::remove(targetFilePath))
        {
            Logger::warning("Cannot copy resource "+resourcePath
                            +" to file: "+targetFilePath
                            +" -- cannot delete existing file at target path");
            return false;
        }
    }

    if (!source.copy(targetFilePath))
    {
        Logger::warning("Cannot copy resource "+resourcePath
                        +" to file: "+targetFilePath);
        return false;
    }
    return true;
}

bool QMDApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen && mainWindow) {
        mainWindow->openFile(static_cast<QFileOpenEvent *>(event)->file());
        return true;
    }

    return QApplication::event(event);
}
