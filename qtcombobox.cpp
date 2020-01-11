#include "qtcombobox.hpp"

#include <iostream>
#include <cassert>
#include <QWheelEvent>

using std::cerr;


QtComboBox::QtComboBox()
: ignore_signals(false)
{
  connect(
    this,
    SIGNAL(currentIndexChanged(int)),
    SLOT(currentIndexChangedSlot(int))
  );

  setFocusPolicy(Qt::StrongFocus);
    // Prevent the mouse wheel from being used so we don't accidentally
    // change the value when the combo box is inside a tree widget and
    // we are scrolling the tree widget with the mouse wheel.
}


void QtComboBox::wheelEvent(QWheelEvent *event_ptr)
{
  assert(event_ptr);

  if (!hasFocus()) {
    event_ptr->ignore();
    // Don't allow wheel events to take the focus so that we don't
    // inadvertantly change the spin box value while scrolling the
    // tree with the mouse wheel.
  }
  else {
    QComboBox::wheelEvent(event_ptr);
    // If we've explicitly focused the spin box by clicking on it, then
    // allow the wheel events to be used.
  }
}


void QtComboBox::currentIndexChangedSlot(int index)
{
  if (ignore_signals) return;

  current_index_changed_function(index);
}


void QtComboBox::setItems(const vector<std::string> &names)
{
  ignore_signals = true;

  clear();
  addItems(names);

  ignore_signals = false;
}


void QtComboBox::setIndex(int arg)
{
  ignore_signals = true;

  setCurrentIndex(arg);

  ignore_signals = false;
}


void QtComboBox::addItems(const vector<std::string> &names)
{
  for (auto &name : names) {
    addItem(QString::fromStdString(name));
  }
}
