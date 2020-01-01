#include "fakescene.hpp"


using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;
using std::cerr;


TransformHandle FakeScene::create(TransformHandle parent_handle)
{
  TransformHandle new_handle = {firstUnusedIndex()};
  bodies[new_handle.index].parent_index = parent_handle.index;
  return new_handle;
}


TransformHandle FakeScene::createSphere(TransformHandle parent_handle)
{
  return create(parent_handle);
}


TransformHandle FakeScene::createBox(TransformHandle parent)
{
  return create(parent);
}


LineHandle FakeScene::createLine(TransformHandle parent)
{
  return LineHandle{create(parent).index};
}
