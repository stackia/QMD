#ifndef QMDAPPLICATION_H
#define QMDAPPLICATION_H

#include <QtWidgets/QApplication>
#include "mainwindow.h"

class QMDApplication : public QApplication
{
    Q_OBJECT
public:
    explicit QMDApplication(int &argc, char **argv);
    ~QMDApplication();

    MainWindow *mainWindow;

    QString copyrightYear();
    QString websiteURL();
    QString applicationStoragePath();
    bool copyResourceToFile(QString resourcePath, QString targetFilePath);

protected:
    bool event(QEvent *event);

signals:

public slots:

};

#endif // QMDAPPLICATION_H
