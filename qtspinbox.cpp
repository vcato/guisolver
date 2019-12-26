#include "qtspinbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <QWheelEvent>

using std::cerr;


QtSpinBox::QtSpinBox()
{
  connect(
    this,
    SIGNAL(valueChanged(double)),
    SLOT(valueChangedSlot(double))
  );

  setSingleStep(0.1);
}


void QtSpinBox::valueChangedSlot(double value)
{
  if (ignore_signals) {
    return;
  }

  if (!value_changed_function) {
    cerr << "value_changed_function is not set.\n";
    return;
  }

  value_changed_function(value);
}


void QtSpinBox::wheelEvent(QWheelEvent *event_ptr)
{
  assert(event_ptr);

  if (!hasFocus()) {
    event_ptr->ignore();
  }
  else {
    QDoubleSpinBox::wheelEvent(event_ptr);
  }
}


void QtSpinBox::focusInEvent(QFocusEvent *)
{
  setFocusPolicy(Qt::WheelFocus);
}


void QtSpinBox::focusOutEvent(QFocusEvent *)
{
  setFocusPolicy(Qt::StrongFocus);
}


static float toleranceForPrecision(int n_digits_of_precision)
{
  if (n_digits_of_precision == 2) {
    return 0.005;
  }
  else if (n_digits_of_precision == 1) {
    return 0.05;
  }
  else {
    assert(false); // not implemented
  }
}


void QtSpinBox::setValue(Value arg)
{
  assert(!ignore_signals);
  ignore_signals = true;
  QDoubleSpinBox::setValue(arg);
  ignore_signals = false;
  float delta = std::abs(arg - value());
  int n_digits_of_precision = decimals();
  float tolerance = toleranceForPrecision(n_digits_of_precision);

  if (delta > tolerance) {
    cerr << "arg: " << arg << "\n";
    cerr << "QtSpinBox::value(): " << value() << "\n";
    cerr << "delta: " << delta << "\n";
    assert(false);
  }
}


void QtSpinBox::setMinimum(Value arg)
{
  QDoubleSpinBox::setMinimum(arg);
}


void QtSpinBox::setMaximum(Value arg)
{
  QDoubleSpinBox::setMaximum(arg);
}
