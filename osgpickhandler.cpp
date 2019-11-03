#include "osgpickhandler.hpp"

#include <cassert>
#include <iostream>
#include "intersector.hpp"

#define USE_INTERSECTOR 1
#define USE_POLYTOPE_INTERSECTOR 0

using std::cerr;


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


static double
  lerp(double x,double from1, double from2, double to1, double to2)
{
  return (x - from1)/(from2 - from1) * (to2 - to1) + to1;
}


#if USE_INTERSECTOR
static osg::Node *
  findIntersection(
    osgViewer::View &view,
    const osgGA::GUIEventAdapter& event_adapter
  )
{
  osg::ref_ptr<const osg::Camera> camera_ptr = view.getCamera();

  if (!camera_ptr.valid()) {
    return nullptr;
  }

  float local_x, local_y = 0.0;

  float norm_x = event_adapter.getXnormalized();
  float norm_y = event_adapter.getYnormalized();
  const osg::Viewport* viewport_ptr = camera_ptr->getViewport();
  assert(viewport_ptr);
  const osg::Viewport &viewport = *viewport_ptr;
  double vx = viewport.x();
  double vy = viewport.y();
  double mx = lerp(norm_x, -1, 1, vx, vx + viewport.width());
  double my = lerp(norm_y, -1, 1, vy, vy + viewport.height());
#if 1
  osg::ref_ptr<const osg::Camera> camera_t =
    view.getCameraContainingPosition(mx, my, local_x, local_y);

  if (camera_t.valid())
    camera_ptr = camera_t.get();

  if (!camera_ptr.valid())
    return nullptr;
#endif

  osgUtil::Intersector::CoordinateFrame cf;
  if (camera_ptr->getViewport())
    cf = osgUtil::Intersector::WINDOW;
  else
    cf = osgUtil::Intersector::PROJECTION;

  // The sensible area for the pick operation is handed in WINDOW coordinates,
  // int this case set to be 4 units from pick point (local_x, local_y) in all directions.
  osg::ref_ptr<IntersectorPrivate> picker =
    new IntersectorPrivate(cf, local_x, local_y, 4.f);

  osgUtil::IntersectionVisitor iv(picker.get());

  const_cast<osg::Camera*>(camera_ptr.get())->accept(iv);

  if (picker->containsIntersections()) {
    const IntersectorPrivate::Intersection& intersection =
      picker->getFirstIntersection();

    osg::NodePath node_path = intersection.nodePath;
    osg::Node *node_ptr = node_path.back();
    return node_ptr;
  }

  return nullptr;
}
#endif


#if USE_POLYTOPE_INTERSECTOR
static osg::Node *
  findPolytopeIntersection(
    const osgGA::GUIEventAdapter& event_adapter,
    osgViewer::View &view
  )
{
  osgUtil::PolytopeIntersector::Intersections intersections;
  osg::Viewport* viewport_ptr = view.getCamera()->getViewport();
  assert(viewport_ptr);
  osg::Viewport &viewport = *viewport_ptr;
  float norm_x = event_adapter.getXnormalized();
  float norm_y = event_adapter.getYnormalized();
  double vx = viewport.x();
  double vy = viewport.y();
  double mx = lerp(norm_x, -1, 1, vx, vx + viewport.width());
  double my = lerp(norm_y, -1, 1, vy, vy + viewport.height());
  double w = 5.0f; // 5 pixels
  double h = 5.0f;
  double min_x = mx-w;
  double min_y = my-h;
  double max_x = mx+w;
  double max_y = my+h;

  auto *polytope_intersector_ptr =
    new osgUtil::PolytopeIntersector(
      osgUtil::PolytopeIntersector::WINDOW,
      min_x,
      min_y,
      max_x,
      max_y
    );

  osgUtil::PolytopeIntersector &intersector = *polytope_intersector_ptr;

  intersector.setIntersectionLimit(
    //osgUtil::Intersector::LIMIT_ONE_PER_DRAWABLE
    osgUtil::Intersector::LIMIT_NEAREST
  );

  osgUtil::IntersectionVisitor intersection_visitor(polytope_intersector_ptr);
  view.getCamera()->accept(intersection_visitor);

  cerr << "polytope_intersector_ptr->containsIntersections(): " <<
    intersector.containsIntersections() << "\n";

  if (intersector.containsIntersections()) {
    osgUtil::PolytopeIntersector::Intersections &intersections =
      intersector.getIntersections();

    cerr << intersections.size() << " intersections.\n";

    for (auto &intersection : intersections) {
      cerr <<
        "node=" << intersection.nodePath.back() << ", "
        "dist=" << intersection.distance << ", "
        "max=" << intersection.maxDistance << "\n";
    }

    osgUtil::PolytopeIntersector::Intersection intersection =
      intersector.getFirstIntersection();

    cerr << "matched intersection.distance: " << intersection.distance << "\n";
    osg::NodePath node_path = intersection.nodePath;
    return node_path.back();
  }

  return nullptr;
}
#endif


#if !USE_INTERSECTOR
static osg::Node *
  findLineSegmentIntersection(
    osgViewer::View &view,
    const osgGA::GUIEventAdapter& event_adapter
  )
{
  typedef osgUtil::LineSegmentIntersector LineSegmentIntersector;
  typedef LineSegmentIntersector::Intersections Intersections;
  Intersections intersections;

  if (view.computeIntersections(event_adapter,intersections)) {
    cerr << intersections.size() << " intersections found.\n";
    Intersections::const_iterator iter = intersections.begin();

    for (;iter!=intersections.end(); ++iter) {
      const LineSegmentIntersector::Intersection &intersection = *iter;
      osg::NodePath node_path = intersection.nodePath;
      osg::Node *node_ptr = node_path.back();
      return node_ptr;
    }
  }
  else {
    cerr << "Unable to compute intersections.\n";
  }

  return nullptr;
}
#endif


bool
  OSGPickHandler::handle(
    const osgGA::GUIEventAdapter& event_adapter,
    osgGA::GUIActionAdapter&
  )
{
  if (event_adapter.getEventType()!=event_adapter.PUSH) return false;
  if (event_adapter.getModKeyMask() & event_adapter.MODKEY_ALT) return false;

  assert(view_ptr);

#if 0
  osg::Node *new_selected_node_ptr =
    findLineSegmentIntersection(*view_ptr, event_adapter);
  cerr << "line intersection node: " << new_selected_node_ptr << "\n";
#endif
#if 1
  osg::Node *new_selected_node_ptr =
    findIntersection(*view_ptr, event_adapter);
#endif

#if 0
  osg::Node *new_selected_node_ptr =
    findPolytopeIntersection(event_adapter, *view_ptr);
#endif

  if (!new_selected_node_ptr) {
    selection_handler_ptr->nodeClicked(nullptr);
  }
  else if (!isDragger(new_selected_node_ptr)) {
    selection_handler_ptr->nodeClicked(new_selected_node_ptr);
  }
  else {
    cerr << "Selected a dragger\n";
  }

  return true;
}
