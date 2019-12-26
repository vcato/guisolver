#include <iostream>
#include <fstream>
#include <memory>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QBoxLayout>
#include <QFileDialog>
#include "osgscene.hpp"
#include "qtwidget.hpp"
#include "qttreewidget.hpp"
#include "qtlayout.hpp"
#include "qtslot.hpp"
#include "qtsplitter.hpp"
#include "mainwindowcontroller.hpp"
#include "optional.hpp"
#include "defaultscenestate.hpp"
#include "scenestateio.hpp"

using std::cerr;
using std::string;
using std::ostream;
using std::ofstream;
using std::ifstream;
using std::unique_ptr;
using std::make_unique;


namespace {
struct ParentWidgetRef {
  struct ParentInterface {
    virtual void addWidget(QWidget &) = 0;
  };

  template <typename T>
  struct Parent : ParentInterface {
    T &parent;

    Parent(T& parent) : parent(parent) {}

    void addWidget(QWidget &widget) override
    {
      parent.addWidget(&widget);
    }
  };

  template <typename T>
  ParentWidgetRef(T &arg)
  : parent_ptr(make_unique<Parent<T>>(arg))
  {
  }

  void addWidget(QWidget &widget)
  {
    parent_ptr->addWidget(widget);
  }

  unique_ptr<ParentInterface> parent_ptr;
};
}


static void createGraphicsWindow(ParentWidgetRef parent, OSGScene &scene)
{
  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  graphics_window_ptr->getGLWidget()->setDefaultSize(QSize(640,480));
    // The default size here doesn't seem to really work.  It defaults to
    // a size that is much smaller than 640x480, but if we don't set the default
    // size at all, then it ends up collapsing to 0 size.

  parent.addWidget(*graphics_window_ptr->getGLWidget());
}


static Optional<string> askForSavePath(QWidget &parent)
{
  QFileDialog file_dialog;
  QString path = file_dialog.getSaveFileName(&parent,"Save Scene",".");

  if (path == "") {
    return {};
  }

  return path.toStdString();
}


static Optional<string> askForOpenPath(QWidget &parent)
{
  QFileDialog file_dialog;
  QString path = file_dialog.getOpenFileName(&parent,"Open Scene",".");

  if (path == "") {
    return {};
  }

  return path.toStdString();
}


static void saveScene(SceneState &scene_state, const string &path)
{
  ofstream stream(path);

  if (!stream) {
    assert(false); // not implemented
  }

  printSceneStateOn(stream, scene_state);

  if (!stream) {
    assert(false); // not implemented
  }
}


static void loadScene(SceneState &scene_state, const string &path)
{
  ifstream stream(path);

  if (!stream) {
    assert(false); // not implemented
  }

  Expected<SceneState> scan_result = scanSceneStateFrom(stream);

  if (scan_result.isError()) {
    assert(false); // not implemented
  }

  scene_state = scan_result.asValue();
}


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(1024,480);
  main_window.show();
  OSGScene scene;
  SceneState scene_state(defaultSceneState());

  QSplitter splitter;
  main_window.setCentralWidget(&splitter);
  QtTreeWidget &tree_widget = createWidget<QtTreeWidget>(splitter);
  createGraphicsWindow(splitter, scene);
  MainWindowController controller(scene_state, scene, tree_widget);

  QtSlot save_slot(
    [&](){
      Optional<string> maybe_path = askForSavePath(/*parent*/main_window);

      if (!maybe_path) {
        // Cancelled
      }
      else {
        saveScene(scene_state, *maybe_path);
      }
    }
  );

  QtSlot open_slot(
    [&](){
      Optional<string> maybe_path = askForOpenPath(/*parent*/main_window);

      if (!maybe_path) {
        // Cancelled
      }
      else {
        loadScene(scene_state, *maybe_path);
        controller.notifySceneStateChanged();
      }
    }
  );

  {
    QMenuBar &menu_bar = *main_window.menuBar();
    QMenu &file_menu = *menu_bar.addMenu("File");
    QAction &open_action = *file_menu.addAction("Open...");
    QAction &save_action = *file_menu.addAction("Save...");
    save_slot.connectSignal(save_action, SIGNAL(triggered()));
    open_slot.connectSignal(open_action, SIGNAL(triggered()));
  }
  app.exec();
}
