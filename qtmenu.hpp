#include <functional>
#include "optional.hpp"


class QMenuBar;
class QAction;
class QMenu;


extern QAction&
  createAction(
    QMenu &menu,
    const std::string &label,
    Optional<bool> optional_checked_state = {}
  );

extern void
  createAction(
    QMenu &menu,
    const std::string &label,
    const std::function<void()> &function,
    Optional<bool> optional_checked_state = {}
  );
