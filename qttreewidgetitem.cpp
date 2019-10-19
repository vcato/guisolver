#include "qttreewidgetitem.hpp"


QTreeWidgetItem& createChildItem(QTreeWidgetItem &parent_item)
{
  QTreeWidgetItem *item_ptr = new QTreeWidgetItem;
  parent_item.addChild(item_ptr);
  QTreeWidgetItem &item = *item_ptr;
  item.setExpanded(true);
  return item;
}


QTreeWidgetItem& insertChildItem(QTreeWidgetItem &parent_item,int index)
{
  QTreeWidgetItem *item_ptr = new QTreeWidgetItem;
  parent_item.insertChild(index,item_ptr);
  QTreeWidgetItem &item = *item_ptr;
  item.setExpanded(true);
  return item;
}


void removeChildItem(QTreeWidgetItem &parent_item,int index)
{
  parent_item.removeChild(parent_item.child(index));
}
