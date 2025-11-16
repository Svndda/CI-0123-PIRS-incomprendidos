#ifndef PAGE_H
#define PAGE_H

#include "model/model.h"
#include <QWidget>
#include <QPainter>

class Page : public QWidget {
  Q_OBJECT
  
public:
  enum LogType : uint64_t {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    CRITICAL = 3
  };
  
protected:
  Model& model;
  
protected:
  explicit Page(QWidget *parent, Model& appModel);
  
  void logger(const LogType, const QString& message);
  void logger(const LogType, const std::string& message);
  void logger(const LogType, const char* message);
  
  void warningMessageBox(const QString& message);
  void warningMessageBox(const std::string& message);
  void warningMessageBox(const char* message);
  
  void infoMessageBox(const QString& message);
  void infoMessageBox(const std::string& message);
  void infoMessageBox(const char* message);
  
  bool askUserConfirmation(const QString& message);
  
signals:
};

#endif // PAGE_H
