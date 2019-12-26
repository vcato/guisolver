#include <QSplitter>


template <typename Widget>
Widget& createWidget(QSplitter &parent_widget)
{
  Widget *widget_ptr = new Widget;
  assert(widget_ptr);
  parent_widget.addWidget(widget_ptr);
  return *widget_ptr;
}
