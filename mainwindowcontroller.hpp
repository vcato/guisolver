#ifndef MAINWINDOWCONTROLLER_HPP_
#define MAINWINDOWCONTROLLER_HPP_

#include <memory>
#include "scenestate.hpp"
#include "mainwindowview.hpp"


class MainWindowController {
  public:
    MainWindowController(MainWindowView &);
    ~MainWindowController();

    void replaceSceneStateWith(const SceneState &);
    void newPressed();
    void savePressed();
    void openPressed();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_ptr;
};


#endif /* MAINWINDOWCONTROLLER_HPP_ */
