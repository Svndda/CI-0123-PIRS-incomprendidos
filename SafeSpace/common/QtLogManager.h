#ifndef QTLOGMANAGER_H
#define QTLOGMANAGER_H
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMutex>
#include <QMessageLogContext>
#include <QtGlobal> 

// severity levels for logging
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Critical,
    Fatal
};

struct LogEntry {
    LogLevel level;
    QString timestamp;
    QString message;
};

class QtLogManager {
    public:
        /**
         * @brief Get the singleton instance of QtLogManager.
         * @brief Get the singleton instance of QtLogManager.
         *
         * @return QtLogManager&
         */
        static QtLogManager& instance();
        /**
         * @brief Install the custom Qt message handler.
         */
        void install();
        /**
         *
         * @return copy of historial logs
         */
        Qvector<LogEntry> getLogs() const;
        /**
         * @brief Clear all stored logs.
         */
        void clearLogs();

    private:
    /**
     * @brief Construct a new Qt Log Manager object 
     * 
     */
        QtLogManager() = default;
        
        Qvector<LogEntry> logs;
        
        mutable QMutex mutex;

        static void messageHandler(QtMsgType type,
                               const QMessageLogContext &context,
                               const QString &msg);
}


#endif // QTLOGMANAGER_H
