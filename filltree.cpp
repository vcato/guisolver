#include "filltree.hpp"

#include "numericvalue.hpp"


using std::string;
using LabelProperties = TreeWidget::LabelProperties;


static TreePaths::XYZ
  createXYZ(TreeWidget &tree_widget, const TreePath &parent_path)
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


static TreePaths::Position
  createMarker(
    TreeWidget &tree_widget,
    const TreePath &path,
    const string &name
  )
{
  tree_widget.createVoidItem(path,LabelProperties{"[Marker]"});

  tree_widget.createVoidItem(
    childPath(path,0),LabelProperties{"name: \"" + name + "\""}
  );

  tree_widget.createVoidItem(childPath(path,1),LabelProperties{"position: []"});

  return TreePaths::Position(createXYZ(tree_widget, childPath(path,1)));
}


TreePaths fillTree(TreeWidget &tree_widget)
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

  tree_paths.locals[0].position = createMarker(tree_widget,{0,0,3},"local1");
  tree_paths.locals[1].position = createMarker(tree_widget,{0,0,4},"local2");
  tree_paths.locals[2].position = createMarker(tree_widget,{0,0,5},"local3");
  tree_paths.globals[0].position = createMarker(tree_widget,{0,1},"global1");
  tree_paths.globals[1].position = createMarker(tree_widget,{0,2},"global2");
  tree_paths.globals[2].position = createMarker(tree_widget,{0,3},"global3");
  return tree_paths;
}
