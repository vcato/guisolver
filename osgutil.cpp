#include "osgutil.hpp"

#include <cassert>
#include <osg/AutoTransform>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>
#include <osg/ShapeDrawable>
#include <osgGA/TrackballManipulator>
#include <osgManipulator/TranslateAxisDragger>

typedef osg::ref_ptr<osg::Geode> GeodePtr;
typedef osg::ref_ptr<osgManipulator::TranslateAxisDragger>
  TranslateAxisDraggerPtr;
typedef osg::ref_ptr<osgManipulator::Dragger> DraggerPtr;
typedef osg::ref_ptr<osg::Group> GroupPtr;
typedef osg::ref_ptr<osg::AutoTransform> AutoTransformPtr;

namespace {
struct HomePosition {
  const osg::Vec3d eye;
  const osg::Vec3d center;
  const osg::Vec3d up;
};
}


namespace {
struct ViewParams {
  const HomePosition home_position;
  const bool disable_rotate;
  const bool vertical_axis_fixed;
};
}


NodePtr createFloor()
{
  size_t n = 8;
  osg::ref_ptr<osg::Vec3Array> vertices_ptr(new osg::Vec3Array((n+1)*4));
  osg::Vec3Array &vertices = *vertices_ptr;
  osg::ref_ptr<osg::Geometry> geometry_ptr(new osg::Geometry);
  float cx = n/2;
  float cz = n/2;

  for (size_t i=0; i<=n; ++i) {
    vertices[i*2+0].set(i-cx,0,0-cz);
    vertices[i*2+1].set(i-cx,0,n-cz);
  }
  for (size_t i=0; i<=n; ++i) {
    vertices[(n+1)*2+i*2+0].set(0-cx,0,i-cz);
    vertices[(n+1)*2+i*2+1].set(n-cx,0,i-cz);
  }
  geometry_ptr->setVertexArray(vertices_ptr);
  geometry_ptr->addPrimitiveSet(
    new osg::DrawArrays(osg::PrimitiveSet::LINES,0,(n+1)*4)
  );
  GeodePtr geode_ptr(new osg::Geode);
  geode_ptr->addDrawable(geometry_ptr);
  geode_ptr->getOrCreateStateSet()->setMode(
    GL_LIGHTING,osg::StateAttribute::OFF
  );
  return geode_ptr;
}

static GeodePtr createSphere()
{
  osg::ref_ptr<osg::Sphere> sphere_ptr = new osg::Sphere;

  osg::ref_ptr<osg::TessellationHints> sphere_tesselation_hints_ptr =
    new osg::TessellationHints;
  sphere_tesselation_hints_ptr->setDetailRatio(1.0);
  osg::ref_ptr<osg::ShapeDrawable> sphere_drawable_ptr =
    new osg::ShapeDrawable(sphere_ptr,sphere_tesselation_hints_ptr);

  GeodePtr sphere_geode_ptr = new osg::Geode;
  sphere_geode_ptr->addDrawable(sphere_drawable_ptr);

  osg::ref_ptr<osg::Material> material = new osg::Material;
  material->setColorMode(osg::Material::DIFFUSE);
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
  material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
  material->setShininess(osg::Material::FRONT_AND_BACK, 64.0f);
  sphere_geode_ptr->getOrCreateStateSet()->setAttributeAndModes(
    material, osg::StateAttribute::ON
  );
  sphere_geode_ptr->setName("Sphere Geode");
  assert(!sphere_geode_ptr->asGroup());

  return sphere_geode_ptr;
}

MatrixTransformPtr createMatrixTransform()
{
  return new osg::MatrixTransform;
}


static void setColor(GeodePtr geode_ptr,const osg::Vec3f &vec)
{
  osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);
  assert(drawable_ptr);
  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);
  assert(shape_drawable_ptr);
  shape_drawable_ptr->setColor(osg::Vec4f(vec.x(),vec.y(),vec.z(),1));
}

void
  addSphereTo(
    MatrixTransformPtr top_group_ptr,
    const osg::Vec3f &color,
    const osg::Matrix &transform
  )
{
  MatrixTransformPtr transform_ptr = createMatrixTransform();
  GeodePtr sphere_ptr = createSphere();
  setColor(sphere_ptr,color);
  transform_ptr->addChild(sphere_ptr);
  transform_ptr->setMatrix(transform);
  top_group_ptr->addChild(transform_ptr);
}

typedef osgGA::TrackballManipulator ManipulatorBase;

struct Manipulator : ManipulatorBase
{
  bool disable_rotate;

  Manipulator() : disable_rotate(false) { }

  virtual bool
    handle(
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter& us
    )
  {
    // Disable view manipulation unless we're holding down the ALT key.
    if (ea.getEventType()==osgGA::GUIEventAdapter::DRAG) {
      if (disable_rotate && ea.getButtonMask() & ea.LEFT_MOUSE_BUTTON) {
        return false;
      }
      if (!(ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)) {
        return false;
      }
    }
    return ManipulatorBase::handle(ea,us);
  }
};


static HomePosition homePosition(ViewType view_type)
{
  osg::Quat orient(worldRotation(),osg::Vec3d(1,0,0));

  switch (view_type) {
    case ViewType::front:
    {
      osg::Vec3d eye = orient*osg::Vec3d(0,0,10);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,1,0);
      return HomePosition{eye,center,up};
    }
    case ViewType::top:
    {
      osg::Vec3d eye = orient*osg::Vec3d(0,10,0);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,0,-1);
      return HomePosition{eye,center,up};
    }
    case ViewType::side:
    {
      osg::Vec3d eye = orient*osg::Vec3d(10,0,0);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,1,0);
      return HomePosition{eye,center,up};
    }
    case ViewType::free:
    {
      osg::Vec3d eye = orient*osg::Vec3d(10,10,10);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,1,0);
      return HomePosition{eye,center,up};
    }
  }

  assert(false);
}


static void
  configureCameraManipatulator(
    Manipulator &manipulator,
    const ViewParams &params
  )
{
  const HomePosition &home_position = params.home_position;

  manipulator.setHomePosition(
    home_position.eye,
    home_position.center,
    home_position.up
  );

  manipulator.setAllowThrow(false);
  manipulator.disable_rotate = params.disable_rotate;
  manipulator.setVerticalAxisFixed(params.vertical_axis_fixed);
}


static ViewParams viewParams(ViewType view_type)
{
  bool disable_rotate = view_type!=ViewType::free;
  bool vertical_axis_fixed = view_type==ViewType::free;
  HomePosition home_position = homePosition(view_type);
  ViewParams params = {home_position,disable_rotate,vertical_axis_fixed};
  return params;
}


CameraManipulatorPtr createCameraManipulator(ViewType view_type)
{
  osg::ref_ptr<Manipulator> manipulator_ptr(new Manipulator);
  configureCameraManipatulator(*manipulator_ptr,viewParams(view_type));
  return manipulator_ptr;
}


static void scaleAxisXY(osg::Node *x_axis_node,float scale_factor)
{
  osg::MatrixTransform *x_axis_transform = x_axis_node->asTransform()->asMatrixTransform();
  x_axis_transform->setMatrix(osg::Matrix::scale(scale_factor,scale_factor,1)*x_axis_transform->getMatrix());
}

bool isDragger(osg::Node *node_ptr)
{
  assert(node_ptr);
  osgManipulator::Dragger *dragger_ptr =
    dynamic_cast<osgManipulator::Dragger *>(node_ptr);
  if (dragger_ptr) return true;
  if (node_ptr->getNumParents()==0) return false;
  return isDragger(node_ptr->getParent(0));
}

#if 1
static NodePtr createDragger(MatrixTransformPtr transform_ptr)
{
  TranslateAxisDraggerPtr dragger_ptr =
    new osgManipulator::TranslateAxisDragger();
  dragger_ptr->setupDefaultGeometry();
  assert(dragger_ptr->getNumChildren()==3);
  float thickness = 4;
  scaleAxisXY(dragger_ptr->getChild(0),thickness);
  scaleAxisXY(dragger_ptr->getChild(1),thickness);
  scaleAxisXY(dragger_ptr->getChild(2),thickness);
  dragger_ptr->setHandleEvents(true);
  assert(isDragger(dragger_ptr));
  dragger_ptr->addTransformUpdating(transform_ptr);
  dragger_ptr->setMatrix(osg::Matrix::scale(2,2,2)*transform_ptr->getMatrix());
  return dragger_ptr;
}
#else
// Dragger that maintains its size relative to the screen
static NodePtr createDragger(MatrixTransformPtr transform_ptr)
{
  osg::ref_ptr<MyDragger> my_dragger_ptr(new MyDragger);
  TranslateAxisDraggerPtr dragger_ptr =
    new osgManipulator::TranslateAxisDragger();
  dragger_ptr->setParentDragger(my_dragger_ptr);
  dragger_ptr->setupDefaultGeometry();
  assert(dragger_ptr->getNumChildren()==3);
  float thickness = 4;
  scaleAxisXY(dragger_ptr->getChild(0),thickness);
  scaleAxisXY(dragger_ptr->getChild(1),thickness);
  scaleAxisXY(dragger_ptr->getChild(2),thickness);
  dragger_ptr->setHandleEvents(true);
  assert(isDragger(dragger_ptr));
  dragger_ptr->setParentDragger(my_dragger_ptr);
  //dragger_ptr->addTransformUpdating(transform_ptr);
  AutoTransformPtr mid_transform_ptr(new osg::AutoTransform);
  mid_transform_ptr->setAutoScaleToScreen(true);
  osg::Vec3d translation;
  osg::Quat rotation;
  osg::Vec3d scale;
  osg::Quat scale_orient;
  transform_ptr->getMatrix().decompose(translation,rotation,scale,scale_orient);
  mid_transform_ptr->setPosition(translation);
  my_dragger_ptr->addChild(mid_transform_ptr);
  mid_transform_ptr->addChild(dragger_ptr);
  my_dragger_ptr->addTransformUpdating(transform_ptr);
  dragger_ptr->setMatrix(osg::Matrix::scale(100,100,100));
  return my_dragger_ptr;
}
#endif

static osg::ref_ptr<osg::Group> createGroup()
{
  return new osg::Group();
}


static void addDraggerTo(MatrixTransformPtr transform_ptr)
{
  osg::Group *parent_ptr = transform_ptr->getParent(0);
  parent_ptr->removeChild(transform_ptr);
  GroupPtr group_ptr = createGroup();
  parent_ptr->addChild(group_ptr);
  NodePtr dragger_ptr = createDragger(transform_ptr);
  group_ptr->addChild(transform_ptr);
  group_ptr->addChild(dragger_ptr);
}

static void removeDraggerFrom(MatrixTransformPtr transform_ptr)
{
  osg::Group *group_ptr = transform_ptr->getParent(0);
  assert(group_ptr);
  osg::Group *parent_ptr = group_ptr->getParent(0);
  assert(group_ptr->getNumChildren()==2);
  group_ptr->removeChild(0,2);
  assert(parent_ptr);
  parent_ptr->removeChild(group_ptr);
  parent_ptr->addChild(transform_ptr);
}

static osg::ShapeDrawable *findShapeDrawable(osg::Node *node_ptr)
{
  osg::Geode *geode_ptr = node_ptr->asGeode();
  if (geode_ptr) {
    osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);
    if (drawable_ptr) {
      osg::ShapeDrawable *shape_drawable_ptr =
        dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);
      return shape_drawable_ptr;
    }
  }

  return 0;
}

static osg::MatrixTransform *transformParentOf(osg::Node *node_ptr)
{
  assert(node_ptr);
  osg::Node *parent_ptr = node_ptr->getParent(0);
  osg::Transform *transform_ptr = parent_ptr->asTransform();
  assert(transform_ptr);
  osg::MatrixTransform *matrix_transform_ptr =
    transform_ptr->asMatrixTransform();
  assert(matrix_transform_ptr);
  return matrix_transform_ptr;
}


void OSGSelectionHandler::nodeSelected(osg::Node *new_selected_node_ptr)
{
  if (selected_node_ptr) {
    osg::ShapeDrawable *shape_drawable_ptr =
      findShapeDrawable(selected_node_ptr);

    if (shape_drawable_ptr) {
      shape_drawable_ptr->setColor(old_color);
    }

    removeDraggerFrom(transformParentOf(selected_node_ptr));
    selected_node_ptr = 0;
  }

  selected_node_ptr = new_selected_node_ptr;

  if (selected_node_ptr) {
    osg::ShapeDrawable *shape_drawable_ptr =
      findShapeDrawable(selected_node_ptr);

    if (shape_drawable_ptr) {
      old_color = shape_drawable_ptr->getColor();
      osg::Vec4 selection_color(1,1,0,1);
      shape_drawable_ptr->setColor(selection_color);
    }

    addDraggerTo(transformParentOf(selected_node_ptr));
  }
}


