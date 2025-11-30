#include <QHBoxLayout>
#include <QFile>
#include <QtUiTools/QUiLoader>

#include "nodespage.h"
#include "model.h"
#include "ui_nodespage.h"

NodesPage::NodesPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::NodesPage) {
  ui->setupUi(this);
  
  this->connect(this->ui->logout_button, &QPushButton::clicked,
      this, [this](){
    if (this->askUserConfirmation("¿Seguro que deseas salir?")) {
      this->model.shutdown();
      emit this->logout();
    }
  });
  
  auto makeCenteredItem = [](const QString& text) {
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
  };
  
  auto table = ui->nodes_table;
  table->setRowCount(0);
  
  const auto nodes = this->model.getNodes();
  
  for (int row = 0; row < nodes.size(); ++row) {
    table->insertRow(row);
    
    table->setItem(row, 0, makeCenteredItem(QString::number(nodes[row].id)));
    table->setItem(row, 1, makeCenteredItem(nodes[row].name));
    table->setItem(row, 2, makeCenteredItem(nodes[row].ip + ":" + QString::number(nodes[row].port)));
    
    QTableWidgetItem* statusItem = new QTableWidgetItem(nodes[row].status);
    statusItem->setTextAlignment(Qt::AlignCenter);
    applyStatusColor(statusItem, nodes[row].status);
    table->setItem(row, 3, statusItem);
    
    QWidget* actionWidget = new QWidget(table);
    QHBoxLayout* layout = new QHBoxLayout(actionWidget);
    layout->setContentsMargins(15,5,5,15);
    layout->setSpacing(30);
    
    // --- Start button (greenish colors) ---
    Button* startBtn = new Button(
        actionWidget,
        "Encender",
        "#2ecc71",   // green
        "#ffffff",
        15,
        true
        );
    
    // --- Shutdown button (reddish colors) ---
    Button* shutdownBtn = new Button(
        actionWidget,
        "Apagar",
        "#e74c3c",   // red
        "#ffffff",
        15,
        true
        );
    
    startBtn->setBorderRadius(15);
    shutdownBtn->setBorderRadius(15);
    
    startBtn->setSize(120, 60);
    shutdownBtn->setSize(120, 60);
    
    layout->addWidget(startBtn);
    layout->addWidget(shutdownBtn);
    actionWidget->setLayout(layout);
    table->setCellWidget(row, 4, actionWidget);
    
    // auto t = ui->nodes_table;
    // t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // t->verticalHeader()->setDefaultSectionSize(40);  // increases row height
    
    connect(startBtn, &QPushButton::clicked, this, [this, row]() {
      onStartNodeClicked(row);
    });
    
    connect(shutdownBtn, &QPushButton::clicked, this, [this, row]() {
      onShutdownNodeClicked(row);
    });
  }
  
  // Sin numeración y sin grilla
  table->verticalHeader()->setVisible(false);
  
  // Ajuste fino por columna
  table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); // ID
  table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // Name
  table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // IP:Port
  table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch); // Status
  table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  
  // Estilo limpio para tabla
  table->setStyleSheet(
      "QTableWidget::item { padding: 6px; }"
      );
  
  // Ajustar contenido
  table->resizeColumnsToContents();
  table->resizeRowsToContents();
  
  auto listview = this->ui->logbook_list;
  // --- Inicializar ListView de logs ---
  logModel = new QStandardItemModel(this);
  ui->logbook_list->setModel(logModel);
  ui->logbook_list->setSelectionMode(QAbstractItemView::NoSelection);
  ui->logbook_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
  
  // Conectar eventos del modelo
  connect(&this->model, &Model::networkEventLogged,
          this, &NodesPage::onNetworkEventLogged);
}

NodesPage::~NodesPage() {
  delete ui;
}

/**
 * @brief Slot executed when shutdown button is clicked.
 * @param row Index of affected node row.
 */
void NodesPage::onShutdownNodeClicked(int row) {
  // Detailed debug info
  qDebug() << "[NodesPage] Shutdown clicked on row:" << row;
  const auto nodes = this->model.getNodes();
  // TODO: implement model.shutdownNode(...)
  this->model.stopNode(nodes[row].id);
  this->updateNodeState(row, "Apagando...");
}

void NodesPage::updateNodeState(int row, QString state) {
  QTableWidgetItem* item = new QTableWidgetItem(state);
  item->setTextAlignment(Qt::AlignCenter);
  applyStatusColor(item, state);
  this->ui->nodes_table->setItem(row, 3, item);
}

int NodesPage::findRowByNodeId(int nodeId) {
  const auto nodes = this->model.getNodes();  
  for (int i = 0; i < nodes.size(); ++i) {
    if (nodes[i].id == nodeId)
      return i;
  }
  return -1;
}

/**
 * @brief Slot executed when start button is clicked.
 * @param row Index of affected node row.
 */
void NodesPage::onStartNodeClicked(int row) {
  qDebug() << "[NodesPage] Start clicked on row:" << row;
  const auto nodes = this->model.getNodes();  
  // TODO: implement model.startNode(...)
  this->model.runNode(nodes[row].id);
  this->updateNodeState(row, "Encendiendo...");
}

void NodesPage::applyStatusColor(QTableWidgetItem* item, const QString& state) {
  if (state == "Online" || state.contains("ACTIVADO")) {
    item->setBackground(QColor("#2ecc71")); // verde
    item->setForeground(Qt::black);
  } else if (state == "Offline" || state.contains("DETENIDO") || state == "Apagado") {
    item->setBackground(QColor("#e67e22")); // naranja
    item->setForeground(Qt::black);
  } else if (state == "Error" || state.contains("FALLIDA")) {
    item->setBackground(QColor("#e74c3c")); // rojo
    item->setForeground(Qt::white);
  } else if (state.contains("Encendiendo") || state.contains("Apagando")) {
    item->setBackground(QColor("#f39c12")); // amarillo - proceso en curso
    item->setForeground(Qt::black);
  } else {
    item->setBackground(Qt::white);
    item->setForeground(Qt::black);
  }
}
void NodesPage::onNetworkEventLogged(const NetworkEvent& evt) {
  addLogToListView(evt);
  
  // Extraer nodeId del mensaje si está disponible
  int nodeId = extractNodeIdFromEvent(evt);
  if (nodeId == -1) return;
  
  int row = findRowByNodeId(nodeId);
  if (row < 0) return;
  
  QString detailLower = evt.detail.toLower();
  
  // Detectar cambios de estado basados en los nuevos mensajes
  if (detailLower.contains("activado correctamente") || 
      detailLower.contains("online") ||
      (detailLower.contains("exitosa") && detailLower.contains("activación"))) {
    
    updateNodeState(row, "Online");
    
  } else if (detailLower.contains("detenido correctamente") || 
             detailLower.contains("offline") ||
             (detailLower.contains("exitosa") && detailLower.contains("detención"))) {
    
    updateNodeState(row, "Offline");
    
  } else if ((detailLower.contains("fallida") && detailLower.contains("activación")) ||
             (detailLower.contains("fallida") && detailLower.contains("detención")) ||
             detailLower.contains("error")) {
    
    updateNodeState(row, "Error");
    
  } else if (detailLower.contains("encendiendo") || detailLower.contains("apagando")) {
    // Mantener estado de "procesando"
    updateNodeState(row, evt.detail.contains("Encendiendo") ? "Encendiendo..." : "Apagando...");
  }
}

int NodesPage::extractNodeIdFromEvent(const NetworkEvent& evt) {
  // Intentar extraer nodeId del detalle del mensaje
  QString detail = evt.detail;
  
  // Buscar patrones como "Nodo X" donde X es el ID
  QRegularExpression re("Nodo\\s+(\\d+)");
  QRegularExpressionMatch match = re.match(detail);
  if (match.hasMatch()) {
    return match.captured(1).toInt();
  }
  
  // Si el evento ya tiene nodeId, usarlo
  if (evt.nodeId != -1) {
    return evt.nodeId;
  }
  
  return -1;
}

void NodesPage::addLogToListView(const NetworkEvent& evt) {
  QString line = QString("[%1] %2 → %3")
                     .arg(evt.timestamp)
                     .arg(evt.type)
                     .arg(evt.detail);
  
  QStandardItem* item = new QStandardItem(line);
  
  QString lower = evt.detail.toLower();
  
  // ---- Colorear filas según el tipo de mensaje ----
  if (lower.contains("exitosa") && 
      (lower.contains("activado") || lower.contains("detenido"))) {
    item->setBackground(QColor("#2ecc71"));  // verde - operación exitosa
    item->setForeground(Qt::black);
    
  } else if (lower.contains("fallida") && 
             (lower.contains("activación") || lower.contains("detención"))) {
    item->setBackground(QColor("#e74c3c"));  // rojo - operación fallida
    item->setForeground(Qt::white);
    
  } else if (lower.contains("envío exitoso")) {
    item->setBackground(QColor("#3498db"));  // azul - solicitud enviada
    item->setForeground(Qt::white);
    
  } else if (lower.contains("envío fallido")) {
    item->setBackground(QColor("#e67e22"));  // naranja - error de envío
    item->setForeground(Qt::black);
    
  } else {
    item->setBackground(Qt::white);          // normal
    item->setForeground(Qt::black);
  }
  
  QFont font = item->font();
  font.setPointSize(10);
  item->setFont(font);
  
  logModel->appendRow(item);
  
  // Auto-scroll al final
  ui->logbook_list->scrollToBottom();
  
  if (logModel->rowCount() > 1000) {
    logModel->removeRow(0);
  }
}
