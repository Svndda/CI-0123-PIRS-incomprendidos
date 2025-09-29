#include "button.h"

Button::Button(QWidget* parent,
    const QString& btntext, const QString& bgColor,
    const QString& fgColor, int radius, bool border)
    : QPushButton(parent), backgroundColor(bgColor),
    textColor(fgColor), text(btntext) {
  setText(btntext);
  applyStyle();
}

void Button::applyStyle() {
  
  QString borderStyle = this->showBorder
    ? QString("border: 2px solid rgba(0, 0, 0, 20);")
    : "border: none;";
  
  QString style = QString(
    "QPushButton {"
    " border-radius: %1px;"
    " background-color: %2;"
    " color: %3;"
    " %4"
    "}"
                      
    "QPushButton:hover {"
    " background-color: rgb(90, 90, 255);"
    " color: %3;"
    "}"
                      
    "QPushButton:pressed {"
    " background-color: rgb(51, 55, 188)"
    " color: %3;"
    "}"
                      
    "QPushButton:disabled {"
    " background-color: rgb(39,39,43);"
    " color: #7f8c8d;"
    "}"
                      
    "QPushButton:checked {"
    " background-color: #27ae60;"
    " color: #ecf0f1;"
    "}"
    )
    .arg(borderRadius)
    .arg(backgroundColor)
    .arg(textColor)
    .arg(borderStyle);
                  
  this->setStyleSheet(style);
}

void Button::setBackgroundColor(const QString& bgColor) {
  this->backgroundColor = bgColor;
  this->applyStyle();
}

void Button::setTextColor(const QString& txtColor) {
  this->textColor = txtColor;
  this->applyStyle();
}

void Button::setButtonText(const QString& btnText) {
  this->text = btnText;
  this->setText(text);
  this->applyStyle();
}

void Button::setBorderRadius(int radius) {
  this->borderRadius = radius;
  this->applyStyle();
}

void Button::setShowBorder(bool enabled) {
  this->showBorder = enabled;
  this->applyStyle();
}
