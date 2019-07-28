#include "osgpickhandler.hpp"

#include <cassert>


OSGPickHandler::OSGPickHandler(
  osgViewer::View *view_ptr,
  OSGSelectionHandler *selection_handler_ptr
)
: view_ptr(view_ptr),
  selection_handler_ptr(selection_handler_ptr)
{
  assert(view_ptr);
  assert(selection_handler_ptr);
}


bool
  OSGPickHandler::handle(
    const osgGA::GUIEventAdapter& ea,
    osgGA::GUIActionAdapter&
  )
{
  if (ea.getEventType()!=ea.PUSH) return false;
  if (ea.getModKeyMask() & ea.MODKEY_ALT) return false;
  assert(view_ptr);
  typedef osgUtil::LineSegmentIntersector LineSegmentIntersector;
  typedef LineSegmentIntersector::Intersections Intersections;
  Intersections intersections;
  osg::Node *new_selected_node_ptr = 0;

  if (view_ptr->computeIntersections(ea,intersections)) {
    Intersections::const_iterator iter = intersections.begin();

    for (;iter!=intersections.end(); ++iter) {
      const LineSegmentIntersector::Intersection &intersection = *iter;
      osg::NodePath node_path = intersection.nodePath;
      //printNodePath(cerr,node_path);
      osg::Node *node_ptr = node_path.back();
      new_selected_node_ptr = node_ptr;
      break;
    }
  }

  if (!new_selected_node_ptr || !isDragger(new_selected_node_ptr)) {
    selection_handler_ptr->nodeSelected(new_selected_node_ptr);
  }

  return true;
}
