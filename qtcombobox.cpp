#include "qtcombobox.hpp"

#include <iostream>

using std::cerr;


QtComboBox::QtComboBox()
: ignore_signals(false)
{
  connect(
    this,
    SIGNAL(currentIndexChanged(int)),
    SLOT(currentIndexChangedSlot(int))
  );
}


void QtComboBox::currentIndexChangedSlot(int index)
{
  if (ignore_signals) return;

  current_index_changed_function(index);
}


void QtComboBox::setItems(const std::vector<std::string> &names)
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


void QtComboBox::addItems(const std::vector<std::string> &names)
{
  for (auto &name : names) {
    addItem(QString::fromStdString(name));
  }
}
