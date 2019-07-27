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
inline double worldRotation() { return M_PI/2; }

extern void
  addSphereTo(
    MatrixTransformPtr top_group_ptr,
    const osg::Vec3f &color,
    const osg::Matrix &transform
  );

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


#endif /* SCENEUTIL_HPP_ */
