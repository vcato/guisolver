#include "osgutil.hpp"

#include <cassert>
#include <iostream>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osgManipulator/Dragger>
#include <osgGA/TrackballManipulator>
#include "osgselectionhandler.hpp"

using std::cerr;
typedef osg::ref_ptr<osg::Geode> GeodePtr;


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

MatrixTransformPtr createMatrixTransform()
{
  return new osg::MatrixTransform;
}


typedef osgGA::TrackballManipulator ManipulatorBase;


namespace {
struct CameraManipulator : ManipulatorBase
{
  bool disable_rotate;

  CameraManipulator() : disable_rotate(false) { }

  virtual bool
    handle(
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter& us
    )
  {
    // Disable view manipulation unless we're holding down the ALT key.
    auto event_type = ea.getEventType();

    if (event_type == ea.PUSH || event_type == ea.DRAG) {
      if (disable_rotate && ea.getButtonMask() & ea.LEFT_MOUSE_BUTTON) {
        return false;
      }

      if (!(ea.getModKeyMask() & ea.MODKEY_ALT)) {
        return false;
      }
    }

    return ManipulatorBase::handle(ea,us);
  }
};
}


static HomePosition homePosition(ViewType view_type)
{
  osg::Quat orient = worldRotation();

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
    CameraManipulator &manipulator,
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
  osg::ref_ptr<CameraManipulator> manipulator_ptr(new CameraManipulator);
  configureCameraManipatulator(*manipulator_ptr,viewParams(view_type));
  return manipulator_ptr;
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

osg::MatrixTransform &parentTransform(osg::MatrixTransform &t)
{
  const osg::MatrixTransform &const_t = t;
  const osg::MatrixTransform &const_result = parentTransform(const_t);
  return const_cast<osg::MatrixTransform&>(const_result);
}


const osg::MatrixTransform &parentTransform(const osg::MatrixTransform &t)
{
  const osg::Node *parent_ptr = t.getParent(0);
  assert(parent_ptr);
  const osg::Transform *transform2_ptr = parent_ptr->asTransform();

  if (!transform2_ptr) {
    cerr << "Parent of " << t.getName() << " is not a transform\n";
    cerr << "  it is a " << parent_ptr->className() << "\n";
  }

  assert(transform2_ptr);
  const osg::MatrixTransform *matrix2_transform_ptr =
    transform2_ptr->asMatrixTransform();
  assert(matrix2_transform_ptr);
  return *matrix2_transform_ptr;
}


osg::Matrix
  compose(
    osg::Vec3f translation,
    osg::Quat rotation,
    osg::Vec3f scale,
    osg::Quat so
  )
{
  osg::Matrix result = osg::Matrix::identity();
  result.setTrans(translation);
  result.setRotate(rotation);
  result.preMultRotate(so);
  result.preMultScale(scale);
  result.preMultRotate(so.conj());
  return result;
}


void setScale(osg::Matrix &m, const osg::Vec3f &new_scale)
{
  osg::Vec3f translation;
  osg::Quat rotation;
  osg::Vec3f scale;
  osg::Quat so;
  m.decompose(translation, rotation, scale, so);
  m = compose(translation,rotation,new_scale,so);
}


void
  setCoordinateAxesOf(
    osg::Matrix &m,
    const osg::Vec3f &x,
    const osg::Vec3f &y,
    const osg::Vec3f &z
  )
{
  m(0,0) = x.x();
  m(0,1) = x.y();
  m(0,2) = x.z();
  m(1,0) = y.x();
  m(1,1) = y.y();
  m(1,2) = y.z();
  m(2,0) = z.x();
  m(2,1) = z.y();
  m(2,2) = z.z();
}


OSGCoordinateAxes coordinateAxesOf(const osg::Matrix &m)
{
  float xx = m(0,0);
  float xy = m(0,1);
  float xz = m(0,2);
  float yx = m(1,0);
  float yy = m(1,1);
  float yz = m(1,2);
  float zx = m(2,0);
  float zy = m(2,1);
  float zz = m(2,2);
  osg::Vec3f x = {xx,xy,xz};
  osg::Vec3f y = {yx,yy,yz};
  osg::Vec3f z = {zx,zy,zz};
  return {x,y,z};
}
