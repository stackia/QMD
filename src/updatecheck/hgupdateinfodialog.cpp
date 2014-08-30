#include "hgupdateinfodialog.h"
#include "ui_hgupdateinfodialog.h"

#include <QtGui/QFontDatabase>

HGUpdateInfoDialog::HGUpdateInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HGUpdateInfoDialog)
{
    ui->setupUi(this);

    setWindowTitle(qApp->applicationName() + " Update");
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint
                   | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);

    QStringList fontFamilies = QFontDatabase().families();
    if (fontFamilies.contains(QString("Helvetica")))
        ui->webView->settings()->setFontFamily(QWebSettings::StandardFont,
                                               QString("Helvetica"));
    else if (fontFamilies.contains(QString("Arial")))
        ui->webView->settings()->setFontFamily(QWebSettings::StandardFont,
                                               QString("Arial"));

    connect(ui->skipThisVersionButton, SIGNAL(clicked()),
            this, SLOT(skipThisVersionButtonClick()));
    connect(ui->remindMeLaterButton, SIGNAL(clicked()),
            this, SLOT(remindLaterButtonClick()));
    connect(ui->goToWebsiteButton, SIGNAL(clicked()),
            this, SLOT(updateAcceptButtonClick()));
}

HGUpdateInfoDialog::~HGUpdateInfoDialog()
{
    delete ui;
}

void HGUpdateInfoDialog::presentUpdate(QString newVersion, QString releaseNotesHTML)
{
    ui->headerLabel->setText("A new version of " + qApp->applicationName() + " is available");
    ui->subtitleLabel->setText("Version "+newVersion+" is available -- you have "+qApp->applicationVersion());
    ui->webView->setHtml(releaseNotesHTML);
    show();
}

void HGUpdateInfoDialog::skipThisVersionButtonClick()
{
    close();
    ui->webView->setHtml("");
    emit skipThisVersion();
}
void HGUpdateInfoDialog::remindLaterButtonClick()
{
    close();
    ui->webView->setHtml("");
    emit remindLater();
}
void HGUpdateInfoDialog::updateAcceptButtonClick()
{
    close();
    ui->webView->setHtml("");
    emit updateAccepted();
}
