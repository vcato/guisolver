#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using LineAndTransformHandle = Scene::LineAndTransformHandle;
using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
using SphereAndTransformHandle = Scene::SphereAndTransformHandle;
using GeometryAndTransformHandle = Scene::GeometryAndTransformHandle;
using std::cerr;


GeometryAndTransformHandle
FakeScene::createGeometryAndTransform(TransformHandle parent_handle)
{
  size_t transform_index = firstUnusedIndex();
  objects[transform_index].parent_index = parent_handle.index;
  size_t geometry_index = firstUnusedIndex();
  objects[geometry_index].parent_index = transform_index;
  GeometryAndTransformHandle new_handle = {TransformHandle{transform_index},GeometryHandle{geometry_index}};
  return new_handle;
}


void FakeScene::destroyGeometry(GeometryHandle handle)
{
  assert(nChildren(handle.index) == 0);
  objects.erase(handle.index);
}


void FakeScene::destroyTransform(TransformHandle handle)
{
  assert(nChildren(handle.index) == 0);
  objects.erase(handle.index);
}


int FakeScene::nChildren(size_t handle_index) const
{
  int count = 0;

  for (auto item : objects) {
    TransformIndex next_parent_index = item.second.parent_index;

    if (next_parent_index == handle_index) {
      ++count;
    }
  }

  return count;
}


SphereAndTransformHandle
FakeScene::createSphereAndTransform(TransformHandle parent)
{
  return SphereAndTransformHandle(createGeometryAndTransform(parent));
}


BoxAndTransformHandle FakeScene::createBoxAndTransform(TransformHandle parent)
{
  return BoxAndTransformHandle{createGeometryAndTransform(parent)};
}


LineAndTransformHandle FakeScene::createLineAndTransform(TransformHandle parent)
{
  return LineAndTransformHandle{createGeometryAndTransform(parent)};
}
