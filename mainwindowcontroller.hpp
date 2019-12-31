#include <memory>
#include "scenestate.hpp"
#include "scene.hpp"
#include "treewidget.hpp"


class MainWindowController {
  public:
    MainWindowController(Scene &,TreeWidget &);
    ~MainWindowController();

    void replaceSceneStateWith(const SceneState &);
    const SceneState &sceneState();

  private:
    struct Impl;

    std::unique_ptr<Impl> impl_ptr;
};
