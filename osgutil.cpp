#include "osgutil.hpp"

#include <cassert>
#include <iostream>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osgManipulator/Dragger>
#include "osgselectionhandler.hpp"
#include "osgcameramanipulator.hpp"

using std::cerr;
typedef osg::ref_ptr<osg::Geode> GeodePtr;


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

bool isDragger(osg::Node *node_ptr)
{
  assert(node_ptr);

  osgManipulator::Dragger *dragger_ptr =
    dynamic_cast<osgManipulator::Dragger *>(node_ptr);

  if (dragger_ptr) return true;
  if (node_ptr->getNumParents()==0) return false;
  return isDragger(node_ptr->getParent(0));
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
