#ifndef BUTTON_H
#define BUTTON_H

#include <QPushButton>

/**
 * @brief Button extends QPushButton
 * to allow easy customization of background color,
 * text color, and button text via constructor or setters.
 */
class Button : public QPushButton
{
  Q_OBJECT

private:
  QString backgroundColor;
  QString textColor;
  QString text;
  int borderRadius;
  bool showBorder;
  
  /**
   * @brief Applies the current style properties (colors, text) to the button.
   */
  void applyStyle();
  
public:
  /**
   * @param parent Optional QWidget parent
   * @brief Constructor with optional customization parameters.
   * @param text Button text
   * @param bgColor Background color in CSS format
   * @param fgColor Text color in CSS format
   * @param radius Button radius size
   * @param border Visible state of the border
   */
  explicit Button(QWidget* parent = nullptr,
                  const QString& text = "Button",
                  const QString& bgColor = "rgb(64, 68, 237)",
                  const QString& fgColor = "white",
                  int radius = 15,
                  bool border = true);
  
  ~Button() = default;
  
public:
  ///> Setters
  /**
 * @brief Set background color and reapply style.
 */
  void setBackgroundColor(const QString& bgColor);
  
  /**
 * @brief Set text color and reapply style.
 */
  void setTextColor(const QString& txtColor);
  
  /**
 * @brief Set displayed text and reapply style.
 */
  void setButtonText(const QString& btnText);
  
  /**
 * @brief Set displayed border radius and reapply style.
 */
  void setBorderRadius(int radius);
  
  /**
 * @brief Set displayed border visible status and reapply style.
 */
  void setShowBorder(bool enabled);
public:  
  ///> Getters
  QString getBackgroundColor() const {return this->backgroundColor;};
  QString getTextColor() const {return this->textColor;};
  QString getButtonText() const {return this->text;};
};

#endif // BUTTON_H
