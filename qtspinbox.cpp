#include "qtspinbox.hpp"

#include <cassert>
#include <iostream>
#include <QWheelEvent>

using std::cerr;


QtSpinBox::QtSpinBox()
{
  connect(
    this,
    SIGNAL(valueChanged(int)),
    SLOT(valueChangedSlot(int))
  );
}


void QtSpinBox::valueChangedSlot(int value)
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
    QSpinBox::wheelEvent(event_ptr);
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


void QtSpinBox::setValue(int arg)
{
  assert(!ignore_signals);
  ignore_signals = true;
  QSpinBox::setValue(arg);
  ignore_signals = false;
}
