#include <QMessageBox>
#include "page.h"

Page::Page(QWidget *parent, Model& appModel)
    : QWidget(parent), model(appModel) {
  
  this->renderer = new QSvgRenderer(
      QString(":/images/bg1.svg"), this
  );
}

void Page::logger(const LogType type, const QString& message) {
  // Logs the given message with the related function.
  switch (type) {
  case Page::DEBUG : qDebug() << message;
    break;
  case Page::INFO : qInfo() << message;
    break;
  case Page::WARNING : qWarning() << message;
    break;
  case Page::CRITICAL : qCritical() << message;
    break;
  default:
    break;
  }
}

void Page::logger(const LogType type, const std::string& message) {
  this->logger(type, QString(message.data()));
}

void Page::logger(const LogType type, const char* message) {
  this->logger(type, QString::fromUtf8(message));
}

void Page::warningMessageBox(const QString& message) {
  // Shows a warning dialog with the given message to the user.
  QMessageBox::warning(this, "Error", message);
  qWarning() << message;
}

void Page::warningMessageBox(const std::string& message) {
  this->warningMessageBox(QString(message.data()));
}

void Page::warningMessageBox(const char* message) {
  this->warningMessageBox(QString::fromUtf8(message));
}

void Page::infoMessageBox(const QString& message) {
  // Shows a info dialog with the given message to the user.  
  QMessageBox::information(this, "Aviso", message);
  qInfo() << message;
}

void Page::infoMessageBox(const char* message) {
  this->infoMessageBox(QString::fromUtf8(message));
}

bool Page::askUserConfirmation(const QString& message) {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Confirmación", message,
                                QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    return true;  // Usuario aceptó
  } else {
    return false; // Usuario denegó
  }
}
