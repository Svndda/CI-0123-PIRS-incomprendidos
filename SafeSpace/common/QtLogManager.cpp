#include "QtLogManager.h"
#include <QDateTime>   // para timestamp
#include <QDebug>      // opcional, por si usamos qDebug dentro


QtLogManager& QtLogManager::instance(){
    static QtLogManager instance;
    return instance;
}

void QtLogManager::install(){
    qInstallMessageHandler(QtLogManager::messageHandler);
}

