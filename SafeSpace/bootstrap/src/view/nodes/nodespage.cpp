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
  
  auto table = ui->nodes_table;
  table->setRowCount(0);
  table->setColumnCount(5);
  table->setHorizontalHeaderLabels({"ID","Node","IP:Port","Status","Actions"});
  
  for (int row = 0; row < nodes.size(); ++row) {
    table->insertRow(row);
    
    table->setItem(row, 0, new QTableWidgetItem(QString::number(nodes[row].id)));
    table->setItem(row, 1, new QTableWidgetItem(nodes[row].name));
    table->setItem(row, 2, new QTableWidgetItem(nodes[row].ip + ":" + QString::number(nodes[row].port)));
    table->setItem(row, 3, new QTableWidgetItem(nodes[row].status));
    
    QWidget* actionWidget = new QWidget(table);
    QHBoxLayout* layout = new QHBoxLayout(actionWidget);
    layout->setContentsMargins(5,5,5,5);
    layout->setSpacing(10);
    
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
  auto t = ui->nodes_table;
  t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  t->verticalHeader()->setDefaultSectionSize(40);  // increases row height
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
  
  // TODO: implement model.shutdownNode(...)
  this->model.stopNode(nodes[row].id);
  
  ui->nodes_table->setItem(row, 3, new QTableWidgetItem("Stopped"));
}

/**
 * @brief Slot executed when start button is clicked.
 * @param row Index of affected node row.
 */
void NodesPage::onStartNodeClicked(int row) {
  qDebug() << "[NodesPage] Start clicked on row:" << row;
  
  // TODO: implement model.startNode(...)
  this->model.runNode(nodes[row].id);
  
  ui->nodes_table->setItem(row, 3, new QTableWidgetItem("Running"));
}
