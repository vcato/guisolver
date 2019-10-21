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


void QtSpinBox::setValue(Value arg)
{
  assert(!ignore_signals);
  ignore_signals = true;
  QDoubleSpinBox::setValue(arg);
  ignore_signals = false;

  if (std::abs(arg - value()) > .005) {
    cerr << "arg: " << arg << "\n";
    cerr << "QtSpinBox::value(): " << value() << "\n";
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
