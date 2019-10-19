#include "qtslider.hpp"

#include <iostream>
#include <QHBoxLayout>
#include "qtwidget.hpp"
#include "qtlayout.hpp"
#include "qtlineedit.hpp"
#include "optional.hpp"
#include "maybeint.hpp"

using std::cerr;
using std::string;


QSlider &QtSlider::slider()
{
  assert(slider_ptr);
  return *slider_ptr;
}


const QSlider &QtSlider::slider() const
{
  assert(slider_ptr);
  return *slider_ptr;
}


QtLineEdit &QtSlider::lineEdit()
{
  assert(line_edit_ptr);
  return *line_edit_ptr;
}


QtSlider::QtSlider()
{
  QHBoxLayout &layout = createLayout<QHBoxLayout>(*this);

  QSlider &slider = createWidget<QSlider>(layout);
  slider.setOrientation(Qt::Horizontal);
  connect(&slider,SIGNAL(valueChanged(int)),SLOT(sliderValueChangedSlot(int)));
  slider_ptr = &slider;

  QtLineEdit &line_edit = createWidget<QtLineEdit>(layout);
  line_edit.text_changed_function =
    [this](string arg){ lineEditValueChanged(arg); };
  line_edit_ptr = &line_edit;
  updateLineEditFromSlider();
}


void QtSlider::sliderValueChangedSlot(int value)
{
  if (ignore_slider_signal) return;

  updateLineEditFromSlider();

  if (value_changed_function) {
    value_changed_function(value);
  }
}


void QtSlider::setMinimum(int arg)
{
  slider().setMinimum(arg);
}


void QtSlider::setMaximum(int arg)
{
  slider().setMaximum(arg);
}


void QtSlider::setSliderValue(int arg)
{
  ignore_slider_signal = true;
  slider().setValue(arg);
  ignore_slider_signal = false;
}


void QtSlider::setValue(int arg)
{
  slider().setValue(arg);
  setLineEditValue(arg);
}


void QtSlider::setLineEditValue(int arg)
{
  lineEdit().setText(std::to_string(arg));
}


void QtSlider::updateLineEditFromSlider()
{
  setLineEditValue(sliderValue());
}


int QtSlider::sliderValue() const
{
  return slider().value();
}


void QtSlider::lineEditValueChanged(const string &arg)
{
  Optional<int> maybe_value = maybeInt(arg);

  if (maybe_value) {
    setSliderValue(*maybe_value);
  }
}
