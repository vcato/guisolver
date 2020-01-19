#include "scene.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "treepaths.hpp"
#include "markernamemap.hpp"


struct Clipboard {
  Optional<BodyIndex> maybe_cut_body_index;
  Optional<BodyIndex> maybe_cut_marker_index;

  Clipboard()
  {
  }

  bool canPasteTo(Optional<BodyIndex>, const SceneState &) const;
};


struct ObservedScene {
  struct TreeItemDescription {
    enum class Type {
      scene,
      body,
      box,
      line,
      marker,
      distance_error,
      translation,
      rotation,
      other
    };

    Type type = Type::other;
    Optional<BodyIndex> maybe_body_index;
    Optional<MarkerIndex> maybe_marker_index;
    Optional<size_t> maybe_box_index;
    Optional<size_t> maybe_line_index;
    Optional<DistanceErrorIndex> maybe_distance_error_index;
    bool has_rotation_ancestor = false;
    bool has_translation_ancesor = false;
  };

  Scene &scene;
  TreeWidget &tree_widget;
  SceneState scene_state;
  SceneHandles scene_handles;
  TreePaths tree_paths;
  Clipboard clipboard;
  std::function<void(SceneState&)> update_errors_function;

  ObservedScene(
    Scene &scene,
    TreeWidget &tree_widget,
    std::function<void(SceneState &)>
  );

  BodyIndex addBody(Optional<BodyIndex> maybe_parent_index);
  BoxIndex addBoxTo(BodyIndex);
  LineIndex addLineTo(BodyIndex);
  MarkerIndex addMarker(Optional<BodyIndex>);

  DistanceErrorIndex
    addDistanceError(
      Optional<MarkerIndex> start,
      Optional<MarkerIndex> end,
      Optional<BodyIndex>
    );

  VariableIndex addVariable();

  void cutBody(BodyIndex);
  void cutMarker(MarkerIndex);

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
  BodyIndex duplicateBody(BodyIndex body_index);
  BodyIndex duplicateBodyWithDistanceErrors(BodyIndex);
  MarkerIndex duplicateMarker(MarkerIndex);
  MarkerIndex duplicateMarkerWithDistanceError(MarkerIndex);


  static TreeItemDescription
    describePath(const TreePath &path, const TreePaths &tree_paths);

  static BodyIndex
    duplicateBody(BodyIndex, MarkerNameMap &, ObservedScene &);

  static void removingMarker(ObservedScene &, MarkerIndex);
  static void removingBody(ObservedScene &, BodyIndex);
  void removeBody(BodyIndex);
  void removeMarker(MarkerIndex);
  void removeBox(BodyIndex, BoxIndex);
  void removeLine(BodyIndex, LineIndex);
  static void clearClipboard(ObservedScene &);

  void replaceSceneStateWith(const SceneState &);
  bool canPasteTo(Optional<BodyIndex>);

  static void
  createBodyInTree(BodyIndex body_index, ObservedScene &observed_scene);

  static void
  createBodyInScene(BodyIndex body_index, ObservedScene &observed_scene);

  Optional<Scene::ManipulatorType> properManpiulatorForSelectedObject() const;

  static void attachProperDraggerToSelectedObject(ObservedScene &);

  static void
  selectBodyInTree(BodyIndex body_index, ObservedScene &observed_scene)
  {
    TreeWidget &tree_widget = observed_scene.tree_widget;
    TreePaths &tree_paths = observed_scene.tree_paths;
    tree_widget.selectItem(tree_paths.body(body_index).path);
  }

  void handleSceneStateChanged();
  void handleTreeSelectionChanged();
  void handleSceneSelectionChanged();

  static void
  createMarkerInScene(MarkerIndex marker_index, ObservedScene &observed_scene);

  static void
  createMarkerInTree(MarkerIndex marker_index, ObservedScene &observed_scene);
};
