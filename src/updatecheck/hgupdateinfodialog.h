#ifndef HGUPDATEINFODIALOG_H
#define HGUPDATEINFODIALOG_H

#include <QtWidgets/QDialog>

namespace Ui {
    class HGUpdateInfoDialog;
}

class HGUpdateInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HGUpdateInfoDialog(QWidget *parent = 0);
    ~HGUpdateInfoDialog();

    void presentUpdate(QString newVersion, QString releaseNotesHTML);

signals:
    void skipThisVersion();
    void remindLater();
    void updateAccepted();
private slots:
    void skipThisVersionButtonClick();
    void remindLaterButtonClick();
    void updateAcceptButtonClick();

private:
    Ui::HGUpdateInfoDialog *ui;
};

#endif // HGUPDATEINFODIALOG_H
