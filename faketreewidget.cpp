#include "faketreewidget.hpp"

#include <sstream>
#include <iostream>
#include "isequal.hpp"
#include "vectorio.hpp"
#include "removeindexfrom.hpp"

using std::ostringstream;
using std::cerr;
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


std::string FakeTreeWidget::boolValueText(bool value)
{
  ostringstream stream;
  stream << "value=" << value;
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


void
FakeTreeWidget::setItemStringValue(
  const TreePath &path, const StringValue &value
)
{
  item(path).value_string = stringValueText(value);
}


void
FakeTreeWidget::createStringItem(
  const TreePath &new_item_path,
  const LabelProperties &label_properties,
  const std::string &value
)
{
  createItem(new_item_path, label_properties, stringValueText(value));
}
