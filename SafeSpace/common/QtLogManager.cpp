#include "QtLogManager.h"
#include <QDateTime>   // para timestamp
#include <QDebug>      // opcional, por si usamos qDebug dentro
#include <QMutexLocker>


QtLogManager& QtLogManager::instance(){
    static QtLogManager instance;
    return instance;
}

void QtLogManager::install(){
    qInstallMessageHandler(QtLogManager::messageHandler);
}

QVector<LogEntry> QtLogManager::getLogs() const {
    QMutexLocker locker(&mutex);
    return logs;
}

void QtLogManager::clearLogs(){
    QMutexLocker locker(&mutex);
    logs.clear();
}

void QtLogManager::messageHandler(QtMsgType type,
                                  const QMessageLogContext &context,
                                  const QString &msg)
{
    Q_UNUSED(context);

    LogLevel level;
    switch (type) {
    case QtDebugMsg:    level = LogLevel::Debug;    break;
    case QtInfoMsg:     level = LogLevel::Info;     break;
    case QtWarningMsg:  level = LogLevel::Warning;  break;
    case QtCriticalMsg: level = LogLevel::Critical; break;
    case QtFatalMsg:    level = LogLevel::Fatal;    break;
    }

    
    LogEntry entry;
    entry.level = level;
    entry.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    entry.message = msg;

    // Almacenar el log de manera thread-safe
    QtLogManager &logger = QtLogManager::instance();
    {
        QMutexLocker lock(&logger.mutex);
        logger.logs.append(entry);
    }
    
    if (type == QtFatalMsg) {
        abort();
    }
}
