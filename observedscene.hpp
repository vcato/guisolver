#include "taggedvalue.hpp"
#include "transform.hpp"
#include "scene.hpp"
#include "treewidget.hpp"
#include "scenestate.hpp"
#include "scenehandles.hpp"
#include "treepaths.hpp"


#define NEW_CUT_BEHAVIOR 1


struct Clipboard {
#if !NEW_CUT_BEHAVIOR
  TaggedValue clipboard_tagged_value;
  Transform clipboard_transform;
#else
  Optional<BodyIndex> maybe_cut_body_index;
#endif

  Clipboard()
#if !NEW_CUT_BEHAVIOR
  : clipboard_tagged_value("clipboard")
#endif
  {
  }

  bool canPasteTo(BodyIndex, const SceneState &) const;
};


struct ObservedScene {
  Scene &scene;
  TreeWidget &tree_widget;
  SceneState scene_state;
  SceneHandles scene_handles;
  TreePaths tree_paths;

  ObservedScene(Scene &scene, TreeWidget &tree_widget);

  static BodyIndex
  addBody(Optional<BodyIndex> maybe_parent_index, ObservedScene &);

  static MarkerIndex addMarker(ObservedScene &, Optional<BodyIndex>);
  static void removingMarker(ObservedScene &, MarkerIndex);
  static void removingBody(ObservedScene &, BodyIndex);
  static void removeBody(ObservedScene &, BodyIndex);
  static void cutBody(ObservedScene &, BodyIndex body_index, Clipboard &);

  static void
    clearClipboard(
      ObservedScene &observed_scene,
      Clipboard &clipboard
    );

  static BodyIndex
    pasteGlobal(
      Optional<BodyIndex> maybe_new_parent_body_index,
      Clipboard &clipboard,
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
