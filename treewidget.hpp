#ifndef TREEWIDGET_HPP_
#define TREEWIDGET_HPP_

#include <string>
#include <functional>
#include "treepath.hpp"
#include "numericvalue.hpp"
#include "optional.hpp"
#include "vector.hpp"


struct TreeWidget {
  struct MenuItem;
  using EnumerationOptions = vector<std::string>;
  using MenuItems = vector<MenuItem>;

  struct LabelProperties {
    std::string text;
  };

  struct MenuItem {
    std::string label;
    std::function<void()> callback;
  };

  virtual void
    createVoidItem(
      const TreePath &new_item_path,
      const TreeWidget::LabelProperties &label_properties
    ) = 0;

  virtual void
    createNumericItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    ) = 0;

  virtual void
    createEnumerationItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      const EnumerationOptions &options,
      int value
    ) = 0;

  virtual void
    createStringItem(
      const TreePath &new_item_path,
      const LabelProperties &,
      const std::string &value
    ) = 0;

  virtual void
    setItemNumericValue(
      const TreePath &,
      NumericValue value,
      NumericValue minimum_value,
      NumericValue maximum_value
    ) = 0;

  virtual void
    setItemNumericValue(
      const TreePath &,
      NumericValue value
    ) = 0;

  virtual void
    setItemLabel(const TreePath &path,const std::string &new_label) = 0;

  virtual void selectItem(const TreePath &path) = 0;
  virtual void removeItem(const TreePath &path) = 0;
  virtual Optional<TreePath> selectedItem() const = 0;

  std::function<void(const TreePath &,NumericValue)>
    spin_box_item_value_changed_callback;

  std::function<void()> selection_changed_callback;

  std::function<void(const TreePath &,int index)>
    enumeration_item_index_changed_callback;

  std::function<void(const TreePath &,int index)>
    slider_item_value_changed_callback;

  std::function<void(const TreePath &,const std::string &value)>
    line_edit_item_value_changed_callback;

  std::function<vector<MenuItem>(const TreePath &)>
    context_menu_items_callback;
};

#endif /* TREEWIDGET_HPP_ */
