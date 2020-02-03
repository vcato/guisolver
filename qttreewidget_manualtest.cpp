#include <QApplication>
#include "qttreewidget.hpp"
#include "vectorio.hpp"
#include "numericvaluelimits.hpp"

using std::cerr;


int main(int argc, char** argv)
{
  QApplication app(argc,argv);
  QtTreeWidget tree_widget;

  TreePath numeric_item_path = {0};
    tree_widget.createNumericItem(
      numeric_item_path,
      /*label*/"numeric",
      /*value*/7.5,
      /*minimum*/noMinimumNumericValue(),
      /*maximum*/noMaximumNumericValue(),
      /*digits_of_precision*/2
    );

  tree_widget.setItemInput(numeric_item_path, "input");

  tree_widget.createBoolItem(
    /*path*/{1},
    /*label*/"bool",
    /*value*/true
  );

  tree_widget.bool_item_value_changed_callback =
  [](const TreePath &path, bool new_value){
    cerr << "bool item value changed: "
      "path=" << path << ", new_value=" << new_value << "\n";
  };

  tree_widget.show();
  app.exec();
}
