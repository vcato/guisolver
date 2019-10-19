#ifndef TREEWIDGET_HPP_
#define TREEWIDGET_HPP_

#include <string>
#include "treepath.hpp"


struct TreeWidget {
  using NumericValue = int;

  struct LabelProperties {
    std::string text;
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
      const std::vector<std::string> &options,
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
    setItemLabel(const TreePath &path,const std::string &new_label) = 0;

  virtual void removeItem(const TreePath &path) = 0;
};

#endif /* TREEWIDGET_HPP_ */
