#include "setupscene.hpp"


void setupScene(Scene &scene)
{
  auto box = scene.createBox();
  scene.setScale(box,5,.1,10);
  {
    auto point1 = scene.createSphere(box);
    scene.setScale(point1,0.1,0.1,0.1);
    scene.setColor(point1,0,0,1);
    scene.setTranslation(point1,0,1,0);
  }
}


