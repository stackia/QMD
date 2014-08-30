#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore/QObject>
#include <QtCore/QHash>

class Logger : public QObject
{
    Q_OBJECT
public:
    enum LogLevel
    {
        InfoLevel,
        DebugLevel,
        WarningLevel,
        CriticalLevel
    };

    static void log(Logger::LogLevel level, QString message);
    static void info(QString message);
    static void debug(QString message);
    static void warning(QString message);
    static void critical(QString message);

    static void setLevelEnabled(Logger::LogLevel level, bool isEnabled);
    static void setAllLevelsEnabled(bool isEnabled);
    static bool levelIsEnabled(Logger::LogLevel level);

private:
    explicit Logger(QObject *parent = 0);

signals:

public slots:

};

#endif // LOGGER_H
