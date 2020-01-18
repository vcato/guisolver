#include <QApplication>
#include "qtmainwindow.hpp"


int main(int argc,char** argv)
{
  QApplication app(argc,argv);
  QtMainWindow main_window;
  main_window.loadDefaultScene();
  app.exec();
}
