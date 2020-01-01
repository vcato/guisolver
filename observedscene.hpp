#include "taggedvalue.hpp"
#include "transform.hpp"
#include "scene.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "treepaths.hpp"


struct Clipboard {
  Optional<BodyIndex> maybe_cut_body_index;

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

  ObservedScene(Scene &scene, TreeWidget &tree_widget);

  static BodyIndex
  addBody(Optional<BodyIndex> maybe_parent_index, ObservedScene &);

  static MarkerIndex addMarker(ObservedScene &, Optional<BodyIndex>);
  static void removingMarker(ObservedScene &, MarkerIndex);
  static void removingBody(ObservedScene &, BodyIndex);
  static void removeBody(ObservedScene &, BodyIndex);
  static void cutBody(ObservedScene &, BodyIndex body_index);
  static void clearClipboard(ObservedScene &);

  void replaceSceneStateWith(const SceneState &);
  bool canPasteTo(Optional<BodyIndex>);

  static BodyIndex
    pasteGlobal(
      Optional<BodyIndex> maybe_new_parent_body_index,
      ObservedScene &observed_scene
    );

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

  static void selectBody(BodyIndex body_index, ObservedScene &observed_scene)
  {
    selectBodyInTree(body_index, observed_scene);
    handleTreeSelectionChanged(observed_scene);
  }

  static void handleSceneSelectionChanged(ObservedScene &);

  static void
  createMarkerInScene(MarkerIndex marker_index, ObservedScene &observed_scene);

  static void
  createMarkerInTree(MarkerIndex marker_index, ObservedScene &observed_scene);
};
