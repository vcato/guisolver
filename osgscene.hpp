#ifndef OSGSCENE_HPP_
#define OSGSCENE_HPP_

#include <osg/MatrixTransform>
#include "scene.hpp"
#include "osgutil.hpp"

struct OSGScene : Scene {
  OSGScene();
  void createDefaultObjects();
  virtual void createSphere();
  void createBox();

  MatrixTransformPtr top_node_ptr;
};

#endif /* OSGSCENE_HPP_ */
