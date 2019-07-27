#ifndef SCENEUTIL_HPP_
#define SCENEUTIL_HPP_

#include <cassert>
#include <osg/Node>
#include <osgViewer/View>
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


struct OSGSelectionHandler {
  OSGSelectionHandler()
  : selected_node_ptr(0)
  {
  }

  void nodeSelected(osg::Node *new_selected_node_ptr);

  osg::Node *selected_node_ptr;
  osg::Vec4 old_color;
};


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
