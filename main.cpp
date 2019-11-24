#include <iostream>
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
#include "mainwindowcontroller.hpp"
#include "optional.hpp"
#include "defaultscenestate.hpp"

#define ADD_SAVE 0

using std::cerr;
using std::string;


static void createGraphicsWindow(QBoxLayout &layout, OSGScene &scene)
{
  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  graphics_window_ptr->getGLWidget()->setDefaultSize(QSize(640,480));
    // The default size here doesn't seem to really work.  It defaults to
    // a size that is much smaller than 640x480, but if we don't set the default
    // size at all, then it ends up collapsing to 0 size.

  layout.addWidget(graphics_window_ptr->getGLWidget());
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


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(1024,480);
  main_window.show();
  OSGScene scene;

  QWidget central_widget;
  main_window.setCentralWidget(&central_widget);

  QtSlot save_slot(
    [&](){
      Optional<string> maybe_path = askForSavePath(/*parent*/main_window);

      if (!maybe_path) {
        // Cancelled
      }
      else {
        // saveScene(scene_state, *maybe_path);
      }
    }
  );

  {
    QMenuBar &menu_bar = *main_window.menuBar();
#if ADD_SAVE
    QMenu &file_menu = *
#endif
      menu_bar.addMenu("File");
#if ADD_SAVE
    QAction &save_action = *file_menu.addAction("Save...");
    save_slot.connectSignal(save_action, SIGNAL(triggered()));
#endif
  }
  QBoxLayout &layout = createLayout<QHBoxLayout>(central_widget);
  QtTreeWidget &tree_widget = createWidget<QtTreeWidget>(layout);
  createGraphicsWindow(layout, scene);
  SceneState scene_state(defaultSceneState());
  MainWindowController controller(scene_state, scene, tree_widget);
  app.exec();
}
