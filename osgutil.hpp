#ifndef SCENEUTIL_HPP_
#define SCENEUTIL_HPP_

#include <osg/Node>
#include <osgGA/CameraManipulator>
#include "viewtype.hpp"

typedef osg::ref_ptr<osg::Node> NodePtr;
typedef osg::ref_ptr<osgGA::CameraManipulator> CameraManipulatorPtr;
typedef osg::ref_ptr<osg::MatrixTransform> MatrixTransformPtr;

extern NodePtr createFloor();
inline double worldRotationAngle() { return M_PI/2; }
inline osg::Vec3d worldRotationAxis() { return osg::Vec3d(1,0,0); }


inline osg::Quat worldRotation()
{
  return osg::Quat(worldRotationAngle(),worldRotationAxis());
}

extern bool isDragger(osg::Node *node_ptr);


extern osg::Matrix
  compose(
    osg::Vec3f translation,
    osg::Quat rotation,
    osg::Vec3f scale,
    osg::Quat so
  );

extern void setScale(osg::Matrix &m,const osg::Vec3f &new_scale);
extern void setNoRotScale(osg::Matrix &m, const osg::Vec3f &new_scale);

extern std::ostream& operator<<(std::ostream &stream, const osg::Vec3f &);

extern void
  setCoordinateAxesOf(
    osg::Matrix &m,
    const osg::Vec3f &x,
    const osg::Vec3f &y,
    const osg::Vec3f &z
  );

struct OSGCoordinateAxes {
  osg::Vec3f x,y,z;
};

extern OSGCoordinateAxes coordinateAxesOf(const osg::Matrix &m);

#endif /* SCENEUTIL_HPP_ */
