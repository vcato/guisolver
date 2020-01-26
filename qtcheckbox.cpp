#include "qtcheckbox.hpp"

#include <iostream>
#include <cassert>

using std::cerr;


void QtCheckBox::stateChangedSlot(int new_state)
{
  if (ignore_state_changed_signal) return;

  if (value_changed_function) {
    value_changed_function(new_state == Qt::Checked);
  }
}


QtCheckBox::QtCheckBox()
{
  connect(this, SIGNAL(stateChanged(int)), SLOT(stateChangedSlot(int)));
}


void QtCheckBox::setValue(bool arg)
{
  assert(!ignore_state_changed_signal);
  ignore_state_changed_signal = true;
  setChecked(arg);
  ignore_state_changed_signal = false;
}
