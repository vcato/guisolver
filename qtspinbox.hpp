#ifndef QTSPINBOX_HPP_
#define QTSPINBOX_HPP_

#include <functional>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "optional.hpp"


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
    std::function<Optional<Value>(const std::string &)> evaluate_function;
    void setValue(Value);
    void setMinimum(Value);
    void setMaximum(Value);
    void setValue(int) = delete;
    void setMinimum(int) = delete;
    void setMaximum(int) = delete;
    void setSingleStep(Value);
    double value() const;
    int decimals() const;
    void setDecimals(int);

  public slots:
    void valueChangedSlot(double);
    void selectTextSlot();
    void editingFinishedSlot();

  private:
    bool _ignore_signals = false;
    double _single_step = 1;
    double _minimum = 0;
    double _maximum = 99.99;
    int _decimals = 2;
    double _value = 0;
    bool _is_being_edited = false;
    mutable bool _evaluating = false;
    std::string _input;

    void mousePressEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    StepEnabled stepEnabled() const override;
    bool event(QEvent *event_ptr) override;
    void keyPressEvent(QKeyEvent *) override;
    void stepBy(int) override;
    QValidator::State validate(QString &input, int &pos) const override;
    void _setLineEditTextToValue();
    void _updateLineEdit();
    double _clamped(double value) const;
    void _callValueChangedFunction(double value) const;
    void _updateButtons();
    bool _inputIsExpression() const;
    Optional<Value> _evaluateInput(const std::string &) const;
};


#endif /* QTSPINBOX_HPP_ */
