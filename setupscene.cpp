#include "setupscene.hpp"

#include <iostream>

using std::cerr;
using std::ostream;
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
  scene.setScale(point,0.1,0.1,0.1);
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
  scene.setScale(point, 0.1, 0.1, 0.1);
  scene.setColor(point, 0, 1, 0);
  scene.setTranslation(point,position);
  return point;
}


static void createLine(Scene &scene,Scene::Point start,Scene::Point end)
{
  LineHandle line = scene.createLine(scene.top());
  scene.setColor(line,1,0,0);
  scene.setStartPoint(line,start);
  scene.setEndPoint(line,end);
}


#if 0
static ostream &operator<<(ostream &stream,const Scene::Point &p)
{
  stream << "(" << p.x << "," << p.y << "," << p.z << ")";
  return stream;
}
#endif


void setupScene(Scene &scene)
{
  auto box = scene.createBox();
  scene.setScale(box,5,.1,10);
  scene.setTranslation(box,{0,1,0});
  Scene::Point local1 = {1,1,0};
  Scene::Point local2 = {1,1,1};
  Scene::Point local3 = {0,1,1};
  Scene::Point global1 = {1,0,0};
  Scene::Point global2 = {1,0,1};
  Scene::Point global3 = {0,0,1};
  createLocal(scene,box,local1);
  createLocal(scene,box,local2);
  createLocal(scene,box,local3);
  createGlobal(scene,global1);
  createGlobal(scene,global2);
  createGlobal(scene,global3);
  Scene::Point pred1 = scene.worldPoint(local1,box);
  Scene::Point pred2 = scene.worldPoint(local2,box);
  Scene::Point pred3 = scene.worldPoint(local3,box);
  createLine(scene,pred1,global1);
  createLine(scene,pred2,global2);
  createLine(scene,pred3,global3);
}
