#include "logger.h"
#include <QtCore/QDebug>

Logger::Logger(QObject *parent) :
    QObject(parent)
{
}

#ifdef BUILD_DEBUG
#define kDefaultIsEnabled true
#else
#define kDefaultIsEnabled false
#endif

static QHash<Logger::LogLevel, bool> levelsEnabled;

void Logger::setLevelEnabled(Logger::LogLevel level, bool isEnabled)
{
    levelsEnabled[level] = isEnabled;
}

void Logger::setAllLevelsEnabled(bool isEnabled)
{
    levelsEnabled[Logger::InfoLevel] = isEnabled;
    levelsEnabled[Logger::DebugLevel] = isEnabled;
    levelsEnabled[Logger::WarningLevel] = isEnabled;
    levelsEnabled[Logger::CriticalLevel] = isEnabled;
}

bool Logger::levelIsEnabled(Logger::LogLevel level)
{
    if (!levelsEnabled.contains(level))
        return kDefaultIsEnabled;
    return levelsEnabled.value(level);
}

void Logger::log(Logger::LogLevel level, QString message)
{
    if (!Logger::levelIsEnabled(level))
        return;

    switch (level)
    {
        case Logger::InfoLevel:
        case Logger::DebugLevel:
            qDebug() << message;
            break;
        case Logger::WarningLevel:
            qWarning() << message;
            break;
        case Logger::CriticalLevel:
            qCritical() << message;
            break;
    }
}

void Logger::info(QString message)
{
    Logger::log(Logger::InfoLevel, message);
}
void Logger::debug(QString message)
{
    Logger::log(Logger::DebugLevel, message);
}
void Logger::warning(QString message)
{
    Logger::log(Logger::WarningLevel, message);
}
void Logger::critical(QString message)
{
    Logger::log(Logger::CriticalLevel, message);
}
