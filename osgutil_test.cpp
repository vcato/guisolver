#include "osgutil.hpp"

#include <cassert>
#include <random>
#include <iostream>
#include <osg/io_utils>

using std::cerr;


static osg::Matrix createRandomMatrix()
{
  std::mt19937 engine(/*seed*/1);
  auto m = osg::Matrix::identity();

  auto randomFloat =
    [&] { return std::uniform_real_distribution<float>(-1,1)(engine); };

  for (int i=0; i!=3; ++i) {
    for (int j=0; j!=4; ++j) {
      m(j,i) = randomFloat();
    }
  }

  return m;
}


static void assertNear(float a,float b,float tolerance)
{
  float delta = std::abs(a-b);
  bool is_near = (delta <= tolerance);

  if (!is_near) {
    cerr << "a: " << a << "\n";
    cerr << "b: " << b << "\n";
    cerr << "delta: " << delta << "\n";
  }

  assert(is_near);
}


static void assertNear(const osg::Quat &a,const osg::Quat &b,float tolerance)
{
  assertNear(a.x(),b.x(),tolerance);
  assertNear(a.y(),b.y(),tolerance);
  assertNear(a.z(),b.z(),tolerance);
  assertNear(a.w(),b.w(),tolerance);
}


static void assertNear(const osg::Vec3f &a,const osg::Vec3f &b,float tolerance)
{
  assertNear(a.x(),b.x(),tolerance);
  assertNear(a.y(),b.y(),tolerance);
  assertNear(a.z(),b.z(),tolerance);
}


static void assertNear(const osg::Matrix &a,const osg::Matrix &b,float tolerance)
{
  for (int i=0; i!=4; ++i) {
    for (int j=0; j!=4; ++j) {
      assertNear(a(i,j), b(i,j), tolerance);
    }
  }
}


static void testCompose()
{
  osg::Matrix old_mat = createRandomMatrix();

  osg::Vec3f old_translation;
  osg::Quat old_rotation;
  osg::Vec3f old_scale;
  osg::Quat old_scale_orient;
  old_mat.decompose(old_translation, old_rotation, old_scale, old_scale_orient);

  osg::Matrix new_mat =
    compose(old_translation,old_rotation,old_scale,old_scale_orient);

  osg::Vec3f new_translation;
  osg::Quat new_rotation;
  osg::Vec3f new_scale;
  osg::Quat new_scale_orient;
  new_mat.decompose(new_translation, new_rotation, new_scale, new_scale_orient);
  assertNear(old_mat, new_mat, 1e-7);
}


static void testSetScale()
{
  osg::Matrix old_mat = createRandomMatrix();

  osg::Vec3f old_translation;
  osg::Quat old_rotation;
  osg::Vec3f old_scale;
  osg::Quat old_scale_orient;
  old_mat.decompose(old_translation, old_rotation, old_scale, old_scale_orient);

  auto desired_scale = osg::Vec3f{1,2,3};
  osg::Matrix new_mat = old_mat;
  setScale(new_mat,desired_scale);

  osg::Vec3f new_translation;
  osg::Quat new_rotation;
  osg::Vec3f new_scale;
  osg::Quat new_scale_orient;
  new_mat.decompose(new_translation, new_rotation, new_scale, new_scale_orient);
  assertNear(new_rotation, old_rotation, 0);
  assertNear(new_translation, old_translation, 0);
  assertNear(new_scale, desired_scale, 0);
  assertNear(new_scale_orient, old_scale_orient, 0);
}


static void testSetCoordinateAxes()
{
  osg::Matrix mat = osg::Matrix::identity();
  mat.setTrans(osg::Vec3f(1,2,3));
  auto x_axis = osg::Vec3f(1, 0,0);
  auto y_axis = osg::Vec3f(0, 0,1);
  auto z_axis = osg::Vec3f(0,-1,0);

  setCoordinateAxes(mat,x_axis,y_axis,z_axis);

  osg::Vec3f trans = mat.getTrans();
  assert(trans == osg::Vec3f(1,2,3));
  osg::Quat rot = mat.getRotate();
  osg::Quat::value_type angle = 0;
  osg::Vec3f axis(0,0,0);
  rot.getRotate(angle,axis);
  assertNear(angle,M_PI/2,0);
  assertNear(axis,osg::Vec3f(1,0,0),0);
}


int main()
{
  testCompose();
  testSetScale();
  testSetCoordinateAxes();
}
