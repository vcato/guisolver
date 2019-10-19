#include <QLayout>


template <typename Widget>
Widget& createWidget(QLayout &layout)
{
  Widget *widget_ptr = new Widget;
  layout.addWidget(widget_ptr);
  assert(widget_ptr);
  return *widget_ptr;
}
