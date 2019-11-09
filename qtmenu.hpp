#include <functional>


class QMenuBar;
class QAction;
class QMenu;


extern QAction& createAction(QMenu &menu,const std::string &label);

extern void
  createAction(
    QMenu &menu,
    const std::string &label,
    const std::function<void()> &function
  );
