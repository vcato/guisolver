#include <QApplication>
#include "qttreewidget.hpp"


int main(int argc, char** argv)
{
  QApplication app(argc,argv);
  QtTreeWidget tree_widget;

  tree_widget.createNumericItem(
    /*path*/{0},
    /*label*/QtTreeWidget::LabelProperties{"numeric"},
    /*value*/7.5,
    /*minimum*/0,
    /*maximum*/100,
    /*digits_of_precision*/2
  );

  tree_widget.show();
  app.exec();
}
