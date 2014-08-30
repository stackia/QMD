#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtWidgets/QProgressDialog>
#include <QtNetwork/QtNetwork>
#include "hgupdateinfodialog.h"

class HGUpdateCheck : public QObject
{
    Q_OBJECT
public:
    HGUpdateCheck(QString baseURL, QWidget *parentWidget);

    void handleAppStartup();

    static void setUpdateCheckSettings(QSettings *settings);
    static bool shouldCheckForUpdatesOnStartup();
    static void setShouldCheckForUpdatesOnStartup(bool value);

    void checkForUpdatesInBackgroundIfNecessary();
    void checkForUpdatesNow(bool userInitiated = true);

private:
    static QSettings *_settings;
    QProgressDialog *_progressDialog;
    QString _baseURL;
    QNetworkAccessManager *_nam;
    QString _latestVersion;
    QNetworkReply *_activeReply;
    bool _canceled;

    HGUpdateInfoDialog *_updateInfoDialog;

    void handleLatestVersionInfo(QString latestVersion, bool userInitiated);
    void handleWhatsChanged(QString whatsChangedHTML);

private slots:
    void replyFinished(QNetworkReply*);
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

    void canceledFromProgressDialog();
    void handleRemindMeLater();
    void handleSkipThisVersion();
    void handleUpdateAccepted();
    void handleError(QString errorMessage, bool userInitiated);
};

#endif
