#include "qtmenu.hpp"

#include <QMenu>
#include "qtslot.hpp"


using std::string;


QAction& createAction(QMenu &menu,const string &label)
{
  QAction *add_pass_action_ptr = new QAction(QString::fromStdString(label),0);
  menu.addAction(add_pass_action_ptr);
  return *add_pass_action_ptr;
}


void
  createAction(
    QMenu &menu,
    const std::string &label,
    const std::function<void()> &function
  )
{
  QAction &action = createAction(menu,label);
  auto slot_ptr = new QtSlot(&menu,function);
  slot_ptr->connectSignal(action,SIGNAL(triggered()));
}
