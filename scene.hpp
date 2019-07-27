#ifndef SCENE_HPP_
#define SCENE_HPP_

#include <cstdlib>


struct Scene {
  struct TransformHandle {
    const size_t index;
  };

  virtual TransformHandle top() = 0;
  virtual TransformHandle createSphere(TransformHandle parent) = 0;
  virtual TransformHandle createBox(TransformHandle parent) = 0;

  TransformHandle createSphere() { return createSphere(top()); }
  TransformHandle createBox() { return createBox(top()); }

  virtual void
    setScale(TransformHandle handle,float x,float y,float z) = 0;

  virtual void
    setTranslation(TransformHandle handle,float x,float y,float z) = 0;

  virtual void setColor(TransformHandle handle,float r,float g,float b) = 0;
};

#endif /* SCENE_HPP_ */
