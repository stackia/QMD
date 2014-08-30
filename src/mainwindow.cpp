#include <QtGui/QDesktopServices>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QScrollBar>
#include <QtCore/QTextStream>
#include <QtCore/QCryptographicHash>

#include "mainwindow.h"
#include "defines.h"
#include "logger.h"
#include "QMDapplication.h"

#ifdef Q_OS_MAC
#include <Cocoa/Cocoa.h>
#endif

/*
TODO:
- More relevant matching in the file search dialog

- Highlight whole blockquotes in PMH

- OS X: Catch the maximize/zoom action (window button + menu item) and set custom "zoomed" size
- Apply highlighting styles incrementally (might not be very easy, though)

- Use QTextOption::ShowTabsAndSpaces
- Use Sparkle on OS X
- Support "quoted args" for compilers (remember to test on Windows !)
*/

#define kUntitledFileUIName "Untitled"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    discardingChangesOnQuit = false;
    settings = new QSettings("org.stackia", "QMD");
    compiler = new MarkdownCompiler(settings);

    // PreferencesDialog depends on the HGUpdateCheck settings being
    // set, so we have to set that up first:
    //HGUpdateCheck::setUpdateCheckSettings(settings);
    //updateCheck = new HGUpdateCheck(((QMDApplication *)qApp)->websiteURL(), this);

    preferencesDialog = new PreferencesDialog(settings, compiler);
    fileSearchDialog = new FileSearchDialog(this);
    fileSearchDialog->setWindowModality(Qt::WindowModal);

    recentFilesMenuActions = new QList<QAction *>();

    setupFileMenu();
    setupEditor();
    setCentralWidget(editor);

    statusBar()->show();

    qApp->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    //delete updateCheck;
    delete settings;
    delete preferencesDialog;
    delete compiler;
    delete recentFilesMenuActions;
}

void MainWindow::handleApplicationLaunched()
{
    Logger::debug("MainWindow: handleApplicationLaunched");
    performStartupTasks();
}

void MainWindow::show()
{
    QSize defaultSize(900, 300);
    resize(defaultSize);

    bool rememberWindow = settings->value(SETTING_REMEMBER_WINDOW,
                                          QVariant(DEF_REMEMBER_WINDOW)).toBool();
    if (rememberWindow) {
        restoreGeometry(settings->value(SETTING_WINDOW_GEOMETRY).toByteArray());
        restoreState(settings->value(SETTING_WINDOW_STATE).toByteArray());
    }

    QMainWindow::show();
}

void MainWindow::checkIfFileModifiedByThirdParty()
{
    // If we don't have a known modification date, we can't do anything:
    if (openFileKnownLastModified.isNull())
        return;
    bool shouldAsk = settings->value(SETTING_ASK_RELOAD_MODIFIED_FILE, DEF_ASK_RELOAD_MODIFIED_FILE).toBool();
    if (!shouldAsk)
        return;

    QDateTime currentLastModified = QFileInfo(openFilePath).lastModified();
    if (openFileKnownLastModified < currentLastModified)
    {
        openFileKnownLastModified = currentLastModified;

        QMessageBox revertMessageBox(this);
        revertMessageBox.setWindowModality(Qt::WindowModal);
        revertMessageBox.setIcon(QMessageBox::Warning);
        revertMessageBox.setText(
                    tr("是否需要重新载入文件 “%1”？")
                    .arg(QFileInfo(openFilePath).fileName()));
        revertMessageBox.setInformativeText(
                    tr("这个文件可能已被外部程序修改，是否需要重新载入它？"));
        revertMessageBox.setDefaultButton(revertMessageBox.addButton(tr("重新载入"), QMessageBox::AcceptRole));
        revertMessageBox.addButton(tr("保持原样"), QMessageBox::RejectRole);
        revertMessageBox.exec();

        QMessageBox::ButtonRole selectedButtonRole = revertMessageBox.buttonRole(revertMessageBox.clickedButton());
        if (selectedButtonRole == QMessageBox::AcceptRole)
            revertToSaved();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != qApp)
        return QMainWindow::eventFilter(obj, event);

    if (event->type() == QEvent::ApplicationActivate)
        checkIfFileModifiedByThirdParty();

    return false;
}

QString standardizeFilePath(QString filePath)
{
    QFileInfo fileInfo(filePath);
    QString std = fileInfo.canonicalFilePath();
    if (std.isEmpty()) // if the path does not exist
        return filePath;
    return std;
}

void MainWindow::setOpenFilePath(QString newValue)
{
    openFilePath = newValue;
    revertToSavedMenuAction->setEnabled(!openFilePath.isNull());
    revealFileAction->setEnabled(!openFilePath.isNull());
    openFileKnownLastModified = openFilePath.isNull()
                                ? QDateTime()
                                : QFileInfo(openFilePath).lastModified();
}

void MainWindow::newFile()
{
    QMessageBox::ButtonRole selectedButtonRole = offerToSaveChangesIfNecessary();
    if (selectedButtonRole == QMessageBox::RejectRole)
        return;

    editor->clear();
    setOpenFilePath(QString::null);
    lastCompileTargetPath = QString::null;
    recompileAction->setEnabled(false);
    setDirty(false);
    updateRecentFilesMenu();
}

QString MainWindow::getMarkdownFilesFilter()
{
    QStringList extensions = settings->value(SETTING_EXTENSIONS, DEF_EXTENSIONS)
                             .toString().split(' ', QString::SkipEmptyParts);
    if (extensions.count() == 0)
        return "All Files (*.*)";

    QString filesFilter = tr("Markdown 文件") + " (";
    foreach (QString ext, extensions)
    {
        QString cleanExt = ext.trimmed();
        if (cleanExt.startsWith("."))
            cleanExt = cleanExt.remove(0,1);
        filesFilter += "*." + cleanExt + " ";
    }
    filesFilter.chop(1); // remove last space
    filesFilter += ")";
    return filesFilter;
}

QStringList MainWindow::getMarkdownFilesFilterList()
{
    QStringList extensions = settings->value(SETTING_EXTENSIONS, DEF_EXTENSIONS)
                             .toString().split(' ', QString::SkipEmptyParts);
    if (extensions.count() == 0)
        return QStringList("*.*");

    QStringList filterList;
    foreach (QString ext, extensions)
    {
        QString cleanExt = ext.trimmed();
        if (cleanExt.startsWith("."))
            cleanExt = cleanExt.remove(0,1);
        filterList.append("*." + cleanExt);
    }
    return filterList;
}

QString MainWindow::getPathFromFileDialog(FileDialogKind dialogKind)
{
    QString title;
    QString defaultPath;
    QString filesFilter;

    switch (dialogKind)
    {
        case OpenFileDialog:
            title = tr("打开文件");
            // intentional fall-thru:
        case SaveFileDialog:
            if (title.isNull())
                title = tr("保存文件");
            filesFilter = getMarkdownFilesFilter();

            if (openFilePath.isNull())
                defaultPath = settings->value(SETTING_LAST_FILE_DIALOG_PATH,
                                              QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
            else
                defaultPath = QFileInfo(openFilePath).absolutePath();

            break;
        case CompilationOutputDialog:
            title = tr("保存 HTML 输出");
            defaultPath = settings->value(SETTING_LAST_COMPILE_DIALOG_PATH,
                                          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
            break;
        default:
            title = tr("选择文件");
    }

    QString retVal;
    if (dialogKind == OpenFileDialog)
        retVal = QFileDialog::getOpenFileName(this, title, defaultPath, filesFilter);
    else
        retVal = QFileDialog::getSaveFileName(this, title, defaultPath, filesFilter);

    if (!retVal.isNull())
    {
        if (dialogKind == CompilationOutputDialog)
            settings->setValue(SETTING_LAST_COMPILE_DIALOG_PATH, QFileInfo(retVal).absolutePath());
        else
            settings->setValue(SETTING_LAST_FILE_DIALOG_PATH, QFileInfo(retVal).absolutePath());
    }

    return retVal;
}

void MainWindow::openFile(const QString &path)
{
    saveCurrentFileViewPositions();
    QMessageBox::ButtonRole selectedButtonRole = offerToSaveChangesIfNecessary();
    if (selectedButtonRole == QMessageBox::RejectRole)
        return;

    QString filePathToOpen = path;

    if (filePathToOpen.isNull())
        filePathToOpen = getPathFromFileDialog(OpenFileDialog);

    if (filePathToOpen.isEmpty()) // canceled?
        return;

    filePathToOpen = standardizeFilePath(filePathToOpen);

    QFile file(filePathToOpen);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, tr("无法打开文件"),
                             tr("无法打开: %1 (原因: %2)")
                             .arg(filePathToOpen)
                             .arg(file.errorString()));
        return;
    }

    QTextStream inStream(&file);
    inStream.setCodec("UTF-8");
    editor->setPlainText(inStream.readAll());
    file.close();

    setOpenFilePath(filePathToOpen);
    recompileAction->setEnabled(false);
    lastCompileTargetPath = QString::null;

    setDirty(false);
    bool rememberLastFile = settings->value(SETTING_REMEMBER_LAST_FILE,
                                            QVariant(DEF_REMEMBER_LAST_FILE)).toBool();
    if (rememberLastFile) {
        settings->setValue(SETTING_LAST_FILE, QVariant(openFilePath));
        settings->sync();
    }
    addToRecentFiles(openFilePath);
    updateRecentFilesMenu();

    loadAndSetCurrentFileViewPositions();
}

void MainWindow::saveFile(QString targetPath)
{
    bool savingNewFile = (targetPath.isNull());

    QString saveFilePath(targetPath);
    if (saveFilePath.isNull())
        saveFilePath = getPathFromFileDialog(SaveFileDialog);

    if (saveFilePath.isEmpty()) // canceled?
        return;

    QFile file(saveFilePath);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this, tr("无法保存文件"),
                             tr("无法保存: %1 (原因: %2)")
                             .arg(saveFilePath)
                             .arg(file.errorString()));
        return;
    }
    QTextStream outStream(&file);
    outStream.setCodec("UTF-8");
    outStream << editor->toPlainText();
    file.close();

    setOpenFilePath(saveFilePath);
    setDirty(false);

    if (savingNewFile)
    {
        bool rememberLastFile = settings->value(SETTING_REMEMBER_LAST_FILE,
                                                QVariant(DEF_REMEMBER_LAST_FILE)).toBool();
        if (rememberLastFile) {
            settings->setValue(SETTING_LAST_FILE, QVariant(saveFilePath));
            settings->sync();
        }
        addToRecentFiles(saveFilePath);
        updateRecentFilesMenu();
    }

    statusBar()->showMessage(tr("文件已保存: %1").arg(QFileInfo(saveFilePath).fileName()), 3000);
}

void MainWindow::saveCurrentFile()
{
    saveFile(openFilePath);
}

void MainWindow::saveMenuItemHandler()
{
    saveCurrentFile();
}

void MainWindow::saveAsMenuItemHandler()
{
    saveFile(QString::null);
}

void MainWindow::revertToSaved()
{
    if (openFilePath.isNull())
        return;
    openFile(openFilePath);
}

void MainWindow::switchToPreviousFile()
{
    openFile(recentFilesMenuActions->at(0)->data().toString());
}

void MainWindow::revealFileDir()
{
    if (openFilePath.isNull())
        return;
    QDesktopServices::openUrl(QUrl("file:///"+QFileInfo(openFilePath).absolutePath()));
}

void MainWindow::trimRecentFilesList()
{
    // Trim the "recent files" list
    QStringList recentFiles = settings->value(SETTING_RECENT_FILES).toStringList();
    int maxNumRecentFiles = settings->value(SETTING_NUM_RECENT_FILES, DEF_NUM_RECENT_FILES).toInt();
    while (maxNumRecentFiles < recentFiles.count())
        recentFiles.removeLast();
    settings->setValue(SETTING_RECENT_FILES, recentFiles);

    // Trim the "file view positions" list
    QMap<QString, QVariant> positions = settings->value(SETTING_RECENT_FILE_VIEW_POSITIONS).toMap();
    foreach (QString fp, positions.keys())
    {
        if (!recentFiles.contains(fp))
            positions.remove(fp);
    }
    settings->setValue(SETTING_RECENT_FILE_VIEW_POSITIONS, positions);

    settings->sync();
}

void MainWindow::addToRecentFiles(QString filePath)
{
    QString stdFilePath = standardizeFilePath(filePath);

    QStringList recentFiles = settings->value(SETTING_RECENT_FILES).toStringList();

    int index = recentFiles.indexOf(stdFilePath);
    if (-1 < index)
        recentFiles.removeAt(index);
    recentFiles.insert(0, stdFilePath);

    settings->setValue(SETTING_RECENT_FILES, recentFiles);
    this->trimRecentFilesList(); // calls sync() on the settings
}

void MainWindow::saveViewPositions(QString filePath, int scrollPosition, int cursorPosition)
{
    QString stdFilePath = standardizeFilePath(filePath);
    QMap<QString, QVariant> positionsByFile = settings->value(SETTING_RECENT_FILE_VIEW_POSITIONS).toMap();
    QList<QVariant> thisPositions;
    thisPositions << scrollPosition << cursorPosition;
    positionsByFile.insert(stdFilePath, QVariant(thisPositions));
    settings->setValue(SETTING_RECENT_FILE_VIEW_POSITIONS, positionsByFile);
    settings->sync();
}
QPair<int,int> MainWindow::getViewPositions(QString filePath)
{
    QMap<QString, QVariant> positionsByFile = settings->value(SETTING_RECENT_FILE_VIEW_POSITIONS).toMap();
    QList<QVariant> thisPositions = positionsByFile.value(standardizeFilePath(filePath)).toList();
    if (thisPositions.isEmpty())
        return QPair<int,int>(0,0);
    return QPair<int,int>(thisPositions.at(0).toInt(), thisPositions.at(1).toInt());
}
void MainWindow::saveCurrentFileViewPositions()
{
    if (openFilePath.isNull())
        return;
    saveViewPositions(openFilePath,
                      editor->verticalScrollBar()->value(),
                      editor->textCursor().position());
}
void MainWindow::loadAndSetCurrentFileViewPositions()
{
    if (openFilePath.isNull())
        return;
    QPair<int,int> scrollAndCursorPositions = getViewPositions(openFilePath);
    editor->verticalScrollBar()->setValue(scrollAndCursorPositions.first);

    QTextCursor cursor = editor->textCursor();
    int savedCursorPos = scrollAndCursorPositions.second;
    int maxCursorPos = editor->document()->characterCount() - 1;
    cursor.setPosition(std::max(0, std::min(savedCursorPos, maxCursorPos)));
    editor->setTextCursor(cursor);
}


void MainWindow::persistFontInfo()
{
    settings->setValue(SETTING_FONT, QVariant(editor->font().toString()));
    settings->sync();
}
void MainWindow::applyPersistedFontInfo()
{
    // font
    QFont font;
    if (settings->contains(SETTING_FONT))
        font.fromString(settings->value(SETTING_FONT).toString());
    else {
        font.setFamily(DEF_FONT_FAMILY);
        font.setPointSize(DEF_FONT_SIZE);
        font.setFixedPitch(true);
    }
    editor->setFont(font);

    // tab stop width (dependent on font)
    int tabWidthInChars = settings->value(SETTING_TAB_WIDTH,
                                          QVariant(DEF_TAB_WIDTH)).toInt();
    QFontMetrics fontMetrics(font);
    editor->setTabStopWidth(fontMetrics.charWidth("m", 0) * tabWidthInChars);
}

void MainWindow::selectTextToSearchFor()
{
    bool ok;
    QString str = QInputDialog::getText(this, tr("查找文本"),
                                        tr("输入要查找的文本:"),
                                        QLineEdit::Normal, searchString, &ok);
    if (!ok || str.isEmpty())
        return;
    searchString = str;
    findNextMenuAction->setEnabled(true);
    findPreviousMenuAction->setEnabled(true);
    findNextSearchMatch();
}

void MainWindow::findNextSearchMatch()
{
    if (searchString.isEmpty())
        return;
    editor->find(searchString);
}

void MainWindow::findPreviousSearchMatch()
{
    if (searchString.isEmpty())
        return;
    editor->find(searchString, QTextDocument::FindBackward);
}

void MainWindow::increaseFontSize()
{
    QFont font(editor->font());
    font.setPointSize(editor->font().pointSize() + 1);
    editor->setFont(font);
    persistFontInfo();

    // need to update relative font sizes:
    applyStyleWithoutErrorReporting();
    highlighter->parseAndHighlightNow();
}

void MainWindow::decreaseFontSize()
{
    QFont font(editor->font());
    font.setPointSize(editor->font().pointSize() - 1);
    editor->setFont(font);
    persistFontInfo();

    // need to update relative font sizes:
    applyStyleWithoutErrorReporting();
    highlighter->parseAndHighlightNow();
}

void MainWindow::about()
{
    QString title = tr("关于 %1").arg(QCoreApplication::applicationName());
    QString msg =
            tr("版本 %1"
               "\n\n"
               "Copyright © %2 %3"
               "\n\n"
               "%4")
            .arg(QCoreApplication::applicationVersion())
            .arg(((QMDApplication *)qApp)->copyrightYear())
            .arg("Stackia")
            .arg(((QMDApplication *)qApp)->websiteURL());
    QMessageBox aboutBox;
    aboutBox.setIconPixmap(QPixmap(":/smallAppIcon.png"));
    aboutBox.setText(title);
    aboutBox.setInformativeText(msg);
    aboutBox.exec();
}

void MainWindow::checkForUpdates()
{
    //updateCheck->checkForUpdatesNow();
}

void MainWindow::applyStyleWithoutErrorReporting()
{
    applyStyle(false);
}
void MainWindow::applyStyle(bool reportParsingErrorsToUser)
{
    if (reportParsingErrorsToUser)
    {
        connect(highlighter, SIGNAL(styleParsingErrors(QList<QPair<int, QString> >*)),
                this, SLOT(reportStyleParsingErrors(QList<QPair<int, QString> >*)));
    }
    else
        disconnect(this, SLOT(reportStyleParsingErrors(QList<QPair<int,QString> >*)));

    QString styleFilePath = settings->value(SETTING_STYLE,
                                            QVariant(DEF_STYLE)).toString();
    if (!QFile::exists(styleFilePath))
    {
        QMessageBox::warning(this, tr("载入样式出错"),
                             tr("无法载入样式文件:\n'%1'"
                                "\n\n"
                                "已退回默认样式。")
                             .arg(styleFilePath)
                             );
        styleFilePath = DEF_STYLE;
        settings->setValue(SETTING_STYLE, DEF_STYLE);
        settings->sync();
    }
    highlighter->getStylesFromStylesheet(styleFilePath, editor);
    editor->setCurrentLineHighlightColor(highlighter->currentLineHighlightColor);
    editor->setLineNumberAreaColor(editor->palette().base().color().darker(140));
}

void MainWindow::applyHighlighterPreferences()
{
    double highlightInterval = settings->value(SETTING_HIGHLIGHT_INTERVAL,
                                               QVariant(DEF_HIGHLIGHT_INTERVAL)).toDouble();
    highlighter->setWaitInterval(highlightInterval);

    bool clickableLinks = settings->value(SETTING_CLICKABLE_LINKS,
                                          QVariant(DEF_CLICKABLE_LINKS)).toBool();
    highlighter->setMakeLinksClickable(clickableLinks);

    applyStyle();
}

void MainWindow::applyEditorPreferences()
{
    // Indentation
    bool indentWithTabs = settings->value(SETTING_INDENT_WITH_TABS,
                                          QVariant(DEF_INDENT_WITH_TABS)).toBool();
    int tabWidthInChars = settings->value(SETTING_TAB_WIDTH,
                                          QVariant(DEF_TAB_WIDTH)).toInt();
    editor->setSpacesIndentWidthHint(tabWidthInChars);
    if (indentWithTabs)
        editor->setIndentString("\t");
    else
    {
        QString indentStr = " ";
        for (int i = 1; i < tabWidthInChars; i++)
            indentStr += " ";
        editor->setIndentString(indentStr);
    }

    // Current line highlighting
    bool highlightCurrentLine = settings->value(SETTING_HIGHLIGHT_CURRENT_LINE,
                                                QVariant(DEF_HIGHLIGHT_CURRENT_LINE)).toBool();
    editor->setHighlightCurrentLine(highlightCurrentLine);

    // Formatting
    bool emphWithUnderscores = settings->value(SETTING_FORMAT_EMPH_WITH_UNDERSCORES,
                                               QVariant(DEF_FORMAT_EMPH_WITH_UNDERSCORES)).toBool();
    editor->setFormatEmphasisWithUnderscores(emphWithUnderscores);
    bool strongWithUnderscores = settings->value(SETTING_FORMAT_STRONG_WITH_UNDERSCORES,
                                                 QVariant(DEF_FORMAT_STRONG_WITH_UNDERSCORES)).toBool();
    editor->setFormatStrongWithUnderscores(strongWithUnderscores);
}

void MainWindow::showPreferences()
{
    // Fixes problem that appeared in Qt 5 where all the widgets
    // in the prefs window would appear disabled (grayed out) if
    // it is opened when the main window is _not_ focused.
    // The Qt window activation/focus methods did not properly
    // make the window into a key window, so let's go into Cocoa
    // land:
#ifdef Q_OS_MAC
    [[((NSView*)this->winId()) window] makeKeyAndOrderFront:nil];
#endif

    preferencesDialog->setModal(true);
    preferencesDialog->showMaximized();
}

void MainWindow::preferencesUpdated()
{
    applyPersistedFontInfo();
    applyHighlighterPreferences();
    applyEditorPreferences();
    highlighter->highlightNow();
}

bool MainWindow::isDirty()
{
    return editor->document()->isModified();
}

void MainWindow::setDirty(bool value)
{
    editor->document()->setModified(value);
    setWindowFilePath(openFilePath);
    setWindowModified(value);
}


void MainWindow::setupEditor()
{
    editor = new QMDTextEdit;
    editor->setAnchorClickKeyboardModifiers(Qt::ControlModifier);
    highlighter = new HGMarkdownHighlighter(editor->document());

    applyPersistedFontInfo();
    applyHighlighterPreferences();
    applyEditorPreferences();
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    openFile(action->data().toString());
}

void MainWindow::showRecentFileSearchDialog()
{
    QStringList recentFiles = settings->value(SETTING_RECENT_FILES).toStringList();
    QStringList otherRecents;
    foreach (QString path, recentFiles) {
        if (QFileInfo(path).absoluteFilePath() != QFileInfo(openFilePath).absoluteFilePath())
            otherRecents.insert(otherRecents.count(), path);
    }
    fileSearchDialog->setWindowTitle(tr("选择最近使用过的文件"));
    fileSearchDialog->resetWithFilePaths(otherRecents);
    fileSearchDialog->show();
}

void MainWindow::showNotesFolderFileSearchDialog()
{
    QVariant notesFolderSetting = settings->value(SETTING_NOTES_FOLDER);
    if (notesFolderSetting.isNull() || notesFolderSetting.toString().isEmpty())
    {
        QMessageBox::information(this, tr("笔记文件夹未设置"),
                                 tr("要从笔记文件夹打开文件，请先到偏好设置中指定笔记文件夹。"));
        return;
    }

    QString notesFolderPath = notesFolderSetting.toString();
    if (!QFile(notesFolderPath).exists())
    {
        QMessageBox::warning(this, tr("笔记文件夹未找到"),
                             tr("未找到您设置的笔记文件夹，请重新指定。"));
        return;
    }

    QStringList fileNames = QDir(notesFolderPath).entryList(getMarkdownFilesFilterList());

    QStringList filePaths;
    foreach (QString fileName, fileNames)
    {
        filePaths.append(notesFolderPath + QDir::separator() + fileName);
    }

    fileSearchDialog->setWindowTitle(tr("选择笔记文件夹中的文件"));
    fileSearchDialog->resetWithFilePaths(filePaths);
    fileSearchDialog->show();
}

void MainWindow::fileSearchDialogSelectedFilePath(QString path)
{
    openFile(path);
}

QString getTempHTMLFilePathForMarkdownFilePath(QString markdownFilePath)
{
    QString tempDirPath = QDir::tempPath();
    QString tempFileExtension = ".html";

    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!markdownFilePath.isNull())
        hash.addData(markdownFilePath.toUtf8());
    else
        hash.addData("Untitled");
    QString tempFileNameBase = "QMD-" + hash.result().toHex();
    QString tempFilePath = tempDirPath + QDir::separator()
                           + tempFileNameBase + tempFileExtension;
    if (QFile::exists(tempFilePath))
        QFile::remove(tempFilePath);
    return tempFilePath;
}

void MainWindow::compileToTempHTML()
{
    QString tempFilePath = getTempHTMLFilePathForMarkdownFilePath(openFilePath);
    if (compileToHTMLFile(tempFilePath))
        QDesktopServices::openUrl(QUrl("file:///" + tempFilePath));
}
void MainWindow::compileToHTMLAs()
{
    QString saveFilePath = getPathFromFileDialog(CompilationOutputDialog);
    if (saveFilePath.isNull())
        return;

    bool openAfterCompiling = settings->value(SETTING_OPEN_TARGET_AFTER_COMPILING,
                                              DEF_OPEN_TARGET_AFTER_COMPILING).toBool();
    if (compileToHTMLFile(saveFilePath) && openAfterCompiling)
        QDesktopServices::openUrl(QUrl("file:///" + saveFilePath));
}
void MainWindow::recompileToHTML()
{
    if (lastCompileTargetPath.isNull())
        return;
    compileToHTMLFile(lastCompileTargetPath);
}

bool MainWindow::compileToHTMLFile(QString targetPath)
{
    QString compilerPath = settings->value(SETTING_COMPILER,
                                           QVariant(DEF_COMPILER)).toString();
    if (!QFile::exists(compilerPath)) {
        QMessageBox::warning(this, tr("无法编译"),
                             tr("无法在该位置找到 Markdown To HTML 编译器:\n'%1'").arg(compilerPath));
        return false;
    }
    statusBar()->showMessage("正在编译到 " + targetPath + "...");
    bool success = compiler->compileToHTMLFile(compilerPath, editor->toPlainText(),
                                               targetPath);
    recompileAction->setEnabled(true);
    if (success)
    {
        lastCompileTargetPath = targetPath;
        statusBar()->showMessage(tr("编译成功: %1").arg(targetPath), 3000);
    }
    else
    {
        QString cleanCompilerPath = compiler->getUserReadableCompilerName(compilerPath);
        QString message = tr("编译失败:\n%1").arg(cleanCompilerPath);
        if (!compiler->errorString().isNull())
            message += "\n\n" + compiler->errorString();
        QMessageBox::warning(this, tr("编译失败"), message);
        statusBar()->showMessage(tr("编译失败: %1").arg(targetPath), 3000);
    }
    return success;
}


void MainWindow::formatSelectionEmphasized()
{
    editor->toggleFormattingForCurrentSelection(QMDTextEdit::Emphasized);
}
void MainWindow::formatSelectionStrong()
{
    editor->toggleFormattingForCurrentSelection(QMDTextEdit::Strong);
}
void MainWindow::formatSelectionCode()
{
    editor->toggleFormattingForCurrentSelection(QMDTextEdit::Code);
}



void MainWindow::updateRecentFilesMenu()
{
    for (int i = 0; i < recentFilesMenuActions->count(); i++)
    {
        QAction *action = recentFilesMenuActions->at(i);
        disconnect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        delete action;
    }
    recentFilesMenuActions->clear();

    recentFilesMenu->clear();
    QStringList recentFiles = settings->value(SETTING_RECENT_FILES).toStringList();
    foreach (QString recentFilePath, recentFiles)
    {
        if (!openFilePath.isEmpty() && openFilePath == recentFilePath)
            continue;
        QAction *action = new QAction(this);
        action->setText(QFileInfo(recentFilePath).fileName());
        action->setToolTip(recentFilePath);
        action->setStatusTip(recentFilePath);
        action->setData(recentFilePath);
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        recentFilesMenuActions->append(action);
        recentFilesMenu->addAction(action);
    }

    switchToPreviousFileAction->setEnabled(0 < recentFilesMenuActions->count());
}

void MainWindow::setupFileMenu()
{
    QMenu *fileMenu = new QMenu(tr("文件"), this);
    menuBar()->addMenu(fileMenu);
    fileMenu->addAction(tr("新建"), this, SLOT(newFile()),
                        QKeySequence::New);
    fileMenu->addAction(tr("打开..."), this, SLOT(openFile()),
                        QKeySequence::Open);
    recentFilesMenu = new QMenu(tr("打开最近文件..."), this);
    fileMenu->addMenu(recentFilesMenu);
    switchToPreviousFileAction = fileMenu->addAction(tr("切换到上一个文件"),
                                                     this, SLOT(switchToPreviousFile()),
                                                     QKeySequence("Ctrl+Shift+P"));
    switchToPreviousFileAction->setEnabled(false);
    fileMenu->addAction(tr("切换到最近文件..."),
                        this, SLOT(showRecentFileSearchDialog()),
                        QKeySequence("Ctrl+Shift+O"));
    fileMenu->addAction(tr("切换到笔记文件夹中的文件"),
                        this, SLOT(showNotesFolderFileSearchDialog()),
                        QKeySequence("Ctrl+Shift+N"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("保存"), this, SLOT(saveMenuItemHandler()),
                        QKeySequence::Save);
    fileMenu->addAction(tr("另存为..."), this, SLOT(saveAsMenuItemHandler()),
                        QKeySequence::SaveAs);
    revertToSavedMenuAction = fileMenu->addAction(tr("舍弃更改"), this,
                                                  SLOT(revertToSaved()));
    revertToSavedMenuAction->setEnabled(false);
    fileMenu->addSeparator();
    revealFileAction = fileMenu->addAction(
            #ifdef Q_OS_MAC
                tr("在 Finder 中显示"),
            #elif defined(Q_OS_WIN)
                tr("在资源管理器中显示"),
            #else
                tr("在文件系统中显示"),
            #endif
            this, SLOT(revealFileDir()));
    revealFileAction->setEnabled(false);
    fileMenu->addAction(tr("退出"), this, SLOT(quitActionHandler()),
                        QKeySequence::Quit);

    QMenu *editMenu = new QMenu(tr("编辑"), this);
    menuBar()->addMenu(editMenu);
    editMenu->addAction(tr("查找..."), this, SLOT(selectTextToSearchFor()),
                        QKeySequence::Find);
    findNextMenuAction = editMenu->addAction(tr("查找下一个"),
                                             this, SLOT(findNextSearchMatch()),
                                             QKeySequence::FindNext);
    findPreviousMenuAction = editMenu->addAction(tr("查找上一个"),
                                                 this, SLOT(findPreviousSearchMatch()),
                                                 QKeySequence::FindPrevious);
    findNextMenuAction->setEnabled(false);
    findPreviousMenuAction->setEnabled(false);

    QMenu *formattingMenu = new QMenu(tr("格式"), this);
    menuBar()->addMenu(formattingMenu);
    formattingMenu->addAction(tr("强调（Emphasized）"), this, SLOT(formatSelectionEmphasized()),
                              QKeySequence("Ctrl+I"));
    formattingMenu->addAction(tr("重点（Strong）"), this, SLOT(formatSelectionStrong()),
                              QKeySequence("Ctrl+B"));
    formattingMenu->addAction(tr("代码（Code）"), this, SLOT(formatSelectionCode()),
                              QKeySequence("Ctrl+D"));

    QMenu *toolsMenu = new QMenu(tr("工具"), this);
    menuBar()->addMenu(toolsMenu);
    toolsMenu->addAction(tr("增大字体"), this, SLOT(increaseFontSize()),
                         QKeySequence("Ctrl++"));
    toolsMenu->addAction(tr("缩小字体"), this, SLOT(decreaseFontSize()),
                         QKeySequence("Ctrl+-"));
    toolsMenu->addAction(tr("偏好设置..."), this, SLOT(showPreferences()),
                         QKeySequence::Preferences);

    QMenu *compilingMenu = new QMenu(tr("编译"), this);
    menuBar()->addMenu(compilingMenu);
    compilingMenu->addAction(tr("编译到临时文件夹"),
                             this, SLOT(compileToTempHTML()),
                             QKeySequence("Ctrl+T"));
    compilingMenu->addAction(tr("编译为 HTML 文件..."),
                             this, SLOT(compileToHTMLAs()),
                             QKeySequence("Ctrl+Shift+T"));
    recompileAction = compilingMenu->addAction(tr("重新编译"),
                                               this, SLOT(recompileToHTML()),
                                               QKeySequence("Ctrl+Return"));
    recompileAction->setEnabled(false);

    QMenu *helpMenu = new QMenu(tr("帮助"), this);
    menuBar()->addMenu(helpMenu);
    helpMenu->addAction(tr("关于 %1").arg(QCoreApplication::applicationName()),
                        this, SLOT(about()));
    helpMenu->addAction(tr("检查更新..."), this, SLOT(checkForUpdates()));

    updateRecentFilesMenu();
}

void MainWindow::performStartupTasks()
{
    //updateCheck->handleAppStartup();

    bool rememberLastFile = settings->value(SETTING_REMEMBER_LAST_FILE,
                                            QVariant(DEF_REMEMBER_LAST_FILE)).toBool();
    if (rememberLastFile && settings->contains(SETTING_LAST_FILE) && openFilePath.isNull())
        openFile(settings->value(SETTING_LAST_FILE).toString());

    connect(qApp, SIGNAL(commitDataRequest(QSessionManager&)),
            this, SLOT(commitDataHandler(QSessionManager&)), Qt::DirectConnection);
    connect(qApp, SIGNAL(aboutToQuit()),
            this, SLOT(aboutToQuitHandler()), Qt::DirectConnection);

    connect(editor->document(), SIGNAL(contentsChange(int,int,int)),
            this, SLOT(handleContentsChange(int,int,int)));
    connect(editor, SIGNAL(anchorClicked(QUrl)),
            this, SLOT(anchorClicked(QUrl)));
    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editor, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(handleCustomContextMenuRequest(QPoint)));
    connect(preferencesDialog, SIGNAL(updated()),
            this, SLOT(preferencesUpdated()));
    connect(fileSearchDialog, SIGNAL(selectedFilePath(QString)),
            this, SLOT(fileSearchDialogSelectedFilePath(QString)));
}

void MainWindow::reportStyleParsingErrors(QList<QPair<int, QString> > *list)
{
    QString msg;
    for (int i = 0; i < list->size(); i++)
        msg += tr("-- Line %1: %2\n").arg(list->at(i).first).arg(list->at(i).second);
    QMessageBox::warning(this, tr("解析样式文件出错"), msg);
}

void MainWindow::anchorClicked(const QUrl &link)
{
    QDesktopServices::openUrl(link);
}

void MainWindow::handleCustomContextMenuRequest(QPoint point)
{
    if (editor->getSelectedText().trimmed().isEmpty())
    {
        QTextCursor clickedPosCursor = editor->selectWordUnderCursor(editor->cursorForPosition(point));
        editor->setTextCursor(clickedPosCursor);
    }

    QMenu *menu = editor->createStandardContextMenu();
#ifdef Q_OS_MAC
    if ([(NSView*)editor->winId() respondsToSelector:@selector(showDefinitionForAttributedString:range:options:baselineOriginProvider:)]
            && !editor->getSelectedText().trimmed().isEmpty()
            )
    {
        QAction *a = menu->addAction(tr("在词典中查找 “%1”").arg(editor->getSelectedText()),
                                     this, SLOT(lookupInDictionary()));
        menu->removeAction(a);
        menu->insertAction(menu->actions().at(0), a);
        menu->insertSeparator(menu->actions().at(1));
    }
#endif
    menu->exec(editor->mapToGlobal(point));
    delete menu;
}

void MainWindow::lookupInDictionary()
{
#ifdef Q_OS_MAC
    NSView *editorNSView = (NSView*)editor->winId();
    if (![editorNSView respondsToSelector:@selector(showDefinitionForAttributedString:range:options:baselineOriginProvider:)])
        return;

    NSFont *nsFont = [NSFont fontWithName:@((char *)editor->font().family().toUtf8().data()) size:editor->font().pointSizeF()];
    NSAttributedString *as = [[[NSAttributedString alloc] initWithString:@(editor->getSelectedText().toUtf8().data()) attributes:@{NSFontAttributeName:nsFont}] autorelease];

    [editorNSView
        showDefinitionForAttributedString:as
        range:NSMakeRange(0,0) // full range
        options:@{NSDefinitionPresentationTypeKey:NSDefinitionPresentationTypeOverlay}
        baselineOriginProvider:^NSPoint(NSRange adjustedRange){
            // Here we must return a "baseline origin for the first character"
            QPoint baselinePoint = editor->getSelectionStartBaselinePoint();
            return NSMakePoint(baselinePoint.x(), baselinePoint.y());
        }];
#endif
}




QMessageBox::ButtonRole MainWindow::offerToSaveChangesIfNecessary()
{
    if (!isDirty())
        return QMessageBox::InvalidRole;

    QString fileBaseName = kUntitledFileUIName;
    bool weHaveSavePath = false;
    if (!openFilePath.isNull())
    {
        fileBaseName = QFileInfo(openFilePath).fileName();
        weHaveSavePath = true;
    }

    QMessageBox saveConfirmMessageBox(this);
    saveConfirmMessageBox.setWindowModality(Qt::WindowModal);
    saveConfirmMessageBox.setIcon(QMessageBox::Warning);
    saveConfirmMessageBox.setText(tr("“%1”已被修改过，你想要保存吗？").arg(fileBaseName));
    saveConfirmMessageBox.setInformativeText(tr("如果不保存，所做的更改将会丢失。"));
    saveConfirmMessageBox.setDefaultButton(saveConfirmMessageBox.addButton(weHaveSavePath ? tr("保存") : tr("保存..."), QMessageBox::AcceptRole));
    saveConfirmMessageBox.addButton(tr("取消"), QMessageBox::RejectRole);
    saveConfirmMessageBox.addButton(tr("不保存"), QMessageBox::DestructiveRole);
    saveConfirmMessageBox.exec();

    QMessageBox::ButtonRole selectedButtonRole = saveConfirmMessageBox.buttonRole(saveConfirmMessageBox.clickedButton());
    if (selectedButtonRole == QMessageBox::AcceptRole)
        saveCurrentFile();

    return selectedButtonRole;
}

bool MainWindow::confirmQuit(bool interactionAllowed)
{
    if (!isDirty())
        return true;

    discardingChangesOnQuit = false;

    if (!interactionAllowed)
    {
        Logger::debug("interaction not allowed -- saving.");
        saveCurrentFile();
        return true;
    }

    Logger::debug("allows interaction.");

    QMessageBox::ButtonRole selectedButtonRole = offerToSaveChangesIfNecessary();
    if (selectedButtonRole == QMessageBox::RejectRole)
        return false;
    else if (selectedButtonRole == QMessageBox::DestructiveRole)
        discardingChangesOnQuit = true;

    return true;
}

void MainWindow::commitDataHandler(QSessionManager &manager)
{
    Logger::debug("commitDataHandler.");

    bool interactionAllowed = manager.allowsInteraction();
    bool okToQuit = confirmQuit(interactionAllowed);
    if (interactionAllowed)
        manager.release();
    if (!okToQuit)
        manager.cancel();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    bool okToQuit = confirmQuit(true);
    if (okToQuit)
        event->accept();
    else
        event->ignore();
}

#ifdef Q_OS_MAC
void MainWindow::cocoaCommitDataHandler()
{
    Logger::debug("cocoaCommitDataHandler.");

    bool okToQuit = confirmQuit(true);
    if (okToQuit)
    {
        [[NSApp delegate] performSelector:@selector(acceptPendingTermination)];
        qApp->quit();
    }
    else
        [[NSApp delegate] performSelector:@selector(cancelPendingTermination)];
}
#endif

void MainWindow::quitActionHandler()
{
    Logger::debug("quitActionHandler.");

    saveCurrentFileViewPositions();

    bool okToQuit = confirmQuit(true);
    if (okToQuit)
        qApp->quit();
}

void MainWindow::aboutToQuitHandler()
{
    // No user interaction allowed here
    bool rememberWindow = settings->value(SETTING_REMEMBER_WINDOW,
                                          QVariant(DEF_REMEMBER_WINDOW)).toBool();
    if (rememberWindow) {
        settings->setValue(SETTING_WINDOW_GEOMETRY, saveGeometry());
        settings->setValue(SETTING_WINDOW_STATE, saveState());
    }
    settings->sync();

    // If we still have uncommitted changes at this point, and the user
    // has now chosen to discard them, just play it safe and save them:
    if (isDirty() && !discardingChangesOnQuit)
        saveCurrentFile();
}

void MainWindow::handleContentsChange(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position); Q_UNUSED(charsRemoved); Q_UNUSED(charsAdded);
    setDirty(true);
}
