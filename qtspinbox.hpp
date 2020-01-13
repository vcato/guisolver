#ifndef QTSPINBOX_HPP_
#define QTSPINBOX_HPP_

#include <functional>
#include <QSpinBox>
#include <QDoubleSpinBox>


using QtSpinBoxBase = QAbstractSpinBox;


class QtSpinBox
: public QtSpinBoxBase
{
  Q_OBJECT
  using Base = QtSpinBoxBase;

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
    void setSingleStep(double);
    double value() const;
    int decimals() const;
    void setDecimals(int);

  public slots:
    void valueChangedSlot(double);
    void selectTextSlot();

  private:
    bool _ignore_signals = false;
    double _single_step = 1;
    double _minimum = 0;
    double _maximum = 99.99;
    int _decimals = 2;
    mutable double _value = 0;

    void mousePressEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    StepEnabled stepEnabled() const override;
    void keyPressEvent(QKeyEvent *) override;
    void stepBy(int) override;
    QValidator::State validate(QString &input, int &pos) const override;
    void _updateLineEdit();
    double _clamped(double value) const;
    void _callValueChangedFunction(double value) const;
};


#endif /* QTSPINBOX_HPP_ */
