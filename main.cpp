#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include <QBoxLayout>
#include "osgscene.hpp"
#include "qtwidget.hpp"
#include "qttreewidget.hpp"
#include "qtlayout.hpp"
#include "streamvector.hpp"
#include "mainwindowcontroller.hpp"

using std::cerr;
using std::string;


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QMainWindow main_window;
  main_window.resize(1024,480);
  main_window.show();
  OSGScene scene;

  GraphicsWindowPtr graphics_window_ptr =
    scene.createGraphicsWindow(ViewType::free);

  // The default size here doesn't seem to really work.  It defaults to
  // a size that is much smaller than 640x480, but if we don't set the default
  // size at all, then it ends up collapsing to 0 size.
  graphics_window_ptr->getGLWidget()->setDefaultSize(QSize(640,480));

  QWidget central_widget;
  main_window.setCentralWidget(&central_widget);
  QBoxLayout &layout = createLayout<QHBoxLayout>(central_widget);

  QtTreeWidget &tree_widget = createWidget<QtTreeWidget>(layout);

  tree_widget.spin_box_item_value_changed_function =
    [](const TreePath &path, int value) {
      cerr << "Handling spin_box_item_value_changed_function\n";
      cerr << "  path: " << path << "\n";
      cerr << "  value: " << value << "\n";
    };

  layout.addWidget(graphics_window_ptr->getGLWidget());
  MainWindowController controller(scene,tree_widget);
  app.exec();
}
