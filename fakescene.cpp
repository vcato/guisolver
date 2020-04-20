#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using LineHandle = Scene::LineHandle;
using MeshHandle = Scene::MeshHandle;
using std::cerr;


TransformHandle
FakeScene::createTransform(TransformHandle parent_handle)
{
  size_t transform_index = firstUnusedIndex();
  objects[transform_index].parent_index = parent_handle.index;
  TransformHandle transform_handle{transform_index};
  assert(parentTransform(transform_handle) == parent_handle);
  return transform_handle;
}


TransformHandle FakeScene::parentTransform(TransformHandle parent_handle) const
{
  size_t transform_index = parent_handle.index;
  auto iter = objects.find(transform_index);
  assert(iter != objects.end());
  return TransformHandle{iter->second.parent_index};
}


GeometryHandle
FakeScene::createGeometry(TransformHandle transform_handle)
{
  size_t geometry_index = firstUnusedIndex();
  objects[geometry_index].parent_index = transform_handle.index;
  objects[geometry_index].maybe_geometry_center = Point{0,0,0};
  objects[geometry_index].maybe_geometry_scale = Point{1,1,1};
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
  if (maybe_dragger_index == handle.index) {
    maybe_dragger_index.reset();
  }

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


MeshHandle FakeScene::createMesh(TransformHandle parent, const Mesh &)
{
  return MeshHandle{createGeometry(parent)};
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


void FakeScene::setTranslation(TransformHandle handle, Point translation)
{
  objects[handle.index].maybe_transform_translation = translation;
}


auto FakeScene::translation(TransformHandle handle) const -> Point
{
  return *elementOf(objects, handle.index).maybe_transform_translation;
}


CoordinateAxes FakeScene::coordinateAxes(TransformHandle) const
{
  return CoordinateAxes {
    {1,0,0},
    {0,1,0},
    {0,0,1}
  };
}


Vec3 FakeScene::geometryScale(GeometryHandle handle) const
{
  return *elementOf(objects, handle.index).maybe_geometry_scale;
}


void FakeScene::setGeometryScale(GeometryHandle handle,const Vec3 &arg)
{
  elementOf(objects, handle.index).maybe_geometry_scale = arg;
}


Vec3 FakeScene::geometryCenter(GeometryHandle handle) const
{
  return *elementOf(objects, handle.index).maybe_geometry_center;
}


void
FakeScene::setGeometryCenter(
  GeometryHandle handle, const Point &center
)
{
  elementOf(objects, handle.index).maybe_geometry_center = center;
}


#if !CHANGE_MANIPULATORS
void FakeScene::attachManipulatorToSelectedNode(ManipulatorType dragger_type)
{
  maybe_dragger_index = maybe_selected_object_index;
  maybe_dragger_type = dragger_type;
}
#endif


#if CHANGE_MANIPULATORS
TransformHandle
FakeScene::createTranslateManipulator(TransformHandle parent)
{
  TransformHandle result = createTransform(parent);
  assert(!maybe_dragger_index);
  maybe_dragger_index = result.index;
  return result;
}
#endif


#if CHANGE_MANIPULATORS
GeometryHandle
FakeScene::createScaleManipulator(TransformHandle /*parent*/)
{
  assert(false); // not implemented
}
#endif
