#include <QTreeWidgetItem>


extern QTreeWidgetItem& createChildItem(QTreeWidgetItem &parent_item);

extern QTreeWidgetItem&
  insertChildItem(QTreeWidgetItem &parent_item,int index);

extern void removeChildItem(QTreeWidgetItem &parent_item,int index);
