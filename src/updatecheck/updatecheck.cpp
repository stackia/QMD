#include "updatecheck.h"
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>

HGUpdateCheck::HGUpdateCheck(QString baseURL, QWidget *parentWidget)
    : QObject(parentWidget)
{
    _baseURL = baseURL;
    _activeReply = NULL;
    _canceled = false;

    // Setting min and max to 0 makes the progress bar indeterminate:
    _progressDialog = new QProgressDialog("Checking for Updates...", "Cancel",
                                          0,0, parentWidget);
    _progressDialog->setWindowModality(Qt::WindowModal);
    connect(_progressDialog, SIGNAL(canceled()),
            this, SLOT(canceledFromProgressDialog()));

    _updateInfoDialog = new HGUpdateInfoDialog(parentWidget);
    connect(_updateInfoDialog, SIGNAL(skipThisVersion()),
            this, SLOT(handleSkipThisVersion()));
    connect(_updateInfoDialog, SIGNAL(remindLater()),
            this, SLOT(handleRemindMeLater()));
    connect(_updateInfoDialog, SIGNAL(updateAccepted()),
            this, SLOT(handleUpdateAccepted()));

    _nam = new QNetworkAccessManager(this);
    connect(_nam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    connect(_nam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            this, SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(_nam, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(_nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
}

#define kSettingKeyLastUpdateCheckTime "LastUpdateCheckTime"
#define kSettingKeyLastSkippedVersion "LastSkippedVersion"
#define kSettingKeyShouldCheckForUpdatesAtStartup "ShouldCheckForUpdatesAtStartup"
#define kSettingKeyFirstStartupCompleted "FirstStartupCompleted"

#define kRequestKindAttribute QNetworkRequest::User
#define kRequestKind_UpdateCheck "UpdateCheck"
#define kRequestKind_WhatsChanged "WhatsChanged"

#define kRequestUserInitiatedAttribute (QNetworkRequest::Attribute)(QNetworkRequest::User+1)


QSettings* HGUpdateCheck::_settings = NULL;

void HGUpdateCheck::setUpdateCheckSettings(QSettings *settings)
{
    _settings = settings;
}

void HGUpdateCheck::handleAppStartup()
{
    bool firstStartupCompleted = _settings->value(kSettingKeyFirstStartupCompleted,
                                                  QVariant(false)).toBool();
    if (!firstStartupCompleted)
    {
        _settings->setValue(kSettingKeyFirstStartupCompleted, true);
        _settings->sync();
        return;
    }

    if (!_settings->contains(kSettingKeyShouldCheckForUpdatesAtStartup))
    {
        QMessageBox shouldCheckDialog;
        shouldCheckDialog.setIconPixmap(QPixmap(":/smallAppIcon.png"));
        shouldCheckDialog.setText(tr("Check for Updates on Startup?"));
        shouldCheckDialog.setInformativeText(
                    tr("Would you like %1 to check for updates on startup? "
                       "If not, you can check manually from the help menu.")
                    .arg(qApp->applicationName()));
        shouldCheckDialog.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        shouldCheckDialog.setDefaultButton(QMessageBox::Yes);
        int response = shouldCheckDialog.exec();
        if (response == QMessageBox::Yes)
        {
            setShouldCheckForUpdatesOnStartup(true);
            checkForUpdatesInBackgroundIfNecessary();
        }
        else
            setShouldCheckForUpdatesOnStartup(false);
        return;
    }

    if (!shouldCheckForUpdatesOnStartup())
        return;

    checkForUpdatesInBackgroundIfNecessary();
}


bool HGUpdateCheck::shouldCheckForUpdatesOnStartup()
{
    if (_settings == NULL)
        return false;
    _settings->sync();
    return _settings->value(kSettingKeyShouldCheckForUpdatesAtStartup).toBool();
}
void HGUpdateCheck::setShouldCheckForUpdatesOnStartup(bool value)
{
    if (_settings == NULL)
        return;
    _settings->setValue(kSettingKeyShouldCheckForUpdatesAtStartup, value);
    _settings->sync();
}

void HGUpdateCheck::checkForUpdatesInBackgroundIfNecessary()
{
    QDate today = QDate::currentDate();
    _settings->sync();
    QDate lastUpdateCheck = _settings->value(kSettingKeyLastUpdateCheckTime).toDate();
    if (lastUpdateCheck.isValid())
    {
        qDebug() << "HGUpdateCheck: Last update check was" << lastUpdateCheck << "(today is:)" << today;
        if (lastUpdateCheck == today)
        {
            qDebug() << "HGUpdateCheck: Checked today already.";
            return;
        }
    }
    else
        qDebug() << "HGUpdateCheck: Last update check was never.";

    checkForUpdatesNow(false);
}

void HGUpdateCheck::checkForUpdatesNow(bool userInitiated)
{
    if (_activeReply != NULL)
    {
        qDebug() << "HGUpdateCheck: Already checking for updates!";
        if (userInitiated)
            QMessageBox::information((QWidget*)parent(),
                                     qApp->applicationName(),
                                     qApp->applicationName()+" is already "
                                     "checking for updates.");
        return;
    }

    qDebug() << "HGUpdateCheck: Checking for updates.";
    QNetworkRequest request(QUrl(_baseURL + "?versioncheck=y"));
    request.setAttribute(kRequestKindAttribute, kRequestKind_UpdateCheck);
    request.setAttribute(kRequestUserInitiatedAttribute, QVariant(userInitiated));
    //qDebug() << "HGUpdateCheck: send:" << request.url();
    if (userInitiated)
        _progressDialog->show();
    _canceled = false;
    _activeReply = _nam->get(request);
}

int compareVersionNumbers(QString first, QString second)
{
    QStringList firstParts = first.split('.');
    QStringList secondParts = second.split('.');

    int longerCount = (firstParts.size() < secondParts.size())
                      ? secondParts.size() : firstParts.size();
    for (int i = 0; i < longerCount; i++)
    {
        int firstInt = (i < firstParts.size()) ? firstParts.at(i).toInt() : 0;
        int secondInt = (i < secondParts.size()) ? secondParts.at(i).toInt() : 0;
        if (firstInt < secondInt)
            return 1;
        else if (firstInt > secondInt)
            return -1;
    }
    return 0;
}

void HGUpdateCheck::handleLatestVersionInfo(QString latestVersion, bool userInitiated)
{
    _latestVersion = latestVersion;

    if (!userInitiated)
    {
        _settings->sync();
        QString lastSkippedVersion = _settings->value(kSettingKeyLastSkippedVersion).toString();
        if (!lastSkippedVersion.isNull()
            && compareVersionNumbers(lastSkippedVersion,
                                     latestVersion) < 1)
        {
            qDebug() << "HGUpdateCheck: user wants to skip this version.";
            return;
        }
    }

    if (compareVersionNumbers(qApp->applicationVersion(),
                              latestVersion) == 1)
    {
        // Update available
        QNetworkRequest request(QUrl(_baseURL + "?whatschanged=y"));
        request.setAttribute(kRequestKindAttribute, kRequestKind_WhatsChanged);
        //qDebug() << "HGUpdateCheck: send:" << request.url();
        _canceled = false;
        _activeReply = _nam->get(request);
        return;
    }

    _settings->setValue(kSettingKeyLastUpdateCheckTime, QDate::currentDate());
    _settings->sync();

    qDebug() << "HGUpdateCheck: up to date.";

    if (userInitiated)
    {
        QMessageBox infoBox;
        infoBox.setIcon(QMessageBox::Information);
        infoBox.setText("You're Up to Date");
        infoBox.setInformativeText(qApp->applicationName() + " "
                                   +qApp->applicationVersion() + " is currently "
                                   "the latest version.");
        infoBox.exec();
    }
}

void HGUpdateCheck::handleWhatsChanged(QString whatsChangedHTML)
{
    _updateInfoDialog->presentUpdate(_latestVersion, whatsChangedHTML);
}


void HGUpdateCheck::handleRemindMeLater()
{
    // do nothing.
}
void HGUpdateCheck::handleSkipThisVersion()
{
    _settings->setValue(kSettingKeyLastSkippedVersion, _latestVersion);
}
void HGUpdateCheck::handleUpdateAccepted()
{
    QDesktopServices::openUrl(QUrl(_baseURL));
}

void HGUpdateCheck::handleError(QString errorMessage, bool userInitiated)
{
    _progressDialog->hide();

    if (!userInitiated)
        return;

    QMessageBox errorMessageBox;
    errorMessageBox.setIcon(QMessageBox::Critical);
    errorMessageBox.setText("Could Not Check for Updates");
    errorMessageBox.setInformativeText(errorMessage + "\n\n"
                                       "You can go to the application website "
                                       "to check for updates manually:\n\n"
                                       + _baseURL);
    errorMessageBox.setStandardButtons(QMessageBox::Close);
    errorMessageBox.addButton("Go to Website", QMessageBox::AcceptRole);
    int ret = errorMessageBox.exec();
    if (ret != QMessageBox::Close)
        QDesktopServices::openUrl(QUrl(_baseURL));

}



void HGUpdateCheck::canceledFromProgressDialog()
{
    qDebug() << "HGUpdateCheck: canceled.";
    if (_activeReply != NULL)
    {
        _canceled = true;
        _activeReply->abort();
        _activeReply = NULL;
    }
}


void HGUpdateCheck::replyFinished(QNetworkReply *reply)
{
    if (_canceled)
    {
        reply->deleteLater();
        return;
    }

    _activeReply = NULL;
    QNetworkRequest request = reply->request();
    QVariant requestKind = request.attribute(kRequestKindAttribute);
    bool userInitiated = request.attribute(kRequestUserInitiatedAttribute).toBool();

    // follow redirects:
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (!redirectUrl.isEmpty())
    {
        redirectUrl = redirectUrl.resolved(redirectUrl);
        QNetworkRequest redirectRequest(redirectUrl);
        redirectRequest.setAttribute(kRequestKindAttribute, requestKind);
        redirectRequest.setAttribute(kRequestUserInitiatedAttribute, QVariant(userInitiated));
        //qDebug() << "HGUpdateCheck: redirect:" << redirectRequest.url();
        _canceled = false;
        _activeReply = _nam->get(redirectRequest);
        reply->deleteLater();
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200)
    {
        qDebug() << "HGUpdateCheck: got status:" << statusCode;
        QString statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        QString errorMessage = statusText.trimmed().isEmpty()
                               ? QString("Server response: HTTP status %1").arg(statusCode)
                               : ("Server response: " + statusText);
        handleError(errorMessage, userInitiated);
        reply->deleteLater();
        return;
    }

    _progressDialog->hide();

    QTextStream textStream(reply);
    textStream.setCodec("UTF-8");
    QString responseStr = textStream.readAll();

    //qDebug() << "HGUpdateCheck: got response:" << responseStr;

    if (requestKind == kRequestKind_UpdateCheck)
    {
        handleLatestVersionInfo(responseStr, userInitiated);
    }
    else if (requestKind == kRequestKind_WhatsChanged)
    {
        handleWhatsChanged(responseStr);
    }

    reply->deleteLater();
}

void HGUpdateCheck::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(authenticator);
    bool userInitiated = reply->request().attribute(kRequestUserInitiatedAttribute).toBool();
    handleError("Server requests for authentication.", userInitiated);
}

void HGUpdateCheck::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_UNUSED(proxy); Q_UNUSED(authenticator);
    handleError("Proxy requests for authentication.", false);
}

void HGUpdateCheck::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
    bool userInitiated = reply->request().attribute(kRequestUserInitiatedAttribute).toBool();
    handleError("SSL Error(s).", userInitiated);
}
