#ifndef OSGSELECTIONHANDLER_HPP_
#define OSGSELECTIONHANDLER_HPP_

#include <osg/Node>


struct OSGSelectionHandler {
  virtual void nodeClicked(osg::Node *new_selected_node_ptr) = 0;
};


#endif /* OSGSELECTIONHANDLER_HPP_ */
