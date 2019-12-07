#include "qtmenu.hpp"

#include <QMenu>
#include "qtslot.hpp"


using std::string;


QAction&
  createAction(
    QMenu &menu,
    const string &label,
    Optional<bool> optional_checked_state
  )
{
  QAction *add_pass_action_ptr = new QAction(QString::fromStdString(label),0);

  if (optional_checked_state) {
    add_pass_action_ptr->setCheckable(true);
    add_pass_action_ptr->setChecked(*optional_checked_state);
  }

  menu.addAction(add_pass_action_ptr);
  return *add_pass_action_ptr;
}


void
  createAction(
    QMenu &menu,
    const std::string &label,
    const std::function<void()> &function,
    Optional<bool> optional_checked_state
  )
{
  QAction &action = createAction(menu, label, optional_checked_state);
  auto slot_ptr = new QtSlot(&menu,function);
  slot_ptr->connectSignal(action,SIGNAL(triggered()));
}
