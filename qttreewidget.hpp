#ifndef QTTREEWIDGET_HPP_
#define QTTREEWIDGET_HPP_

#include <QTreeWidget>

#include "qtslider.hpp"
#include "qtspinbox.hpp"
#include "qtcombobox.hpp"
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
        const NumericValue maximum_value,
        int digits_of_precision
      ) override;

    void
      createBoolItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        bool value
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

    void setItemInput(const TreePath &path, const Input &input) override;
    void setItemBoolValue(const TreePath &, bool value) override;

    void
      setItemLabel(const TreePath &path,const std::string &new_label) override;

    void setItemPending(const TreePath &path,bool) override;

    void
      setItemEnumerationValue(
        const TreePath &,
        int index,
        const EnumerationOptions &
      ) override;

    void selectItem(const TreePath &path) override;
    Optional<TreePath> selectedItem() const override;
    int itemChildCount(const TreePath &parent_item) const override;
    void removeItem(const TreePath &path) override;

    TreePath itemPath(QTreeWidgetItem &item) const;
    void setItemExpanded(const TreePath &path,bool new_expanded_state);
    void removeChildItems(const TreePath &path);

  private slots:
    void selectionChangedSlot();
    void prepareMenuSlot(const QPoint &pos);

  private:
    struct Impl;
    bool _ignore_selelection_changed = false;

    static QTreeWidgetItem&
      createChildItem(QTreeWidgetItem &parent_item,const std::string &label);

    QTreeWidgetItem &itemFromPath(const TreePath &path) const;
    QTreeWidgetItem &parentItemFromPath(const TreePath &) const;
    void buildPath(TreePath &path,QTreeWidgetItem &item) const;
    void changeItemToSlider(const TreePath &path);
    void changeItemToSpinBox(const TreePath &path);
    QtSlider* itemSliderPtr(const TreePath &path);
    QtSpinBox* itemSpinBoxPtr(const TreePath &path);
    QtComboBox* itemComboBoxPtr(const TreePath &path);
    QLabel *itemLabelPtr(const TreePath &path);
    QTreeWidgetItem &insertItem(const TreePath &path);

    template <typename T>
    T &createItemWidget(
      QTreeWidgetItem &item,
      const LabelProperties &
    );

    QTreeWidgetItem&
      createComboBoxItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        const EnumerationOptions &,
        int value
      );

    void
      createSpinBoxItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        NumericValue value,
        NumericValue minimum_value,
        NumericValue maximum_value,
        int digits_of_precision
      );

    void
      createStringItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        const std::string &value
      ) override;

    void
      createSliderItem(
        const TreePath &new_item_path,
        const LabelProperties &,
        int value,
        int minimum_value,
        int maximum_value
      );

    void
      createCheckBoxItem(
        const TreePath &new_item_path,
        const LabelProperties &label_properties,
        bool value
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
