#include "setupscene.hpp"


static void
  createLocal(
    Scene &scene,
    Scene::TransformHandle &parent,
    float x,float y,float z
  )
{
  auto point1 = scene.createSphere(parent);
  scene.setScale(point1,0.1,0.1,0.1);
  scene.setColor(point1,0,0,1);
  scene.setTranslation(point1,x,y,z);
}


static void
  createGlobal(
    Scene &scene,
    float x,float y,float z
  )
{
  auto point1 = scene.createSphere();
  scene.setScale(point1,0.1,0.1,0.1);
  scene.setColor(point1,0,1,0);
  scene.setTranslation(point1,x,y,z);
}


void setupScene(Scene &scene)
{
  auto box = scene.createBox();
  scene.setScale(box,5,.1,10);
  scene.setTranslation(box,0,1,0);
  createLocal(scene,box,1,1,0);
  createLocal(scene,box,1,1,1);
  createLocal(scene,box,0,1,1);
  createGlobal(scene,1,0,0);
  createGlobal(scene,1,0,1);
  createGlobal(scene,0,0,1);
}
