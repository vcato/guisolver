#include "osgscene.hpp"

#include <iostream>
#include "osgutil.hpp"

using std::cerr;

void OSGScene::create()
{
  top_node_ptr = createMatrixTransform();
  top_node_ptr->addChild(createFloor());
  top_node_ptr->setMatrix(
    osg::Matrix::rotate(worldRotation(),osg::Vec3d(1,0,0))
  );
  {
    osg::Vec3f color(1,0,0);
    osg::Matrix transform = osg::Matrix::translate(1,1,0);
    addSphereTo(top_node_ptr,color,transform);
  }
  {
    osg::Vec3f color(0,1,0);
    osg::Matrix transform = osg::Matrix::translate(-1,1,0);
    addSphereTo(top_node_ptr,color,transform);
  }
}


void OSGScene::createSphere()
{
  osg::Vec3f color(0,0,1);
  osg::Matrix transform = osg::Matrix::translate(3,1,0);
  addSphereTo(top_node_ptr,color,transform);
}
