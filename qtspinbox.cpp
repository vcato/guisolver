#include "qtspinbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <QWheelEvent>
#include <QTimer>
#include <QLineEdit>
#include "optional.hpp"
#include "startswith.hpp"
#include "evaluateexpression.hpp"
#include "parsedouble.hpp"

using std::cerr;
using std::string;
using std::istringstream;
using std::ostringstream;


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
  setSingleStep(0.1);

  lineEdit()->installEventFilter(new PassThroughMouseButtonPresses);
    // This is so that if we click on the line edit in the spin box, the
    // button press event will pass through to the tree item so it
    // will be selected.

  connect(this, SIGNAL(editingFinished()), SLOT(editingFinishedSlot()));
}


void QtSpinBox::_callValueChangedFunction(double value) const
{
  if (!value_changed_function) {
    cerr << "value_changed_function is not set.\n";
    return;
  }

  value_changed_function(value);
}


void QtSpinBox::valueChangedSlot(double value)
{
  if (_ignore_signals) {
    return;
  }

  _callValueChangedFunction(value);
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
    Base::wheelEvent(event_ptr);
    // If we've explicitly focused the spin box by clicking on it, then
    // allow the wheel events to be used.
  }
}


void QtSpinBox::mousePressEvent(QMouseEvent *event_ptr)
{
  Base::mousePressEvent(event_ptr);

  event_ptr->ignore();
    // Mark the event as ignored (instead of handled), so that it will
    // pass through to the tree and select the item.
}


void QtSpinBox::focusInEvent(QFocusEvent *event_ptr)
{
  // When we get focus, restore the actual text, since this was replaced by
  // the evaluated text.
  lineEdit()->setText(QString::fromStdString(_input));
  lineEdit()->setReadOnly(false);
  Base::focusInEvent(event_ptr);

  setFocusPolicy(Qt::WheelFocus);
    // One the user clicks on the widget, we have to turn on WheelFocus so that
    // the mouse wheel events will be recognized.

  QTimer::singleShot(0, this, SLOT(selectTextSlot()));
}


void QtSpinBox::focusOutEvent(QFocusEvent *event_ptr)
{
  Base::focusOutEvent(event_ptr);

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


static string valueAsString(double arg, int decimals)
{
  ostringstream stream;
  stream.setf(stream.fixed);
  stream.precision(decimals);
  stream << arg;
  return stream.str();
}


void QtSpinBox::_setLineEditTextToValue()
{
  lineEdit()->setText(QString::fromStdString(valueAsString(_value, _decimals)));
}


void QtSpinBox::_updateLineEdit()
{
  _setLineEditTextToValue();
  _updateButtons();
}


void QtSpinBox::setValue(Value arg)
{
  assert(!_ignore_signals);
    // If ignore_signals is true, then we are already in the middle of
    // setting the value, so somehow a signal got back to the caller.

  if (_inputIsExpression()) {
    // If the input is an expression, then it is effectively read-only.
    return;
  }

  _ignore_signals = true;
  _value = arg;
  _input = valueAsString(arg, _decimals);
  _updateLineEdit();
  _ignore_signals = false;
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
  _minimum = arg;
}


void QtSpinBox::setMaximum(Value arg)
{
  _maximum = arg;
}


void QtSpinBox::selectTextSlot()
{
  lineEdit()->selectAll();
}


double QtSpinBox::_clamped(double value) const
{
  if (value < _minimum) {
    return _minimum;
  }

  if (value > _maximum) {
    return _maximum;
  }

  return value;
}


Optional<QtSpinBox::Value>
QtSpinBox::_evaluateInput(const string &text) const
{
  if (evaluate_function) {
    return evaluate_function(text);
  }
  else {
    return Optional<QtSpinBox::Value>(parseDouble(text));
  }
}


QValidator::State QtSpinBox::validate(QString &input, int &/*pos*/) const
{
  Optional<Value> maybe_value = _evaluateInput(input.toStdString());

  if (maybe_value) {
    return QValidator::State::Acceptable;
  }

  return QValidator::State::Intermediate;
}


void QtSpinBox::setSingleStep(Value arg)
{
  _single_step = arg;
}


double QtSpinBox::value() const
{
  return _value;
}


int QtSpinBox::decimals() const
{
  return _decimals;
}


void QtSpinBox::setDecimals(int arg)
{
  _decimals = arg;
}


QtSpinBox::StepEnabled QtSpinBox::stepEnabled() const
{
  if (_inputIsExpression()) {
    return StepNone;
  }

  StepEnabled result = StepNone;

  if (_value > _minimum) {
    result |= StepDownEnabled;
  }

  if (_value < _maximum) {
    result |= StepUpEnabled;
  }

  return result;
}


bool QtSpinBox::event(QEvent *event_ptr)
{
  return Base::event(event_ptr);
}


void QtSpinBox::keyPressEvent(QKeyEvent *event_ptr)
{
  Base::keyPressEvent(event_ptr);

  if (!lineEdit()->isReadOnly()) {
    _input = lineEdit()->text().toStdString();
  }
  else {
    // If the line edit is read-only, then we've finished editing the
    // value, and the current value may be the evaluate expression value
    // instead of what the user input, so we need to ignore it.
  }

  Optional<Value> maybe_value = _evaluateInput(_input);

  if (maybe_value) {
    if (*maybe_value != _value) {
      _value = _clamped(*maybe_value);

      if (!_ignore_signals) {
        _callValueChangedFunction(_value);
      }
      else {
        // The validation happened due to us setting the text on the line
        // edit as part of setting the value, but we don't want the callback
        // to be called in that case.
      }
    }
  }

  _updateButtons();
}


bool QtSpinBox::_inputIsExpression() const
{
  return startsWith(_input,string("="));
}


void QtSpinBox::_updateButtons()
{
  if (_inputIsExpression()) {
    setButtonSymbols(NoButtons);
  }
  else {
    setButtonSymbols(UpDownArrows);
  }
}


void QtSpinBox::stepBy(int arg)
{
  _value = _clamped(_value + _single_step*arg);
  _updateLineEdit();
  _callValueChangedFunction(_value);
}


void QtSpinBox::editingFinishedSlot()
{
  _setLineEditTextToValue();

  if (_inputIsExpression()) {
    lineEdit()->setReadOnly(true);
    lineEdit()->deselect();
  }
}
