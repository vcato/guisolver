#ifndef MAINWINDOWVIEW_HPP_
#define MAINWINDOWVIEW_HPP_

#include <string>
#include "optional.hpp"
#include "treewidget.hpp"
#include "scene.hpp"


struct MainWindowView {
  virtual Optional<std::string> askForSavePath() = 0;
  virtual Optional<std::string> askForOpenPath() = 0;
  virtual TreeWidget &treeWidget() = 0;
  virtual Scene &scene() = 0;
};


#endif /* MAINWINDOWVIEW_HPP_ */
