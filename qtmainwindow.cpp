#include "qtmainwindow.hpp"

#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include "defaultscenestate.hpp"
#include "qtwidget.hpp"
#include "qtlayout.hpp"
#include "qtsplitter.hpp"
#include "qtslot.hpp"

using std::unique_ptr;
using std::make_unique;
using std::string;


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
  QString path = file_dialog.getSaveFileName(&parent,"Save Scene","./scenes/");

  if (path == "") {
    return {};
  }

  return path.toStdString();
}


static Optional<string> askForOpenPath(QWidget &parent)
{
  QFileDialog file_dialog;
  QString path = file_dialog.getOpenFileName(&parent,"Open Scene","./scenes/");

  if (path == "") {
    return {};
  }

  return path.toStdString();
}


template <typename CentralWidget>
CentralWidget &createCentralWidget(QMainWindow &main_window)
{
  QSplitter *splitter_ptr = new QSplitter;
  main_window.setCentralWidget(splitter_ptr);
  return *splitter_ptr;
}


static QtTreeWidget &createTree(QSplitter &splitter)
{
  return createWidget<QtTreeWidget>(splitter);
}


static void
addActionTo(
  QMenu &file_menu,
  string label,
  std::function<void()> new_function
)
{
  QAction &new_action = *file_menu.addAction(QString::fromStdString(label));
  QtSlot *new_slot_ptr = new QtSlot( &file_menu, new_function);
  new_slot_ptr->connectSignal(new_action, SIGNAL(triggered()));
}


Optional<string> QtMainWindow::View::askForSavePath()
{
  return ::askForSavePath(/*parent*/main_window);
}


Optional<string> QtMainWindow::View::askForOpenPath()
{
  return ::askForOpenPath(/*parent*/main_window);
}


QtMainWindow::QtMainWindow()
: splitter(createCentralWidget<QSplitter>(*this)),
  tree_widget(createTree(splitter)),
  controller((createGraphicsWindow(splitter, scene), view))
{
  QMenuBar &menu_bar = *menuBar();
  QMenu &file_menu = *menu_bar.addMenu("File");

  addActionTo(file_menu, "New...", [&](){ controller.newPressed(); });
  addActionTo(file_menu, "Open...", [&](){ controller.openPressed(); });
  addActionTo(file_menu, "Save...", [&](){ controller.savePressed(); });

  resize(1024,480);
  show();
}


void QtMainWindow::loadDefaultScene()
{
  controller.replaceSceneStateWith(defaultSceneState());
}
