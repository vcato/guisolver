#include "setupscene.hpp"

#include <iostream>

using std::cerr;
using std::ostream;
using std::vector;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;


static TransformHandle
  createLocal(
    Scene &scene,
    TransformHandle &parent,
    Scene::Point position
  )
{
  auto point = scene.createSphere(parent);
  scene.setGeometryScale(point,0.1,0.1,0.1);
  scene.setColor(point,0,0,1);
  scene.setTranslation(point,position);
  return point;
}


static TransformHandle
  createGlobal(
    Scene &scene,
    Scene::Point position
  )
{
  auto point = scene.createSphere();
  scene.setGeometryScale(point, 0.1, 0.1, 0.1);
  scene.setColor(point, 0, 1, 0);
  scene.setTranslation(point,position);
  return point;
}


static void
  createLine(
    vector<SceneSetup::Line> &lines,
    Scene &scene,
    Scene::TransformHandle local_handle,
    Scene::TransformHandle global_handle
  )
{
  Scene::Point start = scene.worldPoint({0,0,0},local_handle);
  Scene::Point end = scene.worldPoint({0,0,0},global_handle);
  LineHandle line = scene.createLine(scene.top());
  scene.setColor(line,1,0,0);
  scene.setStartPoint(line,start);
  scene.setEndPoint(line,end);

  lines.push_back({
    line,
    local_handle,
    global_handle
  });
}


#if 0
static ostream &operator<<(ostream &stream,const Scene::Point &p)
{
  stream << "(" << p.x << "," << p.y << "," << p.z << ")";
  return stream;
}
#endif


SceneSetup setupScene(Scene &scene)
{
  auto box = scene.createBox();
  scene.setGeometryScale(box,5,.1,10);
  scene.setTranslation(box,{0,1,0});

  scene.setCoordinateAxes(
    box,Scene::Vector(0,0,-1),Scene::Vector(0,1,0),Scene::Vector(1,0,0)
  );

  Scene::Point local1 = {1,1,0};
  Scene::Point local2 = {1,1,1};
  Scene::Point local3 = {0,1,1};
  Scene::Point global1 = {1,0,0};
  Scene::Point global2 = {1,0,1};
  Scene::Point global3 = {0,0,1};
  TransformHandle local1_handle = createLocal(scene,box,local1);
  TransformHandle local2_handle = createLocal(scene,box,local2);
  TransformHandle local3_handle = createLocal(scene,box,local3);
  TransformHandle global1_handle = createGlobal(scene,global1);
  TransformHandle global2_handle = createGlobal(scene,global2);
  TransformHandle global3_handle = createGlobal(scene,global3);

  vector<SceneSetup::Line> lines;

  createLine(lines,scene,local1_handle,global1_handle);
  createLine(lines,scene,local2_handle,global2_handle);
  createLine(lines,scene,local3_handle,global3_handle);

  return { box, lines };
}
