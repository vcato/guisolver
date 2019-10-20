#ifndef QTSPINBOX_HPP_
#define QTSPINBOX_HPP_

#include <functional>
#include <QSpinBox>
#include <QDoubleSpinBox>


class QtSpinBox
: public QDoubleSpinBox
{
  Q_OBJECT

  public:
    typedef float Value;

    QtSpinBox();

    std::function<void(Value)> value_changed_function;
    void setValue(Value);
    void setMinimum(Value);
    void setMaximum(Value);
    void setValue(int) = delete;
    void setMinimum(int) = delete;
    void setMaximum(int) = delete;

  public slots:
    void valueChangedSlot(double);

  private:
    bool ignore_signals = false;

    void wheelEvent(QWheelEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
};


#endif /* QTSPINBOX_HPP_ */
