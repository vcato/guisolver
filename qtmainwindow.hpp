#include <QMainWindow>
#include <QSplitter>
#include "mainwindowview.hpp"
#include "osgscene.hpp"
#include "qttreewidget.hpp"
#include "mainwindowcontroller.hpp"


class QtMainWindow : public QMainWindow {
  public:
    QtMainWindow();
    void loadDefaultScene();

  private:
    using FilePath = std::string;

    struct View : MainWindowView {
      QtMainWindow &main_window;
      View(QtMainWindow &main_window) : main_window(main_window) { }
      Optional<FilePath> askForSavePath() override;
      Optional<FilePath> askForOpenPath() override;
      TreeWidget &treeWidget() override { return main_window.tree_widget; }
      Scene &scene() override { return main_window.scene; }
    };

    OSGScene scene;
    QSplitter &splitter;
    QtTreeWidget &tree_widget;
    View view{*this};
    MainWindowController controller;
};
