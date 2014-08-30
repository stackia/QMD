#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "defines.h"
#include "QMDapplication.h"
#include "markdowncompiler.h"
#include "logger.h"
//#include "updatecheck/updatecheck.h"

#include <QtWidgets/QFontDialog>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QDesktopServices>
#include <QtCore/QTextStream>

PreferencesDialog::PreferencesDialog(QSettings *appSettings,
                                     MarkdownCompiler *aCompiler,
                                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    settings = appSettings;
    compiler = aCompiler;
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);

    stylesComboBoxModel = new QStandardItemModel();
    ui->stylesComboBox->setModel(stylesComboBoxModel);
    compilersComboBoxModel = new QStandardItemModel();
    ui->compilersComboBox->setModel(compilersComboBoxModel);

    ui->openStylesFolderButton->setToolTip(userStylesDir().absolutePath());
    ui->openCompilersFolderButton->setToolTip(userCompilersDir().absolutePath());
    ui->editHTMLTemplateButton->setToolTip(HTML_TEMPLATE_FILE_PATH);

    QButtonGroup *emphRadioGroup = new QButtonGroup(this);
    emphRadioGroup->addButton(ui->emphAsteriskRadioButton);
    emphRadioGroup->addButton(ui->emphUnderscoreRadioButton);
    QButtonGroup *strongRadioGroup = new QButtonGroup(this);
    strongRadioGroup->addButton(ui->strongAsteriskRadioButton);
    strongRadioGroup->addButton(ui->strongUnderscoreRadioButton);

#ifdef Q_OS_WIN
    QFont font = ui->infoLabel1->font();
    font.setPointSize(8);
    ui->infoLabel1->setFont(font);
    ui->infoLabel2->setFont(font);
    ui->infoLabel3->setFont(font);
    ui->infoLabel4->setFont(font);
    ui->infoLabel5->setFont(font);
    ui->linkInfoLabel->setFont(font);
    ui->fileExtensionsInfoLabel->setFont(font);
    ui->updateCheckInfoLabel->setFont(font);
    ui->styleInfoTextBrowser->setFont(font);
    ui->notesInfoLabel->setFont(font);
#endif

#ifdef Q_OS_MACX
    ui->linkInfoLabel->setText(tr("勾选后，可以按住 Command 键单击链接来打开。"));
#else
    ui->linkInfoLabel->setText(tr("勾选后，可以按住 Ctrl 键单击链接来打开。"));
#endif

    setupConnections();
    updateUIFromSettings();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
    delete stylesComboBoxModel;
    delete compilersComboBoxModel;
}

void PreferencesDialog::setupConnections()
{
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(rejected()));
    connect(ui->fontButton, SIGNAL(clicked()), this, SLOT(fontButtonClicked()));
    connect(ui->openStylesFolderButton, SIGNAL(clicked()),
            this, SLOT(openStylesFolderButtonClicked()));
    connect(ui->openCompilersFolderButton, SIGNAL(clicked()),
            this, SLOT(openCompilersFolderButtonClicked()));
    connect(ui->editHTMLTemplateButton, SIGNAL(clicked()),
            this, SLOT(editHTMLTemplateButtonClicked()));
    connect(ui->stylesComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(stylesComboBoxCurrentIndexChanged(int)));
    connect(ui->compilersComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(compilersComboBoxCurrentIndexChanged(int)));
    connect(ui->changeNotesFolderButton, SIGNAL(clicked()),
            this, SLOT(changeNotesFolderButtonClicked()));
}

void PreferencesDialog::setFontToLabel(QFont font)
{
    ui->fontLabel->setFont(font);
    QString sizeStr;
    if (font.pixelSize() > -1)
        sizeStr = QVariant(font.pixelSize()).toString()+" px";
    else
        sizeStr = QVariant(font.pointSize()).toString()+" pt";
    ui->fontLabel->setText(font.family()+" "+sizeStr);
}


QDir PreferencesDialog::userStylesDir()
{
    return QDir(((QMDApplication *)qApp)->applicationStoragePath()
                + "/styles/");
}
QStringList PreferencesDialog::userStyleFiles()
{
    QStringList nameFilters;
    nameFilters << "*.style";
    return userStylesDir().entryList(nameFilters);
}

QDir PreferencesDialog::userCompilersDir()
{
    return QDir(((QMDApplication *)qApp)->applicationStoragePath()
                + "/compilers/");
}
QStringList PreferencesDialog::userCompilerFiles()
{
    QStringList ret;
    QString compilersDirPath = userCompilersDir().absolutePath();
    foreach (QString fileName, userCompilersDir().entryList(QDir::Files | QDir::NoDotAndDotDot))
    {
        QString thisFullPath = compilersDirPath + QDir::separator() + fileName;
        if (QFileInfo(thisFullPath).isExecutable())
            ret << fileName;
    }
    return ret;
}


# define ADD_COMBO_LABEL(varname, name) \
    QStandardItem *varname = new QStandardItem(name);\
    varname->setSelectable(false);\
    varname->setEnabled(false);\
    varname->setForeground(QBrush(Qt::gray));\
    rootItem->appendRow(varname)
# define ADD_COMBO_ITEM(name, data, tooltip) \
    QStandardItem *item = new QStandardItem(name);\
    item->setData(QVariant(data), Qt::UserRole);\
    item->setToolTip(tooltip); \
    rootItem->appendRow(item)

void PreferencesDialog::updateStylesComboBoxFromSettings()
{
    QString selectedStylePath = settings->value(SETTING_STYLE, DEF_STYLE).toString();
    stylesComboBoxModel->clear();
    QStandardItem *rootItem = stylesComboBoxModel->invisibleRootItem();

    int indexToSelect = 1;
    int i = 0;

    ADD_COMBO_LABEL(builtinStylesLabel, tr("内置样式:"));
    i++;

    foreach (QString builtInStyleName, QDir(":/styles/").entryList())
    {
        QString builtinStyleFullPath = QDir(":/styles/" + builtInStyleName).absolutePath();
        ADD_COMBO_ITEM(builtInStyleName, builtinStyleFullPath, QString::null);
        if (builtinStyleFullPath == selectedStylePath)
            indexToSelect = i;
        i++;
    }

    QString userStylesDirPath = userStylesDir().absolutePath();
    QStringList userStyles = userStyleFiles();
    if (userStyles.length() > 0)
    {
        ADD_COMBO_LABEL(userStylesLabel, tr("用户样式:"));
        i++;

        foreach (QString userStyleFile, userStyles)
        {
            QString userStyleFullPath = QDir(userStylesDirPath + "/" + userStyleFile).absolutePath();
            ADD_COMBO_ITEM(QFileInfo(userStyleFile).baseName(), userStyleFullPath, QString::null);
            if (userStyleFullPath == selectedStylePath)
                indexToSelect = i;
            i++;
        }
    }
    ui->stylesComboBox->setCurrentIndex(indexToSelect);
}

void PreferencesDialog::updateStyleInfoTextFromComboBoxSelection()
{
    QString selectedStylePath = ui->stylesComboBox->itemData(ui->stylesComboBox->currentIndex()).toString();
    if (!QFile::exists(selectedStylePath)) {
        ui->styleInfoTextBrowser->setText(tr("<i>无法找到样式文件: %1</i>").arg(selectedStylePath));
        return;
    }

    QString stylesheetContents;
    QFile file(selectedStylePath);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        stylesheetContents = stream.readAll();
    }

    QString styleDescription;
    QStringList lines = stylesheetContents.split("\n");
    foreach (QString line, lines)
    {
        if (line.startsWith("#"))
            styleDescription += line.right(line.length() - 1).trimmed() + "\n";
        else
            break;
    }

    if (styleDescription.isEmpty())
        styleDescription = tr("<i>所选样式没有描述。</i>");
    else
    {
        QPair<QString, QString> compilationOutput = compiler->compileSynchronously(styleDescription, DEF_COMPILER, true);
        QString compiledDescription = compilationOutput.first;
        if (!compiledDescription.isNull())
            styleDescription = compiledDescription;
        else
            styleDescription = styleDescription.replace("\n", "<br/>");
    }

    ui->styleInfoTextBrowser->setHtml(styleDescription);
}

QString PreferencesDialog::versionStringForBuiltinCompiler(QString compilerPath)
{
    QStringList args;
    if (compilerPath.contains("discount"))
        args << "-V";
    else
        args << "--version";
    QPair<QString, QString> compilerVersionOutput = compiler->executeCompiler(compilerPath, QString::null, args);
    return compilerVersionOutput.first;
}

void PreferencesDialog::updateCompilersComboBoxFromSettings()
{
    QString selectedCompilerPath = settings->value(SETTING_COMPILER, DEF_COMPILER).toString();
    compilersComboBoxModel->clear();
    QStandardItem *rootItem = compilersComboBoxModel->invisibleRootItem();

    int indexToSelect = 1;
    int i = 0;

    ADD_COMBO_LABEL(builtinCompilersLabel, tr("内置编译器:"));
    i++;

    foreach (QString builtInCompilerName, QDir(":/compilers/").entryList())
    {
        QDir thisBuiltinCompilerDir = QDir(":/compilers/" + builtInCompilerName);
        QString builtinCompilerFullPath =
                thisBuiltinCompilerDir.absolutePath() + "/"
                + thisBuiltinCompilerDir.entryList(QDir::NoDotAndDotDot | QDir::Files).first();
        ADD_COMBO_ITEM(builtInCompilerName, builtinCompilerFullPath,
                       versionStringForBuiltinCompiler(builtinCompilerFullPath));
        if (builtinCompilerFullPath == selectedCompilerPath)
            indexToSelect = i;
        i++;
    }

    QString userCompilersDirPath = userCompilersDir().absolutePath();
    QStringList userCompilers = userCompilerFiles();
    if (userCompilers.length() > 0)
    {
        ADD_COMBO_LABEL(userCompilersLabel, tr("用户编译器:"));
        i++;

        foreach (QString userCompilerFileName, userCompilers)
        {
            QString userCompilerFullPath = QDir(userCompilersDirPath + QDir::separator() + userCompilerFileName).absolutePath();
            ADD_COMBO_ITEM(QFileInfo(userCompilerFileName).baseName(),
                           userCompilerFullPath, QString::null);
            if (userCompilerFullPath == selectedCompilerPath)
                indexToSelect = i;
            i++;
        }
    }
    ui->compilersComboBox->setCurrentIndex(indexToSelect);
}


void PreferencesDialog::updateCompilerArgsFieldFromComboBoxSelection()
{
    QString selectedCompilerPath = ui->compilersComboBox->itemData(ui->compilersComboBox->currentIndex()).toString();
    ui->compilerArgsField->setText(compiler->getSavedArgsForCompiler(selectedCompilerPath));
}



// Some helper macros
#define PREF_TO_UI_STRING(pref, def, elem) elem->setText(settings->value(pref, QVariant(def)).toString())
#define PREF_TO_UI_INT(pref, def, elem) elem->setValue(settings->value(pref, QVariant(def)).toInt())
#define PREF_TO_UI_DOUBLE(pref, def, elem) elem->setValue(settings->value(pref, QVariant(def)).toDouble())
#define PREF_TO_UI_BOOL_CHECKBOX(pref, def, elem) elem->setChecked(settings->value(pref, QVariant(def)).toBool())
#define PREF_TO_UI_BOOL_INVERT_CHECKBOX(pref, def, elem) elem->setChecked(!settings->value(pref, QVariant(def)).toBool())

void PreferencesDialog::updateUIFromSettings()
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
    setFontToLabel(font);

    // styles
    updateStylesComboBoxFromSettings();
    updateStyleInfoTextFromComboBoxSelection();

    // compilers
    updateCompilersComboBoxFromSettings();
    updateCompilerArgsFieldFromComboBoxSelection();

    // others
    PREF_TO_UI_INT(SETTING_TAB_WIDTH, DEF_TAB_WIDTH, ui->tabWidthSpinBox);
    PREF_TO_UI_BOOL_INVERT_CHECKBOX(SETTING_INDENT_WITH_TABS, DEF_INDENT_WITH_TABS, ui->tabsWithSpacesCheckBox);
    PREF_TO_UI_DOUBLE(SETTING_HIGHLIGHT_INTERVAL, DEF_HIGHLIGHT_INTERVAL, ui->highlightIntervalSpinBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_REMEMBER_LAST_FILE, DEF_REMEMBER_LAST_FILE, ui->rememberLastFileCheckBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_CLICKABLE_LINKS, DEF_CLICKABLE_LINKS, ui->linksClickableCheckBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_HIGHLIGHT_CURRENT_LINE, DEF_HIGHLIGHT_CURRENT_LINE, ui->highlightLineCheckBox);
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_OPEN_TARGET_AFTER_COMPILING, DEF_OPEN_TARGET_AFTER_COMPILING, ui->openTargetAfterCompilingCheckBox);
    PREF_TO_UI_STRING(SETTING_EXTENSIONS, DEF_EXTENSIONS, ui->extensionsLineEdit);
    PREF_TO_UI_STRING(SETTING_NOTES_FOLDER, "", ui->notesFolderLineEdit);

    PREF_TO_UI_BOOL_CHECKBOX(SETTING_FORMAT_EMPH_WITH_UNDERSCORES, DEF_FORMAT_EMPH_WITH_UNDERSCORES, ui->emphUnderscoreRadioButton);
    ui->emphAsteriskRadioButton->setChecked(!ui->emphUnderscoreRadioButton->isChecked());
    PREF_TO_UI_BOOL_CHECKBOX(SETTING_FORMAT_STRONG_WITH_UNDERSCORES, DEF_FORMAT_STRONG_WITH_UNDERSCORES, ui->strongUnderscoreRadioButton);
    ui->strongAsteriskRadioButton->setChecked(!ui->strongUnderscoreRadioButton->isChecked());

    ui->checkForUpdatesCheckBox->setChecked(false);
    //ui->checkForUpdatesCheckBox->setChecked(HGUpdateCheck::shouldCheckForUpdatesOnStartup());
}

void PreferencesDialog::updateSettingsFromUI()
{
    settings->setValue(SETTING_FONT, ui->fontLabel->font().toString());
    settings->setValue(SETTING_TAB_WIDTH, ui->tabWidthSpinBox->value());
    settings->setValue(SETTING_HIGHLIGHT_INTERVAL, ui->highlightIntervalSpinBox->value());
    settings->setValue(SETTING_INDENT_WITH_TABS, !ui->tabsWithSpacesCheckBox->isChecked());
    settings->setValue(SETTING_REMEMBER_LAST_FILE, ui->rememberLastFileCheckBox->isChecked());
    settings->setValue(SETTING_CLICKABLE_LINKS, ui->linksClickableCheckBox->isChecked());
    settings->setValue(SETTING_HIGHLIGHT_CURRENT_LINE, ui->highlightLineCheckBox->isChecked());
    settings->setValue(SETTING_OPEN_TARGET_AFTER_COMPILING, ui->openTargetAfterCompilingCheckBox->isChecked());
    settings->setValue(SETTING_STYLE, ui->stylesComboBox->itemData(ui->stylesComboBox->currentIndex()).toString());
    settings->setValue(SETTING_EXTENSIONS, ui->extensionsLineEdit->text());
    settings->setValue(SETTING_FORMAT_EMPH_WITH_UNDERSCORES, ui->emphUnderscoreRadioButton->isChecked());
    settings->setValue(SETTING_FORMAT_STRONG_WITH_UNDERSCORES, ui->strongUnderscoreRadioButton->isChecked());
    settings->setValue(SETTING_NOTES_FOLDER, ui->notesFolderLineEdit->text());

    QString selectedCompilerPath = ui->compilersComboBox->itemData(ui->compilersComboBox->currentIndex()).toString();
    QMap<QString, QVariant> compilerArgsMap = settings->value(SETTING_COMPILER_ARGS).toMap();
    compilerArgsMap[selectedCompilerPath] = QVariant(ui->compilerArgsField->text());

    settings->setValue(SETTING_COMPILER, selectedCompilerPath);
    settings->setValue(SETTING_COMPILER_ARGS, compilerArgsMap);

    //HGUpdateCheck::setShouldCheckForUpdatesOnStartup(ui->checkForUpdatesCheckBox->isChecked());

    settings->sync();
}

void PreferencesDialog::showEvent(QShowEvent *event)
{
    updateUIFromSettings();
    QDialog::showEvent(event);
}

void PreferencesDialog::fontButtonClicked()
{
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, ui->fontLabel->font(),
                                         this, tr("选择新字体"));
    if (!ok)
        return;
    setFontToLabel(newFont);
}


void PreferencesDialog::openPath(QString path, bool isFolder)
{
    bool couldOpen = QDesktopServices::openUrl(QUrl("file:///" + path));
    QString type = isFolder ? "folder" : "file";
    if (!couldOpen)
        QMessageBox::information(this, tr("Could not open %1").arg(type),
                                 tr("For some reason %1 could not open the %2. "
                                    "You'll have to do it manually. "
                                    "The path is:\n\n%3")
                                 .arg(QCoreApplication::applicationName())
                                 .arg(type)
                                 .arg(path));
}

void PreferencesDialog::openFolderEnsuringItExists(QString path)
{
    // Let's make sure the path exists
    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }
    openPath(path, true);
}

void PreferencesDialog::openStylesFolderButtonClicked()
{
    openFolderEnsuringItExists(userStylesDir().absolutePath());
}

void PreferencesDialog::openCompilersFolderButtonClicked()
{
    openFolderEnsuringItExists(userCompilersDir().absolutePath());
}

void PreferencesDialog::changeNotesFolderButtonClicked()
{
    QString selection = QFileDialog::getExistingDirectory(this, tr("选择笔记文件夹"));
    if (selection.isNull())
        return;
    ui->notesFolderLineEdit->setText(selection);
}

void PreferencesDialog::editHTMLTemplateButtonClicked()
{
    QString path = HTML_TEMPLATE_FILE_PATH;
    if (!QFile::exists(path))
        ((QMDApplication*)qApp)->copyResourceToFile(":/template.html", path);
    QDir dir(path);
    dir.cdUp();
    openPath(dir.absolutePath(), false);
}

void PreferencesDialog::stylesComboBoxCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateStyleInfoTextFromComboBoxSelection();
}

void PreferencesDialog::compilersComboBoxCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateCompilerArgsFieldFromComboBoxSelection();
}

void PreferencesDialog::accepted()
{
    updateSettingsFromUI();
    emit updated();
}

void PreferencesDialog::rejected()
{
    updateUIFromSettings();
}
