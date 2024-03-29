#include "qttreewidget.hpp"

#include <cassert>
#include <iostream>
#include <QBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QCheckBox>
#include <QMenu>
#include "qtlayout.hpp"
#include "qtwidget.hpp"
#include "qtcombobox.hpp"
#include "qtlineedit.hpp"
#include "qttreewidgetitem.hpp"
#include "qtmenu.hpp"
#include "qtcheckbox.hpp"
#include "vectorio.hpp"
#include "numericvalue.hpp"
#include "numericvaluelimits.hpp"
#include "parsedouble.hpp"

using std::string;
using std::cerr;

struct QtTreeWidget::Impl {
  struct QtItemWrapperWidget : QWidget {
    QLabel *label_widget_ptr = nullptr;
    QWidget *value_widget_ptr = nullptr;
  };

  static QtItemWrapperWidget *
    itemWrapperWidgetPtr(QTreeWidget &tree,QTreeWidgetItem &item)
  {
    QWidget *widget_ptr = tree.itemWidget(&item,/*column*/0);

    QtItemWrapperWidget *item_widget_ptr =
      dynamic_cast<QtItemWrapperWidget*>(widget_ptr);

    return item_widget_ptr;
  }

  static bool
  itemIsInTree(QTreeWidgetItem &item, const QTreeWidget &tree_widget)
  {
    auto item_ptr = &item;

    for (;;) {
      auto parent_item_ptr = item_ptr->parent();

      if (!parent_item_ptr) {
        break;
      }

      item_ptr = parent_item_ptr;
    }

    assert(item_ptr);
    return (tree_widget.indexOfTopLevelItem(item_ptr) >= 0);
  }

  static Optional<NumericValue>
  evaluateNumberInput(
    QtTreeWidget &tree_widget, const string &input, QTreeWidgetItem &item
  )
  {
    if (!itemIsInTree(item, tree_widget)) {
      // This can happen when we remove an item.  The item is removed from
      // thre tree before the widgets are destroyed.  A spin box will try
      // to validate its input before it is destroyed.
      return {};
    }

    if (tree_widget.evaluate_function) {
      return tree_widget.evaluate_function(tree_widget.itemPath(item), input);
    }

    return parseDouble(input);
  }

  static QtItemWrapperWidget &
    itemWrapperWidget(QTreeWidget &tree,QTreeWidgetItem &item)
  {
    QtItemWrapperWidget *item_widget_ptr = itemWrapperWidgetPtr(tree,item);
    assert(item_widget_ptr);
    return *item_widget_ptr;
  }

  static void
    setupSpinBox(
      QtTreeWidget &tree_widget,
      QtSpinBox &spin_box,
      QTreeWidgetItem &item
    )
  {
    spin_box.setFocusPolicy(Qt::StrongFocus);

    spin_box.value_changed_function =
      [&tree_widget,&item](NumericValue value){
        tree_widget.handleSpinBoxItemValueChanged(&item,value);
      };

    spin_box.evaluate_function =
      [&tree_widget, &item](const string &input)
      -> Optional<NumericValue>
      {
        return evaluateNumberInput(tree_widget, input, item);
      };
  }

  static void
    setupSlider(
      QtTreeWidget &tree_widget,
      QtSlider &slider,
      QTreeWidgetItem &item
    )
  {
    slider.value_changed_function =
      [&tree_widget, &item](int value){
        tree_widget.handleSliderItemValueChanged(&item,value);
      };
  }

  static QBoxLayout &boxLayout(QtItemWrapperWidget &wrapper_widget)
  {
    auto *box_layout_ptr =
      dynamic_cast<QBoxLayout *>(wrapper_widget.layout());
    assert(box_layout_ptr);
    QBoxLayout &box_layout = *box_layout_ptr;
    return box_layout;
  }

  static void
    destroyValueWidget(QtItemWrapperWidget &wrapper_widget)
  {
    QWidget *value_widget_ptr = wrapper_widget.value_widget_ptr;
    wrapper_widget.value_widget_ptr = 0;
    QBoxLayout &box_layout = boxLayout(wrapper_widget);
    box_layout.removeWidget(value_widget_ptr);
    delete value_widget_ptr;
  }

  static QtSlider &
    createSlider(
      QtTreeWidget &tree_widget,
      QtItemWrapperWidget &wrapper_widget,
      QTreeWidgetItem &item
    )
  {
    QBoxLayout &box_layout = boxLayout(wrapper_widget);
    auto& slider = createWidget<QtSlider>(box_layout);
    wrapper_widget.value_widget_ptr = &slider;
    setupSlider(tree_widget,slider,item);
    return slider;
  }

  static QtSpinBox &
    createSpinBox(
      QtTreeWidget &tree_widget,
      QtItemWrapperWidget &wrapper_widget,
      QTreeWidgetItem &item
    )
  {
    QBoxLayout &box_layout = boxLayout(wrapper_widget);
    auto& spin_box = createWidget<QtSpinBox>(box_layout);
    setupSpinBox(tree_widget,spin_box,item);
    wrapper_widget.value_widget_ptr = &spin_box;
    return spin_box;
  }

  static QtCheckBox* itemCheckBoxPtr(QtTreeWidget &, const TreePath &path);
  static QtLineEdit* itemLineEditPtr(QtTreeWidget &, const TreePath &path);
};


static void setLabelWidgetText(QLabel &label_widget,const string &label)
{
  label_widget.setText(QString::fromStdString(label));
  // label_widget.setStyleSheet("QLabel { color : gray; }");
}


static void setLabelWidgetPending(QLabel &label_widget,bool state)
{
  if (state) {
    label_widget.setStyleSheet("QLabel { color : gray; }");
  }
  else {
    label_widget.setStyleSheet("QLabel { color : black; }");
  }
}


QLabel&
QtTreeWidget::createItemLabelWidget(
  QTreeWidgetItem &,
  QHBoxLayout &layout,
  const LabelProperties &label_properties
)
{
  const std::string &label = label_properties.text;
  QLabel &label_widget = createWidget<QLabel>(layout);
  setLabelWidgetText(label_widget,label);
  return label_widget;
}


QtTreeWidget::QtTreeWidget()
{
  assert(header());
  header()->close();
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(
    this,
    SIGNAL(currentItemChanged(QTreeWidgetItem *,QTreeWidgetItem *)),
    SLOT(selectionChangedSlot())
  );

  connect(
    this,
    SIGNAL(customContextMenuRequested(const QPoint &)),
    SLOT(prepareMenuSlot(const QPoint &))
  );
}


template <typename T>
T &
QtTreeWidget::createItemWidget(
  QTreeWidgetItem &item,
  const LabelProperties &label_properties
)
{
  Impl::QtItemWrapperWidget *wrapper_widget_ptr =
    new Impl::QtItemWrapperWidget();

  // NOTE: setting the item widget before adding the contents makes
  // it not have the proper size.
  QHBoxLayout &layout = createLayout<QHBoxLayout>(*wrapper_widget_ptr);

  wrapper_widget_ptr->label_widget_ptr =
    &createItemLabelWidget(item,layout,label_properties);

  T& widget = createWidget<T>(layout);

  wrapper_widget_ptr->value_widget_ptr = &widget;
  setItemWidget(&item,/*column*/0,wrapper_widget_ptr);

  return widget;
}


static void setItemText(QTreeWidgetItem &item, const std::string &label)
{
  item.setText(/*column*/0,QString::fromStdString(label));
  // item.setTextColor(/*column*/0, Qt::GlobalColor::gray);
}


static void setItemPending(QTreeWidgetItem &item,const bool new_state)
{
  if (new_state) {
    item.setForeground(/*column*/0, Qt::GlobalColor::gray);
  }
  else {
    item.setForeground(/*column*/0, Qt::GlobalColor::black);
  }
}


QTreeWidgetItem &QtTreeWidget::insertItem(const TreePath &path)
{
  return ::insertChildItem(parentItemFromPath(path), path.back());
}


void
  QtTreeWidget::createVoidItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties
  )
{
  QTreeWidgetItem &item = insertItem(new_item_path);
  setItemText(item, label_properties.text);
}


static bool
  useSliderForRange(NumericValue minimum_value,NumericValue maximum_value)
{
  bool value_is_limited_on_both_ends =
    minimum_value != noMinimumNumericValue() &&
    maximum_value != noMaximumNumericValue();

  return value_is_limited_on_both_ends;
}


void
  QtTreeWidget::createNumericItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties,
    const NumericValue value,
    const NumericValue minimum_value,
    const NumericValue maximum_value,
    int digits_of_precision
  )
{
  static_assert(std::is_same<NumericValue,float>::value,"");

  if (useSliderForRange(minimum_value,maximum_value)) {
    createSliderItem(
      new_item_path,
      label_properties,
      value,
      minimum_value,
      maximum_value
    );
  }
  else {
    createSpinBoxItem(
      new_item_path,
      label_properties,
      value,
      minimum_value,
      maximum_value,
      digits_of_precision
    );
  }
}


void
QtTreeWidget::createBoolItem(
  const TreePath &new_item_path,
  const LabelProperties &label_properties,
  bool value
)
{
  createCheckBoxItem(new_item_path, label_properties, value);
}


void
  QtTreeWidget::createEnumerationItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties,
    const EnumerationOptions &options,
    int value
  )
{
  createComboBoxItem(new_item_path, label_properties, options, value);
}


void
  QtTreeWidget::createStringItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties,
    const string &value
  )
{
  QTreeWidgetItem &parent_item = parentItemFromPath(new_item_path);
  createLineEditItem(parent_item,label_properties,value);
}


QTreeWidgetItem&
  QtTreeWidget::createComboBoxItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties,
    const EnumerationOptions &enumeration_names,
    int value
  )
{
  QTreeWidgetItem &item = insertItem(new_item_path);
  QtComboBox &combo_box = createItemWidget<QtComboBox>(item,label_properties);

  combo_box.current_index_changed_function =
    [this,&item](int index){
      handleComboBoxItemIndexChanged(&item,index);
    };

  combo_box.setItems(enumeration_names);
  combo_box.setIndex(value);
  return item;
}


void
  QtTreeWidget::createLineEditItem(
    QTreeWidgetItem &parent_item,
    const LabelProperties &label_properties,
    const std::string &value
  )
{
  QTreeWidgetItem &item = ::createChildItem(parent_item);

  QtLineEdit &line_edit =
    createItemWidget<QtLineEdit>(item,label_properties);

  line_edit.text_changed_function = [&](const string &new_text){
    handleLineEditItemValueChanged(&item, new_text);
  };

  line_edit.setText(value);
}


static float
singleStepForDigitsOfPrecision(int n_digits_of_precision)
{
  if (n_digits_of_precision == 2) {
    return 0.1;
  }
  else {
    return 1;
  }
}


void
  QtTreeWidget::createSpinBoxItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties,
    NumericValue value,
    NumericValue minimum_value,
    NumericValue maximum_value,
    int digits_of_precision
  )
{
  QTreeWidgetItem &item = insertItem(new_item_path);
  QtSpinBox &spin_box = createItemWidget<QtSpinBox>(item,label_properties);
  spin_box.setMinimum(minimum_value);
  spin_box.setMaximum(maximum_value);
  spin_box.setDecimals(digits_of_precision);
  spin_box.setSingleStep(singleStepForDigitsOfPrecision(digits_of_precision));
  spin_box.setValue(value);
  Impl::setupSpinBox(*this,spin_box,item);
}


void
  QtTreeWidget::createSliderItem(
    const TreePath &new_item_path,
    const LabelProperties &label_properties,
    int value,
    int minimum_value,
    int maximum_value
  )
{
  QTreeWidgetItem &item = insertItem(new_item_path);

  // Logic needs to be consistent when we recreate the widget if the value
  // changes.
  QtSlider &slider = createItemWidget<QtSlider>(item, label_properties);
  Impl::setupSlider(*this,slider,item);
  slider.setValue(value);
  slider.setMinimum(minimum_value);
  slider.setMaximum(maximum_value);

  // Need to have a value changed function
}


void
QtTreeWidget::createCheckBoxItem(
  const TreePath &new_item_path,
  const LabelProperties &label_properties,
  bool value
)
{
  QTreeWidgetItem &item = insertItem(new_item_path);
  QtCheckBox &check_box = createItemWidget<QtCheckBox>(item, label_properties);

  check_box.value_changed_function =
    [this, &item](bool new_value){
      if (bool_item_value_changed_callback) {
        bool_item_value_changed_callback(itemPath(item), new_value);
      }
      else {
        cerr << "bool_item_value_changed_callback not set.\n";
      }
    };

  check_box.setValue(value);
}


QTreeWidgetItem &QtTreeWidget::itemFromPath(const vector<int> &path) const
{
  int path_length = path.size();

  if (path_length==0) {
    assert(invisibleRootItem());
    return *invisibleRootItem();
  }

  assert(path_length>0);
  QTreeWidgetItem *item_ptr = topLevelItem(path[0]);

  int i = 1;
  while (i!=path_length) {
    item_ptr = item_ptr->child(path[i]);
    ++i;
  }

  if (!item_ptr) {
    cerr << "No item for path " << path << "\n";
  }

  assert(item_ptr);

  return *item_ptr;
}


QTreeWidgetItem &QtTreeWidget::parentItemFromPath(const TreePath &path) const
{
  return itemFromPath(parentPath(path));
}


void QtTreeWidget::buildPath(vector<int> &path,QTreeWidgetItem &item) const
{
  QTreeWidgetItem *parent_item_ptr = item.parent();

  if (!parent_item_ptr) {
    int index = indexOfTopLevelItem(&item);
    assert(index >= 0);
    addChildToPath(path, index);
    return;
  }

  buildPath(path,*parent_item_ptr);
  addChildToPath(path, parent_item_ptr->indexOfChild(&item));
}


vector<int> QtTreeWidget::itemPath(QTreeWidgetItem &item) const
{
  vector<int> path;
  buildPath(path,item);
  return path;
}


void
  QtTreeWidget::handleComboBoxItemIndexChanged(
    QTreeWidgetItem *item_ptr,
    int index
  )
{
  assert(item_ptr);

  TreePath path = itemPath(*item_ptr);
  enumeration_item_index_changed_callback(path,index);
}


void
  QtTreeWidget::handleSpinBoxItemValueChanged(
    QTreeWidgetItem *item_ptr,
    NumericValue value
  )
{
  assert(item_ptr);

  if (!numeric_item_value_changed_callback) {
    cerr << "numeric_item_value_changed_function is not set\n";
    return;
  }

  numeric_item_value_changed_callback(itemPath(*item_ptr), value);
}


void
  QtTreeWidget::handleSliderItemValueChanged(
    QTreeWidgetItem *item_ptr,
    int value
  )
{
  assert(item_ptr);

  if (!numeric_item_value_changed_callback) {
    cerr << "numeric_item_value_changed_function is not set\n";
    return;
  }

  numeric_item_value_changed_callback(itemPath(*item_ptr), value);
}


void
  QtTreeWidget::handleLineEditItemValueChanged(
    QTreeWidgetItem *item_ptr,
    const string &value
  )
{
  assert(item_ptr);
  assert(string_item_value_changed_callback);
  string_item_value_changed_callback(itemPath(*item_ptr),value);
}


void QtTreeWidget::changeItemToSlider(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    Impl::itemWrapperWidget(*this,item);

  Impl::destroyValueWidget(wrapper_widget);
  Impl::createSlider(*this,wrapper_widget,item);
}


void QtTreeWidget::changeItemToSpinBox(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    Impl::itemWrapperWidget(*this,item);

  Impl::destroyValueWidget(wrapper_widget);
  Impl::createSpinBox(*this,wrapper_widget,item);
}


QLabel *QtTreeWidget::itemLabelPtr(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  Impl::QtItemWrapperWidget *item_widget_ptr =
    Impl::itemWrapperWidgetPtr(*this,item);

  if (!item_widget_ptr) {
    return nullptr;
  }

  QLabel *label_widget_ptr = item_widget_ptr->label_widget_ptr;
  return label_widget_ptr;
}


QtSlider* QtTreeWidget::itemSliderPtr(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    Impl::itemWrapperWidget(*this,item);

  auto slider_ptr = dynamic_cast<QtSlider*>(wrapper_widget.value_widget_ptr);
  return slider_ptr;
}


QtSpinBox* QtTreeWidget::itemSpinBoxPtr(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    Impl::itemWrapperWidget(*this,item);

  auto spin_box_ptr = dynamic_cast<QtSpinBox*>(wrapper_widget.value_widget_ptr);
  return spin_box_ptr;
}


QtComboBox* QtTreeWidget::itemComboBoxPtr(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    Impl::itemWrapperWidget(*this,item);

  auto combo_box_ptr =
    dynamic_cast<QtComboBox*>(wrapper_widget.value_widget_ptr);

  return combo_box_ptr;
}


QtCheckBox*
QtTreeWidget::Impl::itemCheckBoxPtr(
  QtTreeWidget &tree_widget,
  const TreePath &path
)
{
  QTreeWidgetItem &item = tree_widget.itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    itemWrapperWidget(tree_widget, item);

  auto check_box_ptr =
    dynamic_cast<QtCheckBox*>(wrapper_widget.value_widget_ptr);

  return check_box_ptr;
}


QtLineEdit*
QtTreeWidget::Impl::itemLineEditPtr(
  QtTreeWidget &tree_widget, const TreePath &path
)
{
  QTreeWidgetItem &item = tree_widget.itemFromPath(path);

  Impl::QtItemWrapperWidget &wrapper_widget =
    itemWrapperWidget(tree_widget, item);

  auto line_edit_ptr =
    dynamic_cast<QtLineEdit*>(wrapper_widget.value_widget_ptr);

  return line_edit_ptr;
}


void
QtTreeWidget::setItemNumericValue(
  const TreePath &path,
  NumericValue value,
  NumericValue minimum_value,
  NumericValue maximum_value
)
{
  bool use_slider = useSliderForRange(minimum_value,maximum_value);
  auto *slider_ptr = itemSliderPtr(path);
  auto *spin_box_ptr = itemSpinBoxPtr(path);

  if (use_slider && !slider_ptr) {
    assert(spin_box_ptr);
    changeItemToSlider(path);
    spin_box_ptr = nullptr;
    slider_ptr = itemSliderPtr(path);
  }

  if (!use_slider && !spin_box_ptr) {
    assert(slider_ptr);
    changeItemToSpinBox(path);
    slider_ptr = nullptr;
    spin_box_ptr = itemSpinBoxPtr(path);
  }

  if (slider_ptr) {
    auto &slider = *slider_ptr;
    slider.setMinimum(minimum_value);
    slider.setMaximum(maximum_value);
    slider.setValue(value);
  }
  else {
    assert(spin_box_ptr);
    auto &spin_box = *spin_box_ptr;
    spin_box.setMinimum(minimum_value);
    spin_box.setMaximum(maximum_value);
    spin_box.setValue(value);
  }
}


void QtTreeWidget::setItemInput(const TreePath &path, const string &input)
{
  auto *spin_box_ptr = itemSpinBoxPtr(path);
  assert(spin_box_ptr);
  spin_box_ptr->setInput(input);
}


void QtTreeWidget::setItemBoolValue(const TreePath &path, bool value)
{
  auto *check_box_ptr = Impl::itemCheckBoxPtr(*this, path);

  assert(check_box_ptr);
  check_box_ptr->setValue(value);
}


void
QtTreeWidget::setItemStringValue(
  const TreePath &path, const StringValue &value
)
{
  QtLineEdit *line_edit_ptr = Impl::itemLineEditPtr(*this, path);
  assert(line_edit_ptr);
  line_edit_ptr->setText(value);
}


void
  QtTreeWidget::setItemNumericValue(
    const TreePath &path,
    NumericValue value
  )
{
  auto *slider_ptr = itemSliderPtr(path);
  auto *spin_box_ptr = itemSpinBoxPtr(path);

  if (slider_ptr) {
    slider_ptr->setValue(value);
  }
  else {
    assert(spin_box_ptr);
    spin_box_ptr->setValue(value);
  }
}


void
  QtTreeWidget::setItemEnumerationValue(
    const TreePath &path,
    int value,
    const EnumerationOptions &options
  )
{
  QtComboBox *combo_box_ptr = itemComboBoxPtr(path);
  assert(combo_box_ptr);
  combo_box_ptr->setItems(options);
  combo_box_ptr->setIndex(value);
}


void
  QtTreeWidget::setItemLabel(const TreePath &path,const std::string &new_label)
{
  QLabel *label_widget_ptr = itemLabelPtr(path);

  if (label_widget_ptr) {
    setLabelWidgetText(*label_widget_ptr,new_label);
  }
  else {
    QTreeWidgetItem &item = itemFromPath(path);
    setItemText(item,new_label);
  }
}


void QtTreeWidget::setItemPending(const TreePath &path, bool new_state)
{
  QLabel *label_widget_ptr = itemLabelPtr(path);

  if (label_widget_ptr) {
    setLabelWidgetPending(*label_widget_ptr, new_state);
  }
  else {
    QTreeWidgetItem &item = itemFromPath(path);
    ::setItemPending(item, new_state);
  }
}


QTreeWidgetItem&
  QtTreeWidget::createChildItem(
    QTreeWidgetItem &parent_item,
    const std::string &label
  )
{
  QTreeWidgetItem &pass_item = ::createChildItem(parent_item);
  setItemText(pass_item,label);
  return pass_item;
}


void QtTreeWidget::removeItem(const TreePath &path)
{
  assert(!_ignore_selelection_changed);
  _ignore_selelection_changed = true;
  auto child_index = path.back();
  ::removeChildItem(parentItemFromPath(path),child_index);
  _ignore_selelection_changed = false;
}


int QtTreeWidget::itemChildCount(const TreePath &parent_path) const
{
  return itemFromPath(parent_path).childCount();
}


void QtTreeWidget::removeChildItems(const TreePath &path)
{
  QTreeWidgetItem &item = itemFromPath(path);

  while (item.childCount()>0) {
    item.removeChild(item.child(item.childCount()-1));
  }
}


void QtTreeWidget::selectItem(const TreePath &path)
{
  _ignore_selelection_changed = true;
  setCurrentItem(&itemFromPath(path));
  _ignore_selelection_changed = false;
}


void QtTreeWidget::setItemExpanded(const TreePath &path,bool new_expanded_state)
{
  itemFromPath(path).setExpanded(new_expanded_state);
}


void QtTreeWidget::selectionChangedSlot()
{
  if (_ignore_selelection_changed) {
    return;
  }

  if (selection_changed_callback) {
    selection_changed_callback();
  }
}


Optional<TreePath> QtTreeWidget::selectedItem() const
{
  QTreeWidgetItem *item_ptr = currentItem();

  if (!item_ptr) {
    return {};
  }

  return itemPath(*item_ptr);
}


QtTreeWidget::~QtTreeWidget()
{
}


void QtTreeWidget::prepareMenuSlot(const QPoint &pos)
{
  prepareMenu(pos);
}


void QtTreeWidget::prepareMenu(const QPoint &pos)
{
  QTreeWidgetItem *widget_item_ptr = itemAt(pos);
  TreePath path;

  if (widget_item_ptr) {
    path = itemPath(*widget_item_ptr);
  }

  if (!context_menu_items_callback) {
    return;
  }

  auto menu_items = context_menu_items_callback(path);

  if (menu_items.empty()) {
    return;
  }

  QMenu menu;

  for (auto &item : menu_items) {
    createAction(menu, item.label, item.callback, item.optional_checked_state);
  }

  menu.exec(mapToGlobal(pos));
}
