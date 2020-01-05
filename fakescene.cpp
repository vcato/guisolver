#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using BoxAndTransformHandle = Scene::BoxAndTransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using LineHandle = Scene::LineHandle;
using SphereAndTransformHandle = Scene::SphereAndTransformHandle;
using GeometryAndTransformHandle = Scene::GeometryAndTransformHandle;
using std::cerr;


TransformHandle
FakeScene::createTransform(TransformHandle parent_handle)
{
  size_t transform_index = firstUnusedIndex();
  objects[transform_index].parent_index = parent_handle.index;
  TransformHandle transform_handle{transform_index};
  return transform_handle;
}


GeometryHandle
FakeScene::createGeometry(TransformHandle transform_handle)
{
  size_t geometry_index = firstUnusedIndex();
  objects[geometry_index].parent_index = transform_handle.index;
  objects[geometry_index].maybe_geometry_center = Point{0,0,0};
  GeometryHandle geometry_handle{geometry_index};
  return geometry_handle;
}


GeometryAndTransformHandle
FakeScene::createGeometryAndTransform(TransformHandle parent_handle)
{
  TransformHandle transform_handle = createTransform(parent_handle);
  GeometryHandle geometry_handle = createGeometry(transform_handle);
  GeometryAndTransformHandle new_handle = {transform_handle, geometry_handle};
  return new_handle;
}


TransformHandle FakeScene::parentTransform(GeometryHandle) const
{
  assert(false); // not needed
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
FakeScene::createSphereAndTransform(TransformHandle parent_handle)
{
  TransformHandle transform_handle = createTransform(parent_handle);
  GeometryHandle geometry_handle = createGeometry(transform_handle);
  GeometryAndTransformHandle new_handle = {transform_handle, geometry_handle};
  return SphereAndTransformHandle{new_handle};
}


GeometryHandle FakeScene::createBox(TransformHandle parent)
{
  return createGeometry(parent);
}


LineHandle FakeScene::createLine(TransformHandle parent)
{
  return LineHandle(createGeometry(parent));
}
