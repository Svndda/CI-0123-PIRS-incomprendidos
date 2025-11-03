#include "machinespage.h"
#include "ui_machinespage.h"

MachinesPage::MachinesPage(QWidget *parent, Model &model)
    : Page(parent, model)
    , ui(new Ui::MachinesPage) {
  ui->setupUi(this);
  this->drawArduinesTable();
}

MachinesPage::~MachinesPage() {
  delete ui;
}


void MachinesPage::drawArduinesTable() {
  QTableWidget* table = this->ui->arduinos_table;
  table->setRowCount(0);
  
  const int row = table->rowCount();
  table->insertRow(row);

  QVector<QTableWidgetItem*> rowFields = this->arduineDeserialization();
  for (size_t i = 0; i < rowFields.size(); ++i) {
      table->setItem(row, i, rowFields[i]);
  }
  // Resize columns to fit their content first
  table->resizeColumnsToContents();
  
  // Then make columns stretch to fill all available width
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  
  // // Optional: Adjust row height automatically
  // table->resizeRowsToContents();
  
  // Optional: Disable word wrapping for cleaner layout
  table->setWordWrap(false);
}

QVector<QTableWidgetItem*> MachinesPage::arduineDeserialization() {
  QVector<QTableWidgetItem*> fields;
  
  auto deserializeToItem = [](auto value) {
    QTableWidgetItem* item = nullptr;
    
    // Check if the value is an integer type at compile-time
    if constexpr (std::is_integral_v<decltype(value)>) {
      item = new QTableWidgetItem(QString::number(value));
    } else {
      item = new QTableWidgetItem(QString(value));
    }
    item->setTextAlignment(Qt::AlignCenter);    
    return item;
  };
  
  fields.emplace_back(deserializeToItem("0001"));
  fields.emplace_back(deserializeToItem("Arduino " + QString::number(0001)));
  fields.emplace_back(deserializeToItem("Piso 3"));
  fields.emplace_back(deserializeToItem("10.1.35.24"));
  
  return fields;
}
