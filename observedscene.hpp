#include "scene.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "treepaths.hpp"
#include "markernamemap.hpp"
#include "stringvalue.hpp"
#include "sceneelementdescription.hpp"


enum class ManipulationType {
  translate,
  rotate,
  translate_and_scale,
  points
};


struct Clipboard {
  Optional<BodyIndex> maybe_cut_body_index;
  Optional<BodyIndex> maybe_cut_marker_index;

  Clipboard()
  {
  }

  bool canPasteTo(Optional<BodyIndex>, const SceneState &) const;
};


struct ObservedScene {
  Scene &scene;
  TreeWidget &tree_widget;
  SceneState scene_state;
  SceneHandles scene_handles;
  TreePaths tree_paths;
  Clipboard clipboard;
  std::function<void(SceneState&)> update_errors_function;
  std::function<void(SceneState&)> solve_function;

  ObservedScene(
    Scene &scene,
    TreeWidget &tree_widget,
    std::function<void(SceneState &)> update_errors_function,
    std::function<void(SceneState &)> solve_function
  );

  BodyIndex addBody(Optional<BodyIndex> maybe_parent_index = {});
  BoxIndex addBoxTo(BodyIndex);
  LineIndex addLineTo(BodyIndex);
  MeshIndex addMeshTo(BodyIndex, const Mesh &);
  MeshIndex addMeshTo(BodyIndex, const SceneState::MeshShape &);
  MarkerIndex addMarker(Optional<BodyIndex> = {});

  DistanceErrorIndex
    addDistanceError(
      Optional<MarkerIndex> start,
      Optional<MarkerIndex> end,
      Optional<BodyIndex>
    );

  void setDistanceErrorPointToMark(DistanceError, DistanceErrorPoint);

  VariableIndex addVariable();

  void cutBody(BodyIndex);
  void cutMarker(MarkerIndex);
  void markMarker(Marker);

  void markMeshPosition(BodyMeshPosition);

  BodyIndex
    pasteBodyGlobal(Optional<BodyIndex> maybe_new_parent_body_index);

  MarkerIndex
    pasteMarkerGlobal(Optional<BodyIndex> maybe_new_parent_body_index);

  bool clipboardContainsABody() const;
  bool clipboardContainsAMarker() const;
  void selectBody(BodyIndex);
  void selectMarker(MarkerIndex);
  void selectDistanceError(DistanceErrorIndex);
  void selectVariable(VariableIndex);
  void selectBox(BodyIndex, BoxIndex);
  void selectLine(BodyIndex, LineIndex);
  void selectMesh(BodyIndex, MeshIndex);
  BodyIndex duplicateBody(BodyIndex body_index);
  BodyIndex duplicateBodyWithDistanceErrors(BodyIndex);
  MarkerIndex duplicateMarker(MarkerIndex);
  MarkerIndex duplicateMarkerWithDistanceError(MarkerIndex);
  SceneElementDescription describePath(const TreePath &path) const;

  static BodyIndex
    duplicateBody(BodyIndex, MarkerNameMap &, ObservedScene &);

  static void removingMarker(ObservedScene &, MarkerIndex);
  static void removingBody(ObservedScene &, BodyIndex);
  void removeBody(BodyIndex);
  void removeMarker(MarkerIndex);
  void removeBox(BodyIndex, BoxIndex);
  void removeLine(BodyIndex, LineIndex);
  void removeMesh(BodyIndex, MeshIndex);
  void removeDistanceError(DistanceErrorIndex);
  void removeVariable(VariableIndex);
  static void clearClipboard(ObservedScene &);

  MeshIndex convertBoxToMesh(BodyIndex, BoxIndex);

  void replaceSceneStateWith(const SceneState &);
  void solveScene();
  bool canPasteTo(Optional<BodyIndex>);

  static void
  createBodyInTree(BodyIndex body_index, ObservedScene &observed_scene);

  static void
  createBodyInScene(BodyIndex body_index, ObservedScene &observed_scene);

  Optional<ManipulationType> properManipulationForSelectedObject() const;
  const bool *solveStatePtr(const TreePath &) const;
  bool *solveStatePtr(const TreePath &);
  void setSolveFlags(const SceneElementDescription &item, bool state);
  bool pathSupportsExpressions(const TreePath &) const;
  void updateSceneStateFromSceneObjects();

  void handleSceneStateChanged();
  void handleBodyMeshPositionStateChanged(BodyMeshPosition);
  void handleTreeSelectionChanged();
  void handleSceneSelectionChanged();
  void handleTreeExpressionChanged(const TreePath &, const std::string &);
  void handleTreeEnumerationIndexChanged(const TreePath &, int value);
  void handleTreeNumericValueChanged(const TreePath &path, NumericValue value);
  void handleTreeStringValueChanged(const TreePath &, const StringValue &);
  void handleTreeBoolValueChanged(const TreePath &, bool);

  static void
  createMarkerInScene(MarkerIndex marker_index, ObservedScene &observed_scene);

  static void
  createMarkerInTree(MarkerIndex marker_index, ObservedScene &observed_scene);

  private:
    struct Impl;
};
