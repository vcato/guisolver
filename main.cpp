#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include <QBoxLayout>
#include "osgscene.hpp"
#include "qtwidget.hpp"
#include "qttreewidget.hpp"
#include "qtlayout.hpp"
#include "mainwindowcontroller.hpp"

using std::cerr;


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


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(1024,480);
  main_window.show();
  OSGScene scene;

  QWidget central_widget;
  main_window.setCentralWidget(&central_widget);
  QBoxLayout &layout = createLayout<QHBoxLayout>(central_widget);
  QtTreeWidget &tree_widget = createWidget<QtTreeWidget>(layout);
  createGraphicsWindow(layout, scene);
  MainWindowController controller(scene, tree_widget);
  app.exec();
}
