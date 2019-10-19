#include "qtspinbox.hpp"


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
  if (!value_changed_function) {
    return;
  }

  value_changed_function(value);
}
