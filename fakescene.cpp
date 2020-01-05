#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using LineHandle = Scene::LineHandle;
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


TransformHandle FakeScene::parentTransform(GeometryHandle handle) const
{
  return TransformHandle{elementOf(objects, handle.index).parent_index};
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


GeometryHandle FakeScene::createBox(TransformHandle parent)
{
  return createGeometry(parent);
}


LineHandle FakeScene::createLine(TransformHandle parent)
{
  GeometryHandle geometry = createGeometry(parent);
  objects[geometry.index].is_line = true;
  return LineHandle(geometry);
}


GeometryHandle FakeScene::createSphere(TransformHandle parent)
{
  return createGeometry(parent);
}


Optional<LineHandle> FakeScene::maybeLine(GeometryHandle handle) const
{
  if (elementOf(objects, handle.index).is_line) {
    return LineHandle{handle.index};
  }
  else {
    return {};
  }
}
