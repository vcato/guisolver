#include "qtlineedit.hpp"

#include <iostream>

using std::cerr;


QtLineEdit::QtLineEdit()
{
  connect(
    this,
    SIGNAL(textChanged(const QString &)),
    SLOT(textChangedSlot(const QString &))
  );
}


void QtLineEdit::textChangedSlot(const QString &new_text)
{
  if (ignore_signals) return;
  text_changed_function(new_text.toStdString());
}


void QtLineEdit::setText(const std::string &new_text)
{
  ignore_signals = true;
  QLineEdit::setText(QString::fromStdString(new_text));
  ignore_signals = false;
}
