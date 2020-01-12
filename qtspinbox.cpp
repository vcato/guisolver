#include "qtspinbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <QWheelEvent>
#include <QTimer>
#include <QLineEdit>

using std::cerr;


namespace {
struct PassThroughMouseButtonPresses : QObject {
  bool eventFilter(QObject *, QEvent *event_ptr) override
  {
    if (event_ptr->type() == QEvent::MouseButtonPress) {
      event_ptr->ignore();
    }

    return false;
  }
};
}



QtSpinBox::QtSpinBox()
{
  connect(
    this,
    SIGNAL(valueChanged(double)),
    SLOT(valueChangedSlot(double))
  );

  setSingleStep(0.1);

  lineEdit()->installEventFilter(new PassThroughMouseButtonPresses);
    // This is so that if we click on the line edit in the spin box, the
    // button press event will pass through to the tree item so it
    // will be selected.
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
    // Don't allow wheel events to take the focus so that we don't
    // inadvertantly change the spin box value while scrolling the
    // tree with the mouse wheel.
  }
  else {
    QDoubleSpinBox::wheelEvent(event_ptr);
    // If we've explicitly focused the spin box by clicking on it, then
    // allow the wheel events to be used.
  }
}


void QtSpinBox::mousePressEvent(QMouseEvent *event_ptr)
{
  QDoubleSpinBox::mousePressEvent(event_ptr);

  event_ptr->ignore();
    // Mark the event as ignored (instead of handled), so that it will
    // pass through to the tree and select the item.
}


void QtSpinBox::focusInEvent(QFocusEvent *event_ptr)
{
  QDoubleSpinBox::focusInEvent(event_ptr);

  setFocusPolicy(Qt::WheelFocus);
    // One the user clicks on the widget, we have to turn on WheelFocus so that
    // the mouse wheel events will be recognized.

  QTimer::singleShot(0, this, SLOT(selectTextSlot()));
}


void QtSpinBox::focusOutEvent(QFocusEvent *event_ptr)
{
  QDoubleSpinBox::focusOutEvent(event_ptr);

  setFocusPolicy(Qt::StrongFocus);
    // Go back to strong focus if this widget loses focus so that the mouse
    // wheel won't cause this widget to gain focus.
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


void QtSpinBox::selectTextSlot()
{
  lineEdit()->selectAll();
}
