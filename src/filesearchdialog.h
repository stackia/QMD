#ifndef FILESEARCHDIALOG_H
#define FILESEARCHDIALOG_H

#include <QtWidgets/QDialog>
#include <QtGui/QStandardItemModel>
#include <QtGui/QKeyEvent>

namespace Ui {
    class FileSearchDialog;
}

class FileSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileSearchDialog(QWidget *parent = 0);
    ~FileSearchDialog();

    void resetWithFilePaths(QStringList filePaths);
    bool eventFilter(QObject* obj, QEvent *event);

private:
    Ui::FileSearchDialog *ui;
    QStringList filePaths;
    QStandardItemModel *fileListModel;
    void setupConnections();
    void clearFileList();
    void updateSearchResults(QString searchQuery);
    bool queryMatchesPath(QString query, QString path);

private slots:
    void inputTextEdited(QString newText);
    void accept();
    void reject();

signals:
    void selectedFilePath(QString path);

};

#endif // FILESEARCHDIALOG_H
