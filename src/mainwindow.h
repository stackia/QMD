#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtGui/QSessionManager>

#include "peg-markdown-highlight/highlighter.h"
#include "preferencesdialog.h"
#include "filesearchdialog.h"
#include "editor/QMDtextedit.h"
#include "markdowncompiler.h"
//#include "updatecheck/updatecheck.h"

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void show();

public slots:
    void newFile();
    void openFile(const QString &path = QString::null);
    void saveFile(QString targetPath);
    void saveCurrentFile();
    void saveMenuItemHandler();
    void saveAsMenuItemHandler();
    void revertToSaved();
    void switchToPreviousFile();
    void revealFileDir();

    void increaseFontSize();
    void decreaseFontSize();
    void showPreferences();
    void about();
    void checkForUpdates();

    void selectTextToSearchFor();
    void findNextSearchMatch();
    void findPreviousSearchMatch();

    void formatSelectionEmphasized();
    void formatSelectionStrong();
    void formatSelectionCode();

    void preferencesUpdated();

    void openRecentFile();
    void showRecentFileSearchDialog();
    void showNotesFolderFileSearchDialog();
    void fileSearchDialogSelectedFilePath(QString path);
    void compileToTempHTML();
    void compileToHTMLAs();
    void recompileToHTML();

    void anchorClicked(const QUrl &link);
    void handleCustomContextMenuRequest(QPoint);
    void lookupInDictionary();

    QMessageBox::ButtonRole offerToSaveChangesIfNecessary();

    void handleApplicationLaunched();
    bool confirmQuit(bool interactionAllowed);
    void commitDataHandler(QSessionManager &manager);
#ifdef Q_OS_MAC
    void cocoaCommitDataHandler();
#endif
    void aboutToQuitHandler();
    void quitActionHandler();

    void handleContentsChange(int position, int charsRemoved, int charsAdded);
    void reportStyleParsingErrors(QList<QPair<int, QString> > *list);

protected:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj,  QEvent *event);

private:
    enum FileDialogKind
    {
        OpenFileDialog,
        SaveFileDialog,
        CompilationOutputDialog
    };

    QString getPathFromFileDialog(FileDialogKind dialogKind);
    void setOpenFilePath(QString newValue);
    QString getMarkdownFilesFilter();
    QStringList getMarkdownFilesFilterList();
    void setupEditor();
    void setupFileMenu();
    void updateRecentFilesMenu();
    void performStartupTasks();
    void trimRecentFilesList();
    void addToRecentFiles(QString filePath);
    void saveViewPositions(QString filePath, int scrollPosition, int cursorPosition);
    QPair<int, int> getViewPositions(QString filePath);
    void saveCurrentFileViewPositions();
    void loadAndSetCurrentFileViewPositions();
    void persistFontInfo();
    void applyPersistedFontInfo();
    void applyStyleWithoutErrorReporting();
    void applyStyle(bool reportParsingErrorsToUser = true);
    void applyHighlighterPreferences();
    void applyEditorPreferences();
    bool isDirty();
    void setDirty(bool value);
    bool compileToHTMLFile(QString targetPath);
    void checkIfFileModifiedByThirdParty();

    //HGUpdateCheck *updateCheck;
    MarkdownCompiler *compiler;
    QString lastCompileTargetPath;

    PreferencesDialog *preferencesDialog;
    FileSearchDialog *fileSearchDialog;
    QSettings *settings;
    QMDTextEdit *editor;
    HGMarkdownHighlighter *highlighter;
    QString openFilePath;
    QDateTime openFileKnownLastModified;
    QString searchString;

    QMenu *recentFilesMenu;
    QList<QAction *> *recentFilesMenuActions;

    QAction *findNextMenuAction;
    QAction *findPreviousMenuAction;
    QAction *revertToSavedMenuAction;
    QAction *recompileAction;
    QAction *revealFileAction;
    QAction *switchToPreviousFileAction;

    bool discardingChangesOnQuit;
};

#endif
