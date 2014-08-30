#include "filesearchdialog.h"
#include "ui_filesearchdialog.h"
#include "logger.h"
#include <QtCore/QFileInfo>

FileSearchDialog::FileSearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileSearchDialog)
{
    ui->setupUi(this);
    setupConnections();
    ui->lineEdit->installEventFilter(this);
    ui->listView->installEventFilter(this);

    fileListModel = new QStandardItemModel();
    ui->listView->setModel(fileListModel);
}

FileSearchDialog::~FileSearchDialog()
{
    delete ui;
}

void FileSearchDialog::setupConnections()
{
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(ui->lineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(inputTextEdited(QString)));
}

void FileSearchDialog::resetWithFilePaths(QStringList aFilePaths)
{
    filePaths = aFilePaths;
    ui->lineEdit->clear();
    ui->lineEdit->setFocus();
    updateSearchResults("");
}

void FileSearchDialog::clearFileList()
{
    fileListModel->clear();
}

#define PATH_ROLE Qt::UserRole

void FileSearchDialog::accept()
{
    if (0 < fileListModel->rowCount())
    {
        QModelIndexList selectedRows = ui->listView->selectionModel()->selectedRows();
        int selectedRow = (0 < selectedRows.count()) ? selectedRows.at(0).row() : 0;
        QStandardItem *selectedItem = fileListModel->item(selectedRow, 0);
        QString selectedPath = selectedItem->data(PATH_ROLE).toString();
        Logger::debug(QString("path: %1").arg(selectedPath));
        emit selectedFilePath(selectedPath);
    }
    else
        Logger::debug("No selection");

    clearFileList();
    QDialog::accept();
}

void FileSearchDialog::reject()
{
    clearFileList();
    QDialog::reject();
}


bool FileSearchDialog::queryMatchesPath(QString query, QString path)
{
    if (query.length() == 0)
        return false;
    return (path.toLower().indexOf(query) != -1);
}


void FileSearchDialog::updateSearchResults(QString searchQuery)
{
    if (filePaths.isEmpty())
        return;
    bool queryIsEmpty = searchQuery.trimmed().isEmpty();
    fileListModel->clear();
    int count = filePaths.count();
    for (int i = 0; i < count; i++)
    {
        QString path = filePaths.at(i);
        if (!QFile::exists(path) || (!queryIsEmpty && !queryMatchesPath(searchQuery, path)))
            continue;
        QStandardItem *item = new QStandardItem();
        QString basename = QFileInfo(path).fileName();
        QString parentDirPath = QFileInfo(path).path();
        item->setText(tr("%1  —  %2").arg(basename).arg(parentDirPath));
        item->setData(path, PATH_ROLE);
        fileListModel->appendRow(item);
    }
}


void FileSearchDialog::inputTextEdited(QString newText)
{
    updateSearchResults(newText);
}


bool FileSearchDialog::eventFilter(QObject* obj, QEvent *event)
{
    if (obj != ui->lineEdit && obj != ui->listView)
        return QDialog::eventFilter(obj, event);

    if (event->type() != QEvent::KeyPress)
        return false;

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    if (obj == ui->lineEdit)
    {
        if (keyEvent->key() == Qt::Key_Down)
        {
            ui->listView->setFocus();
            QModelIndex index = fileListModel->index(0, 0);
            ui->listView->setCurrentIndex(index);
            return true;
        }
    }
    else if (obj == ui->listView)
    {
        if (keyEvent->key() == Qt::Key_Return)
        {
            accept();
            return true;
        }
    }
    return false;
}
