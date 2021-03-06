#include "mainwindowcontroller.hpp"

#include <fstream>
#include "vectorio.hpp"
#include "sceneerror.hpp"
#include "scenesolver.hpp"
#include "sceneobjects.hpp"
#include "matchconst.hpp"
#include "scenestatetaggedvalue.hpp"
#include "observedscene.hpp"
#include "scenestateio.hpp"
#include "parsedouble.hpp"
#include "evaluateexpression.hpp"
#include "solveflags.hpp"
#include "readobj.hpp"
#include "objmesh.hpp"
#include "vec3state.hpp"

using View = MainWindowView;
using std::cerr;
using std::string;
using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using ManipulatorType = Scene::ManipulatorType;


template <typename F>
static bool anyFlagsAreSet(const F &for_each_function)
{
  bool any_are_set = false;
  for_each_function([&](bool arg){ if (arg) any_are_set = true; });
  return any_are_set;
}


struct MainWindowController::Impl {
  struct Data {
    View &view;
    ObservedScene observed_scene;
    Clipboard clipboard;

    Data(View &, Scene &, TreeWidget &);
  };

  Data data_member;

  Impl(View &view, Scene &scene, TreeWidget &tree_widget)
  : data_member(view, scene, tree_widget)
  {
  }

  static Impl &impl(MainWindowController &controller)
  {
    assert(controller.impl_ptr);
    return *controller.impl_ptr;
  }

  static Data &data(MainWindowController &controller)
  {
    return impl(controller).data_member;
  }

  static ObservedScene &observedScene(MainWindowController &controller)
  {
    return data(controller).observed_scene;
  }

  static View &view(MainWindowController &controller)
  {
    return data(controller).view;
  }

  static void handleSceneChanging(MainWindowController &);
  static void handleSceneChanged(MainWindowController &);

  static Optional<NumericValue>
  evaluateInput(
    MainWindowController &controller, const string &text, const TreePath &path
  )
  {
    ObservedScene &observed_scene = observedScene(controller);
    const string &arg = text;

    if (text.length() == 0) {
      return {};
    }

    if (text[0] == '=') {
      if (!observed_scene.pathSupportsExpressions(path)) {
        return {};
      }

      string expr = arg.substr(1);
      observed_scene.handleTreeExpressionChanged(path, expr);
      return {};
    }
    else {
      observed_scene.handleTreeExpressionChanged(path, "");
      return parseDouble(arg);
    }
  }

  static TreeWidget::MenuItems
    contextMenuItemsForPath(
      MainWindowController &controller,
      const TreePath &path
    );

  static void
    addDistanceErrorPressed(
      MainWindowController &controller,
      Optional<BodyIndex>
    );

  static void
    importObjPressed(MainWindowController &controller, BodyIndex body_index);

  static void addVariablePressed(MainWindowController &controller);

  static void
    pasteGlobalPressed(MainWindowController &controller, const TreePath &path);

  static void
    solveAllPressed(
      ObservedScene &,
      const SceneElementDescription &,
      bool state
    );

  static void addMarkerPressed(MainWindowController &, const TreePath &);
  static void addBodyPressed(MainWindowController &, const TreePath &);
  static void addBoxPressed(MainWindowController &, BodyIndex);
  static void addLinePressed(MainWindowController &, BodyIndex);
  static void removeBodyPressed(MainWindowController &, BodyIndex);
  static void duplicateBodyPressed(MainWindowController &, BodyIndex);

  static void
    duplicateBodyWithDistanceErrorsPressed(MainWindowController &, BodyIndex);

  static void
    duplicateMarkerWithDistanceErrorPressed(
      MainWindowController &, MarkerIndex
    );

  static void
    removeDistanceErrorPressed(
      ObservedScene &,
      DistanceErrorIndex distance_error_index
    );

  static void removeMarkerPressed(ObservedScene &, MarkerIndex);

  static void
    removeBoxPressed(
      MainWindowController &,
      BodyIndex,
      BoxIndex
    );

  static void duplicateMarkerPressed(ObservedScene &, MarkerIndex);
  static void cutBodyPressed(MainWindowController &, const TreePath &);
  static void cutMarkerPressed(MainWindowController &, MarkerIndex);
  static void markMarkerPressed(MainWindowController &, MarkerIndex);
};


void
MainWindowController::Impl::handleSceneChanging(
  MainWindowController &controller
)
{
  // The mouse button is down.  The scene is being changed, but we don't
  // consider this change complete.

  Impl::observedScene(controller).handleSceneChanging();
}


void
  MainWindowController::Impl::handleSceneChanged(
    MainWindowController &controller
  )
{
  observedScene(controller).handleSceneChanged();
}


void
  MainWindowController::Impl::addDistanceErrorPressed(
    MainWindowController &controller,
    Optional<BodyIndex> optional_body_index
  )
{
  ObservedScene &observed_scene = observedScene(controller);

  DistanceErrorIndex index =
    observed_scene.addDistanceError(
      /*maybe_start_marker_index*/{},
      /*maybe_end_marker_index*/{},
      optional_body_index
    );

  observed_scene.selectDistanceError(index);
}


void
MainWindowController::Impl::importObjPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);

  Optional<std::string> maybe_path =
    Impl::view(controller).askForOpenPath();

  if (!maybe_path) {
    return;
  }

  auto &path = *maybe_path;
  std::ifstream stream(path);

  if (!stream) {
    cerr << "Unable to open " << path << "\n";
    return;
  }

  ObjData obj_data = readObj(stream);
  Mesh mesh = meshFromObj(obj_data);
  MeshIndex mesh_index = observed_scene.addMeshTo(body_index, mesh);
  observed_scene.selectMesh(body_index, mesh_index);
}


void
  MainWindowController::Impl::addVariablePressed(
    MainWindowController &controller
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  VariableIndex index = observed_scene.addVariable();
  observed_scene.selectVariable(index);
}


static BodyIndex
  bodyIndexFromTreePath(const TreePath &path, const TreePaths &tree_paths)
{
  for (auto i : indicesOf(tree_paths.bodies)) {
    if (tree_paths.body(i).path == path) {
      return i;
    }
  }

  assert(false);
}


static Optional<BodyIndex>
maybeBodyIndexFromTreePath(const TreePath &path, const TreePaths &tree_paths)
{
  if (tree_paths.path == path) {
    return {};
  }

  return bodyIndexFromTreePath(path, tree_paths);

}


void
  MainWindowController::Impl::addMarkerPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;

  Optional<BodyIndex> maybe_body_index =
    maybeBodyIndexFromTreePath(path, tree_paths);;

  Marker marker = observed_scene.createMarker(maybeBody(maybe_body_index));
  observed_scene.selectMarker(marker.index);
}


void
MainWindowController::Impl::pasteGlobalPressed(
  MainWindowController &controller,
  const TreePath &path
)
{
  ObservedScene &observed_scene = observedScene(controller);
  SceneState &scene_state = observed_scene.scene_state;

  Optional<BodyIndex> maybe_new_parent_body_index =
    maybeBodyIndexFromTreePath(path, observed_scene.tree_paths);

  if (observed_scene.clipboardContainsABody()) {
    BodyIndex new_body_index =
      observed_scene.pasteBodyGlobal(maybe_new_parent_body_index);

    observed_scene.selectBody(new_body_index);
    solveScene(scene_state);
    observed_scene.handleSceneStateChanged();
    return;
  }

  if (observed_scene.clipboardContainsAMarker()) {
    MarkerIndex new_marker_index =
      observed_scene.pasteMarkerGlobal(maybe_new_parent_body_index);

    observed_scene.selectMarker(new_marker_index);
    solveScene(scene_state);
    observed_scene.handleSceneStateChanged();
    return;
  }

  else {
    assert(false); // not implemented
  }
}


void
MainWindowController::Impl::solveAllPressed(
  ObservedScene &observed_scene,
  const SceneElementDescription &item,
  bool state
)
{
  observed_scene.setSolveFlags(item, state);
  observed_scene.solveScene();
  observed_scene.handleSceneStateChanged();
}


void
MainWindowController::Impl::addBodyPressed(
  MainWindowController &controller,
  const TreePath &parent_path
)
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;

  Optional<BodyIndex> maybe_parent_body_index =
    maybeBodyIndexFromTreePath(parent_path, tree_paths);

  BodyIndex new_body_index = observed_scene.addBody(maybe_parent_body_index);
  observed_scene.selectBody(new_body_index);
}


void
MainWindowController::Impl::addBoxPressed(
  MainWindowController &controller,
  BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);
  BoxIndex box_index = observed_scene.addBoxTo(body_index);
  observed_scene.selectBox(body_index, box_index);
}


void
MainWindowController::Impl::addLinePressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);
  LineIndex line_index = observed_scene.addLineTo(body_index);
  observed_scene.selectLine(body_index, line_index);
}


void
MainWindowController::Impl::removeBodyPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  observedScene(controller).removeBody(body_index);
}


void
MainWindowController::Impl::duplicateBodyPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);
  BodyIndex new_body_index = observed_scene.duplicateBody(body_index);
  observed_scene.selectBody(new_body_index);
}


void
MainWindowController::Impl::duplicateBodyWithDistanceErrorsPressed(
  MainWindowController &controller, BodyIndex body_index
)
{
  ObservedScene &observed_scene = observedScene(controller);

  BodyIndex new_body_index =
    observed_scene.duplicateBodyWithDistanceErrors(body_index);

  observed_scene.selectBody(new_body_index);
}


void
MainWindowController::Impl::duplicateMarkerWithDistanceErrorPressed(
  MainWindowController &controller, MarkerIndex marker_index
)
{
  ObservedScene &observed_scene = observedScene(controller);

  BodyIndex new_marker_index =
    observed_scene.duplicateMarkerWithDistanceError(marker_index);

  observed_scene.selectMarker(new_marker_index);
}


void
  MainWindowController::Impl::removeDistanceErrorPressed(
    ObservedScene &observed_scene,
    DistanceErrorIndex distance_error_index
  )
{
  observed_scene.removeDistanceError(distance_error_index);
}


void
  MainWindowController::Impl::removeMarkerPressed(
    ObservedScene &observed_scene,
    MarkerIndex marker_index
  )
{
  observed_scene.removeMarker(marker_index);
}


void
  MainWindowController::Impl::duplicateMarkerPressed(
    ObservedScene &observed_scene,
    MarkerIndex source_marker_index
  )
{
  MarkerIndex new_marker_index =
    observed_scene.duplicateMarker(source_marker_index);

  observed_scene.selectMarker(new_marker_index);
}


void
MainWindowController::Impl::cutMarkerPressed(
  MainWindowController &controller,
  MarkerIndex marker_index
)
{
  observedScene(controller).cutMarker(marker_index);
}


void
MainWindowController::Impl::markMarkerPressed(
  MainWindowController &controller,
  MarkerIndex marker_index
)
{
  observedScene(controller).markMarker(marker_index);
}


void
  MainWindowController::Impl::cutBodyPressed(
    MainWindowController &controller,
    const TreePath &path
  )
{
  ObservedScene &observed_scene = observedScene(controller);
  TreePaths &tree_paths = observed_scene.tree_paths;
  BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);
  observed_scene.cutBody(body_index);
}


template <typename T>
static void appendTo(vector<T> &v, const vector<T> &n)
{
  v.insert(v.end(), n.begin(), n.end());
}


static Optional<DistanceErrorPoint>
maybeDistanceErrorPoint(SceneElementDescription::Type type)
{
  if (type == SceneElementDescription::Type::distance_error_start) {
    return DistanceErrorPoint::start;
  }
  else if (type == SceneElementDescription::Type::distance_error_end) {
    return DistanceErrorPoint::end;
  }

  return {};
}


TreeWidget::MenuItems
MainWindowController::Impl::contextMenuItemsForPath(
  MainWindowController &controller,
  const TreePath &path
)
{
  ObservedScene &observed_scene = observedScene(controller);
  TreeWidget::MenuItems menu_items;
  const TreePaths &tree_paths = observed_scene.tree_paths;
  SceneElementDescription item = observed_scene.describePath(path);
  using ItemType = SceneElementDescription::Type;
  const ItemType item_type = item.type;

  auto add_marker_function =
    [&controller,path]{
      Impl::addMarkerPressed(controller, path);
    };

  auto add_body_function =
    [&controller,path]{ Impl::addBodyPressed(controller, path); };

  auto paste_global_function =
    [&controller,path]{
      Impl::pasteGlobalPressed(controller, path);
    };

  if (item_type == ItemType::translation || item_type == ItemType::rotation) {
    auto solve_all_on_function =
      [&observed_scene, item]{
        Impl::solveAllPressed(observed_scene, item, true);
      };

    auto solve_all_off_function =
      [&observed_scene, item]{
        Impl::solveAllPressed(observed_scene, item, false);
      };

    appendTo(menu_items,{
      {"Solve All On", solve_all_on_function },
      {"Solve All Off", solve_all_off_function },
    });
  }

  if (item_type == ItemType::scene) {
    auto add_distance_error_function =
      [&controller,path]{
        Impl::addDistanceErrorPressed(controller, /*optional_body_index*/{});
      };

    auto add_variable_function =
      [&controller, path]{
        Impl::addVariablePressed(controller);
      };

    appendTo(menu_items,{
      {"Add Distance Error", add_distance_error_function },
      {"Add Marker", add_marker_function },
      {"Add Body", add_body_function },
      {"Add Variable", add_variable_function },
    });

    if (observed_scene.canPasteTo({})) {
      appendTo(menu_items,{
        {"Paste Preserving Global", paste_global_function}
      });
    }
  }

  if (item_type == ItemType::body) {
    BodyIndex body_index = bodyIndexFromTreePath(path, tree_paths);

    auto cut_body_function =
      [&controller,path]{
        Impl::cutBodyPressed(controller, path);
      };

    auto remove_body_function =
      [&controller,body_index]{
        Impl::removeBodyPressed(controller, body_index);
      };

    auto duplicate_body_function =
      [&controller,body_index]{
        Impl::duplicateBodyPressed(controller, body_index);
      };

    auto duplicate_body_with_distance_errors_function =
      [&controller,body_index]{
        Impl::duplicateBodyWithDistanceErrorsPressed(controller, body_index);
      };

    auto add_box_function =
      [&controller,body_index]{
        Impl::addBoxPressed(controller, body_index);
      };

    auto add_line_function =
      [&controller,body_index]{
        Impl::addLinePressed(controller, body_index);
      };

    auto add_distance_error_function =
      [&controller,body_index]{
        Impl::addDistanceErrorPressed(controller, body_index);
      };

    auto import_obj_function =
      [&controller,body_index]{
        Impl::importObjPressed(controller, body_index);
      };

    appendTo(menu_items,{
      {"Add Marker", add_marker_function},
      {"Add Body", add_body_function},
      {"Add Box", add_box_function},
      {"Add Line", add_line_function},
      {"Import Obj...", import_obj_function},
      {"Add Distance Error", add_distance_error_function},
      {"Cut", cut_body_function },
      {"Remove", remove_body_function },
      {"Duplicate", duplicate_body_function },
      {"Duplicate With Distance Errors",
        duplicate_body_with_distance_errors_function },
    });

    if (observed_scene.canPasteTo(body_index)) {
      appendTo(menu_items,{
        {"Paste Preserving Global", paste_global_function}
      });
    }
  }

  if (item_type == ItemType::box_geometry) {
    BodyIndex body_index = *item.maybe_body_index;
    size_t box_index = *item.maybe_box_index;

    auto remove_box_function =
      [&observed_scene, body_index, box_index]{
        observed_scene.removeBox(body_index, box_index);
      };

    auto convert_to_mesh_function =
      [&observed_scene, body_index, box_index]{
        MeshIndex new_mesh_index =
          observed_scene.convertBoxToMesh(body_index, box_index);

        observed_scene.selectMesh(body_index, new_mesh_index);
      };

    appendTo(menu_items,{
      {"Convert to Mesh", convert_to_mesh_function},
      {"Remove", remove_box_function}
    });
  }

  if (item_type == ItemType::line_geometry) {
    BodyIndex body_index = *item.maybe_body_index;
    LineIndex line_index = *item.maybe_line_index;

    auto remove_line_function =
      [&observed_scene, body_index, line_index]{
        observed_scene.removeLine(body_index, line_index);
      };

    appendTo(menu_items,{
      {"Remove", remove_line_function}
    });
  }

  if (item_type == ItemType::mesh_geometry) {
    BodyIndex body_index = *item.maybe_body_index;
    MeshIndex mesh_index = *item.maybe_mesh_index;

    auto remove_mesh_function = [&observed_scene, body_index, mesh_index]{
      observed_scene.removeMesh(body_index, mesh_index);
    };

    appendTo(menu_items,{
      {"Remove", remove_mesh_function}
    });
  }

  if (item_type == ItemType::marker) {
    auto index = *item.maybe_marker_index;

    auto remove_marker_function = [&observed_scene,index]{
      Impl::removeMarkerPressed(observed_scene, index);
    };

    auto duplicate_marker_function = [&observed_scene,index]{
      Impl::duplicateMarkerPressed(observed_scene, index);
    };

    auto duplicate_marker_with_distance_error_function =
      [&controller,index]{
        Impl::duplicateMarkerWithDistanceErrorPressed(controller, index);
      };

    auto cut_marker_function = [&controller,index]{
      Impl::cutMarkerPressed(controller, index);
    };

    auto mark_marker_function = [&controller,index]{
      Impl::markMarkerPressed(controller, index);
    };

    appendTo(menu_items,{
      {"Remove", remove_marker_function},
      {"Duplicate", duplicate_marker_function},
      {"Cut", cut_marker_function},
      {"Mark", mark_marker_function},
      {"Duplicate With Distance Error",
        duplicate_marker_with_distance_error_function },
    });
  }

  if (item_type == ItemType::distance_error) {
    auto index = *item.maybe_distance_error_index;

    auto remove_distance_error_function =
      [&observed_scene,index]{
        Impl::removeDistanceErrorPressed(observed_scene, index);
      };

    appendTo(menu_items,{
      {"Remove", remove_distance_error_function}
    });
  }

  if (Optional<DistanceErrorPoint> maybe_distance_error_point =
    maybeDistanceErrorPoint(item_type)) {
    auto index = *item.maybe_distance_error_index;
    DistanceErrorPoint point = *maybe_distance_error_point;

    auto set_to_mark_function = [&observed_scene,index,point]{
      observed_scene.setDistanceErrorPointToMark(
        DistanceError{index}, point
      );
    };

    appendTo(menu_items,{
      {"Set To Mark", set_to_mark_function}
    });
  }

  if (item_type == ItemType::variable) {
    VariableIndex variable_index = *item.maybe_variable_index;

    auto remove_variable_function =
      [&observed_scene, variable_index]{
        observed_scene.removeVariable(variable_index);
      };

    appendTo(menu_items, {
      {"Remove", remove_variable_function}
    });
  }

  if (item_type == ItemType::mesh_position) {
    BodyIndex body_index = *item.maybe_body_index;
    MeshIndex mesh_index = *item.maybe_mesh_index;
    MeshPositionIndex position_index = *item.maybe_mesh_position_index;

    BodyMeshPosition body_mesh_position =
      Body(body_index).mesh(mesh_index).position(position_index);

    auto mark_function = [&observed_scene, body_mesh_position]{
      observed_scene.markMeshPosition(body_mesh_position);
    };

    auto add_handle_function =
    [&observed_scene, body_mesh_position, body_index]{
      Body body(body_index);
      Marker marker = observed_scene.createMarker(body);

      Vec3 local =
        bodyMeshPositionRelativeToBody(
          body_mesh_position, observed_scene.scene_state
        );

      observed_scene.scene_state[marker].position =
        makePositionStateFromVec3(local);

      observed_scene.createDistanceError(
        PointLink(marker),
        PointLink(body_mesh_position),
        body
      );

      observed_scene.select(marker);
    };

    appendTo(menu_items, {
      {"Mark", mark_function},
      {"Add Handle", add_handle_function}
    });
  }

  return menu_items;
}


MainWindowController::Impl::Data::Data(
  View &view_arg,
  Scene &scene_arg,
  TreeWidget &tree_widget_arg
)
: view(view_arg),
  observed_scene(
    scene_arg,
    tree_widget_arg,
    [](SceneState &state){
      updateErrorsInState(state);
    },
    [](SceneState &state){
      solveScene(state);
    }
  )
{
}


MainWindowController::MainWindowController(View &view)
: impl_ptr(new Impl(view, view.scene(), view.treeWidget()))
{
  TreeWidget &tree_widget = view.treeWidget();
  Scene &scene = view.scene();
  ObservedScene &observed_scene = Impl::observedScene(*this);
  scene.changed_callback = [&]{ Impl::handleSceneChanged(*this); };
  scene.changing_callback = [&]{ Impl::handleSceneChanging(*this); };

  scene.selection_changed_callback =
    [&observed_scene]{ observed_scene.handleSceneSelectionChanged(); };

  tree_widget.evaluate_function =
    [this](const TreePath &path, const string &text){
      return Impl::evaluateInput(*this, text, path);
    };

  tree_widget.enumeration_item_index_changed_callback =
    [&observed_scene](const TreePath &path, int index){
      observed_scene.handleTreeEnumerationIndexChanged(path, index);
    };

  tree_widget.selection_changed_callback =
    [&observed_scene](){
      observed_scene.handleTreeSelectionChanged();
    };

  tree_widget.context_menu_items_callback =
    [this](const TreePath &path){
      return Impl::contextMenuItemsForPath(*this, path);
    };

  tree_widget.numeric_item_value_changed_callback =
    [&observed_scene](const TreePath &path, NumericValue value){
      observed_scene.handleTreeNumericValueChanged(path, value);
    };

  tree_widget.string_item_value_changed_callback =
    [&observed_scene](const TreePath &path, const StringValue &value){
      observed_scene.handleTreeStringValueChanged(path, value);
    };

  tree_widget.bool_item_value_changed_callback =
    [&observed_scene](const TreePath &path, bool new_value){
      observed_scene.handleTreeBoolValueChanged(path, new_value);
    };
}


MainWindowController::~MainWindowController() = default;


void MainWindowController::replaceSceneStateWith(const SceneState &new_state)
{
  ObservedScene &observed_scene = Impl::observedScene(*this);
  SceneState solved_new_state = new_state;
  solveScene(solved_new_state);
  observed_scene.replaceSceneStateWith(solved_new_state);
}


void MainWindowController::newPressed()
{
  replaceSceneStateWith(SceneState());
}


void MainWindowController::savePressed()
{
  Optional<string> maybe_path = Impl::view(*this).askForSavePath();

  if (!maybe_path) {
    // Cancelled
  }
  else {
    saveScene(Impl::observedScene(*this).scene_state, *maybe_path);
  }
}


void MainWindowController::openPressed()
{
  Optional<string> maybe_path = Impl::view(*this).askForOpenPath();

  if (!maybe_path) {
    // Cancelled
  }
  else {
    SceneState new_scene_state;
    loadScene(new_scene_state, *maybe_path);
    replaceSceneStateWith(new_scene_state);
  }
}
