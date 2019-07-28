#ifndef SCENEUTIL_HPP_
#define SCENEUTIL_HPP_

#include <osg/Node>
#include <osgGA/CameraManipulator>
#include "viewtype.hpp"

typedef osg::ref_ptr<osg::Node> NodePtr;
typedef osg::ref_ptr<osgGA::CameraManipulator> CameraManipulatorPtr;
typedef osg::ref_ptr<osg::MatrixTransform> MatrixTransformPtr;

extern MatrixTransformPtr createMatrixTransform();
extern NodePtr createFloor();
inline double worldRotationAngle() { return M_PI/2; }
inline osg::Vec3d worldRotationAxis() { return osg::Vec3d(1,0,0); }


inline osg::Quat worldRotation()
{
  return osg::Quat(worldRotationAngle(),worldRotationAxis());
}

extern CameraManipulatorPtr createCameraManipulator(ViewType view_type);
extern osg::MatrixTransform &parentTransform(osg::MatrixTransform &t);

extern const
  osg::MatrixTransform &parentTransform(const osg::MatrixTransform &t);


extern bool isDragger(osg::Node *node_ptr);


extern osg::Matrix
  compose(
    osg::Vec3f translation,
    osg::Quat rotation,
    osg::Vec3f scale,
    osg::Quat so
  );

extern void setScale(osg::Matrix &m,const osg::Vec3f &new_scale);

#endif /* SCENEUTIL_HPP_ */
