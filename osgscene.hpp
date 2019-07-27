#ifndef OSGSCENE_HPP_
#define OSGSCENE_HPP_

#include <osg/MatrixTransform>
#include "scene.hpp"
#include "osgutil.hpp"

class OSGScene : public Scene {
  public:
    OSGScene();
    void createDefaultObjects();
    TransformHandle top() override;

    TransformHandle createSphere(TransformHandle parent) override;
    TransformHandle createBox(TransformHandle parent);
    void setScale(TransformHandle handle,float x,float y,float z) override;
    void setTranslation(TransformHandle handle,float x,float y,float z) override;
    void setColor(TransformHandle handle,float r,float g,float b) override;

    osg::Node &topNode() const
    {
      assert(top_node_ptr);
      return *top_node_ptr.get();
    }

  private:
    std::vector<osg::MatrixTransform *> transform_ptrs;
    const MatrixTransformPtr top_node_ptr;
    const TransformHandle top_handle;

    osg::MatrixTransform &transform(TransformHandle handle);
    TransformHandle makeHandle(osg::MatrixTransform &);
};

#endif /* OSGSCENE_HPP_ */
