#ifndef QTWIDGET_HPP_
#define QTWIDGET_HPP_

#include <cassert>
#include <QWidget>


template <typename Layout>
Layout& createLayout(QWidget &widget)
{
  Layout *layout_ptr = new Layout;
  widget.setLayout(layout_ptr);
  return *layout_ptr;
}


template <typename Widget>
Widget& createWidget(QWidget &parent_widget)
{
  Widget *widget_ptr = new Widget(&parent_widget);
  assert(widget_ptr);
  return *widget_ptr;
}



#endif /* QTWIDGET_HPP_ */
