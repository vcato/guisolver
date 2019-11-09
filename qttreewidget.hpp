#ifndef QTTREEWIDGET_HPP_
#define QTTREEWIDGET_HPP_

#include <QTreeWidget>

#include "qtslider.hpp"
#include "qtspinbox.hpp"
#include "treewidget.hpp"

struct QHBoxLayout;
struct QLabel;


class QtTreeWidget : public QTreeWidget, public TreeWidget {
  Q_OBJECT

  public:
    QtTreeWidget();
    ~QtTreeWidget();

    void
      createVoidItem(
        const TreePath &new_item_path,
        const TreeWidget::LabelProperties &label_properties
      ) override;

    void
      createNumericItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        const NumericValue value,
        const NumericValue minimum_value,
        const NumericValue maximum_value
      ) override;

    void
      createEnumerationItem(
        const TreePath &new_item_path,
        const LabelProperties &label_properties,
        const EnumerationOptions &options,
        int value
      ) override;

    void
      setItemNumericValue(
        const TreePath &,
        NumericValue value,
        NumericValue minimum_value,
        NumericValue maximum_value
      ) override;

    void
      setItemNumericValue(
        const TreePath &,
        NumericValue value
      ) override;

    void
      setItemLabel(const TreePath &path,const std::string &new_label) override;

    TreePath itemPath(QTreeWidgetItem &item) const;
    void selectItem(const TreePath &path) override;
    void setItemExpanded(const TreePath &path,bool new_expanded_state);
    int itemChildCount(const TreePath &parent_item) const;
    void removeItem(const TreePath &path) override;
    void removeChildItems(const TreePath &path);
    Optional<TreePath> selectedItem() const override;

  private slots:
    void selectionChangedSlot();
    void prepareMenuSlot(const QPoint &pos);

  private:
    struct Impl;
    bool _ignore_selelection_changed = false;

    static QTreeWidgetItem&
      createChildItem(QTreeWidgetItem &parent_item,const std::string &label);

    static void setItemText(QTreeWidgetItem &item,const std::string &label);

    QTreeWidgetItem &itemFromPath(const vector<int> &path) const;
    void buildPath(TreePath &path,QTreeWidgetItem &item) const;
    void changeItemToSlider(const TreePath &path);
    void changeItemToSpinBox(const TreePath &path);
    QtSlider* itemSliderPtr(const TreePath &path);
    QtSpinBox* itemSpinBoxPtr(const TreePath &path);
    QLabel *itemLabelPtr(const TreePath &path);

    template <typename T>
    T &createItemWidget(
      QTreeWidgetItem &item,
      const LabelProperties &
    );

    QTreeWidgetItem&
      createComboBoxItem(
        QTreeWidgetItem &parent_item,
        int index,
        const LabelProperties &,
        const vector<std::string> &enumeration_names,
        int value
      );

    void
      createSpinBoxItem(
        QTreeWidgetItem &parent_item,
        int child_index,
        const LabelProperties &,
        NumericValue value,
        NumericValue minimum_value,
        NumericValue maximum_value
      );

    void
      createStringItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        const std::string &value
      ) override;

    void
      createSliderItem(
        QTreeWidgetItem &parent_item,
        int child_index,
        const LabelProperties &,
        int value,
        int minimum_value,
        int maximum_value
      );

    QLabel&
      createItemLabelWidget(
        QTreeWidgetItem &item,
        QHBoxLayout &layout,
        const LabelProperties &label_properties
      );

    void
      createLineEditItem(
        QTreeWidgetItem &,
        const LabelProperties &,
        const std::string &value
      );

    void
      handleComboBoxItemIndexChanged(
        QTreeWidgetItem *item_ptr,
        int index
      );

    void
      handleSliderItemValueChanged(
        QTreeWidgetItem *item_ptr,
        int value
      );

    void
      handleSpinBoxItemValueChanged(
        QTreeWidgetItem *item_ptr,
        NumericValue
      );

    void
      handleLineEditItemValueChanged(
        QTreeWidgetItem *item_ptr,
        const std::string &value
      );

    void prepareMenu(const QPoint &pos);
};


#endif /* QTTREEWIDGET_HPP_ */
