#include "filltree.hpp"


using std::string;


static void
  createMarker(
    QtTreeWidget &tree_widget,
    const TreePath &path,
    const string &name
  )
{
  using LabelProperties = QtTreeWidget::LabelProperties;
  using NumericValue = QtTreeWidget::NumericValue;
  NumericValue no_minimum = std::numeric_limits<NumericValue>::min();
  NumericValue no_maximum = std::numeric_limits<NumericValue>::max();
  tree_widget.createVoidItem(path,LabelProperties{"[Marker]"});

  tree_widget.createVoidItem(
    childPath(path,0),LabelProperties{"name: \"" + name + "\""}
  );

  tree_widget.createVoidItem(childPath(path,1),LabelProperties{"position: []"});

  tree_widget.createNumericItem(
    childPath(path,1,0),LabelProperties{"x"},0,no_minimum,no_maximum
  );

  tree_widget.createNumericItem(
    childPath(path,1,1),LabelProperties{"y"},0,no_minimum,no_maximum
  );

  tree_widget.createNumericItem(
    childPath(path,1,2),LabelProperties{"z"},0,no_minimum,no_maximum
  );
}


TreePaths fillTree(QtTreeWidget &tree_widget)
{
  TreePaths tree_paths;
  using NumericValue = QtTreeWidget::NumericValue;
  using LabelProperties = QtTreeWidget::LabelProperties;
  NumericValue no_minimum = std::numeric_limits<NumericValue>::min();
  NumericValue no_maximum = std::numeric_limits<NumericValue>::max();

  tree_widget.createVoidItem({0},LabelProperties{"[Scene]"});

  tree_widget.createVoidItem(
    {0,0},LabelProperties{"[Transform]"}
  );

  tree_widget.createVoidItem(
    {0,0,0},LabelProperties{"translation: []"}
  );

  TreePath box_translation_x_path = {0,0,0,0};
  TreePath box_translation_y_path = {0,0,0,1};
  TreePath box_translation_z_path = {0,0,0,2};
  tree_paths.box.translation.x = box_translation_x_path;
  tree_paths.box.translation.y = box_translation_y_path;
  tree_paths.box.translation.z = box_translation_z_path;

  tree_widget.createNumericItem(
    box_translation_x_path,LabelProperties{"x:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    box_translation_y_path,LabelProperties{"y:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    box_translation_z_path,LabelProperties{"z:"},0,no_minimum, no_maximum
  );

  tree_widget.createVoidItem(
    {0,0,1},LabelProperties{"rotation: []"}
  );

  tree_widget.createNumericItem(
    {0,0,1,0},LabelProperties{"x:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    {0,0,1,1},LabelProperties{"y:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    {0,0,1,2},LabelProperties{"z:"},0,no_minimum, no_maximum
  );

  tree_widget.createVoidItem(
    {0,0,2},LabelProperties{"[Box]"}
  );

  createMarker(tree_widget,{0,0,3},"local1");
  createMarker(tree_widget,{0,0,4},"local2");
  createMarker(tree_widget,{0,0,5},"local3");
  createMarker(tree_widget,{0,1},"global1");
  createMarker(tree_widget,{0,2},"global2");
  createMarker(tree_widget,{0,3},"global3");
  return tree_paths;
}
