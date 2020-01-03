#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using LineAndTransformHandle = Scene::LineAndTransformHandle;
using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
using SphereAndTransformHandle = Scene::SphereAndTransformHandle;
using GeometryAndTransformHandle = Scene::GeometryAndTransformHandle;
using std::cerr;


GeometryAndTransformHandle FakeScene::create(TransformHandle parent_handle)
{
  size_t index = firstUnusedIndex();
  GeometryAndTransformHandle new_handle = {index,index};
  bodies[index].parent_index = parent_handle.index;
  return new_handle;
}


SphereAndTransformHandle
FakeScene::createSphereAndTransform(TransformHandle parent)
{
  return SphereAndTransformHandle(create(parent));
}


BoxAndTransformHandle FakeScene::createBoxAndTransform(TransformHandle parent)
{
  return BoxAndTransformHandle{create(parent)};
}


LineAndTransformHandle FakeScene::createLineAndTransform(TransformHandle parent)
{
  return LineAndTransformHandle{create(parent)};
}
