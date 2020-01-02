#include "faketreewidget.hpp"

#include <sstream>
#include "isequal.hpp"
#include "vectorio.hpp"
#include "removeindexfrom.hpp"

using std::ostringstream;
using LabelText = FakeTreeItem::LabelText;


bool FakeTreeItem::operator==(const FakeTreeItem &arg) const
{
  return isEqual(*this, arg);
}


std::string
  FakeTreeWidget::numericValueText(
    NumericValue value,
    NumericValue minimum_value,
    NumericValue maximum_value
  )
{
  ostringstream stream;

  stream <<
    ", value=" << value <<
    ", min=" << minimum_value <<
    ", max=" << maximum_value;

  return stream.str();
}


LabelText
  FakeTreeWidget::enumerationValueText(
    int value,
    const EnumerationOptions &options
  )
{
  ostringstream stream;
  stream << ", value=" << value << ", options=" << options;
  return stream.str();
}


LabelText FakeTreeWidget::stringValueText(const StringValue &value)
{
  ostringstream stream;
  stream << ", value=" << value;
  return stream.str();
}


void FakeTreeWidget::removeItem(const TreePath &path)
{
  Item &parent_item = item(parentPath(path));
  removeIndexFrom(parent_item.children, path.back());
}


bool FakeTreeWidget::operator==(const FakeTreeWidget &arg) const
{
  return isEqual(*this, arg);
}
