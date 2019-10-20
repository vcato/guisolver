#include "filltree.hpp"

#include "numericvalue.hpp"


using std::string;
using LabelProperties = QtTreeWidget::LabelProperties;


static void
  createMarker(
    QtTreeWidget &tree_widget,
    const TreePath &path,
    const string &name
  )
{
  using LabelProperties = QtTreeWidget::LabelProperties;
  const NumericValue no_minimum = noMinimumNumericValue();
  const NumericValue no_maximum = noMaximumNumericValue();
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


static TreePaths::XYZ
  createXYZ(QtTreeWidget &tree_widget, const TreePath &parent_path)
{
  const NumericValue no_minimum = noMinimumNumericValue();
  const NumericValue no_maximum = noMaximumNumericValue();

  TreePaths::XYZ xyz_paths;
  xyz_paths.x = childPath(parent_path,0);
  xyz_paths.y = childPath(parent_path,1);
  xyz_paths.z = childPath(parent_path,2);

  tree_widget.createNumericItem(
    xyz_paths.x,LabelProperties{"x:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    xyz_paths.y,LabelProperties{"y:"},0,no_minimum, no_maximum
  );

  tree_widget.createNumericItem(
    xyz_paths.z,LabelProperties{"z:"},0,no_minimum, no_maximum
  );

  return xyz_paths;
}


TreePaths fillTree(QtTreeWidget &tree_widget)
{
  TreePaths tree_paths;

  tree_widget.createVoidItem({0},LabelProperties{"[Scene]"});

  tree_widget.createVoidItem(
    {0,0},LabelProperties{"[Transform]"}
  );

  TreePath box_translation_path = {0,0,0};
  TreePath box_rotation_path = {0,0,1};

  tree_widget.createVoidItem(
    box_translation_path,LabelProperties{"translation: []"}
  );

  tree_paths.box.translation =
    TreePaths::Translation(createXYZ(tree_widget, box_translation_path));

  tree_widget.createVoidItem(
    box_rotation_path,LabelProperties{"rotation: []"}
  );

  tree_paths.box.rotation =
    TreePaths::Rotation(createXYZ(tree_widget, box_rotation_path));

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
