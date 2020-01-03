#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using LineAndTransformHandle = Scene::LineAndTransformHandle;
using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
using SphereAndTransformHandle = Scene::SphereAndTransformHandle;
using std::cerr;


TransformHandle FakeScene::create(TransformHandle parent_handle)
{
  TransformHandle new_handle = {firstUnusedIndex()};
  bodies[new_handle.index].parent_index = parent_handle.index;
  return new_handle;
}


SphereAndTransformHandle
FakeScene::createSphereAndTransform(TransformHandle parent_handle)
{
  return SphereAndTransformHandle{create(parent_handle).index};
}


BoxAndTransformHandle FakeScene::createBoxAndTransform(TransformHandle parent)
{
  return BoxAndTransformHandle{create(parent).index};
}


LineAndTransformHandle FakeScene::createLineAndTransform(TransformHandle parent)
{
  return LineAndTransformHandle{create(parent).index};
}
