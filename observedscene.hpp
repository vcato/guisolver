#include "taggedvalue.hpp"
#include "transform.hpp"
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
  MarkerIndex addMarker(Optional<BodyIndex>);

  DistanceErrorIndex
    addDistanceError(Optional<MarkerIndex> start, Optional<MarkerIndex> end);

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
  BodyIndex duplicateBody(BodyIndex body_index);

  static BodyIndex
  duplicateBody(BodyIndex body_index, MarkerNameMap &, ObservedScene &);

  BodyIndex duplicateBodyWithDistanceErrors(BodyIndex body_index);

  static void removingMarker(ObservedScene &, MarkerIndex);
  static void removingBody(ObservedScene &, BodyIndex);
  static void removeBody(ObservedScene &, BodyIndex);
  static void clearClipboard(ObservedScene &);

  void replaceSceneStateWith(const SceneState &);
  bool canPasteTo(Optional<BodyIndex>);

  static void
  createBodyInTree(BodyIndex body_index, ObservedScene &observed_scene);

  static void
  createBodyInScene(BodyIndex body_index, ObservedScene &observed_scene);

  static void attachProperDraggerToSelectedObject(ObservedScene &);

  static void
  selectBodyInTree(BodyIndex body_index, ObservedScene &observed_scene)
  {
    TreeWidget &tree_widget = observed_scene.tree_widget;
    TreePaths &tree_paths = observed_scene.tree_paths;
    tree_widget.selectItem(tree_paths.body(body_index).path);
  }

  static void handleTreeSelectionChanged(ObservedScene &);

  static void handleSceneSelectionChanged(ObservedScene &);

  static void
  createMarkerInScene(MarkerIndex marker_index, ObservedScene &observed_scene);

  static void
  createMarkerInTree(MarkerIndex marker_index, ObservedScene &observed_scene);
};
