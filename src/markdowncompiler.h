#ifndef MARKDOWNCOMPILER_H
#define MARKDOWNCOMPILER_H

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QSettings>

class MarkdownCompiler : public QObject
{
    Q_OBJECT
public:
    explicit MarkdownCompiler(QSettings *appSettings, QObject *parent = 0);
    ~MarkdownCompiler();

    QPair<QString, QString> executeCompiler(QString compilerPath, QString input, QStringList compilerArgsList);
    QPair<QString, QString> compileSynchronously(QString input, QString compilerPath, bool useDefaultArguments = false);
    bool compileToHTMLFile(QString compilerPath, QString input, QString targetPath);
    QString getUserReadableCompilerName(QString compilerPath);
    QString errorString();
    QString getHTMLTemplate();
    QString wrapHTMLContentInTemplate(QString htmlContent);
    QString getSavedArgsForCompiler(QString compilerPath);
    QStringList getArgsListForCompiler(QString compilerPath, bool useDefaultArguments = false);

private:
    QSettings *settings;
    QProcess *compilerProcess;
    QString _errorString;
    QString getFilesystemPathForResourcePath(QString resourcePath);

signals:

public slots:

};

#endif // MARKDOWNCOMPILER_H
