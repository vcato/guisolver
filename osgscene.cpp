#include "osgscene.hpp"

#include <iostream>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include "osgutil.hpp"

using std::cerr;
typedef osg::ref_ptr<osg::Geode> GeodePtr;


static void setColor(GeodePtr geode_ptr,const osg::Vec3f &vec)
{
  osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);
  assert(drawable_ptr);
  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);
  assert(shape_drawable_ptr);
  shape_drawable_ptr->setColor(osg::Vec4f(vec.x(),vec.y(),vec.z(),1));
}


static GeodePtr createSphere()
{
  osg::ref_ptr<osg::Sphere> sphere_ptr = new osg::Sphere;

  osg::ref_ptr<osg::TessellationHints> sphere_tesselation_hints_ptr =
    new osg::TessellationHints;
  sphere_tesselation_hints_ptr->setDetailRatio(1.0);
  osg::ref_ptr<osg::ShapeDrawable> sphere_drawable_ptr =
    new osg::ShapeDrawable(sphere_ptr,sphere_tesselation_hints_ptr);

  GeodePtr sphere_geode_ptr = new osg::Geode;
  sphere_geode_ptr->addDrawable(sphere_drawable_ptr);

  osg::ref_ptr<osg::Material> material = new osg::Material;
  material->setColorMode(osg::Material::DIFFUSE);
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
  material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
  material->setShininess(osg::Material::FRONT_AND_BACK, 64.0f);
  sphere_geode_ptr->getOrCreateStateSet()->setAttributeAndModes(
    material, osg::StateAttribute::ON
  );
  sphere_geode_ptr->setName("Sphere Geode");
  assert(!sphere_geode_ptr->asGroup());

  return sphere_geode_ptr;
}


static void
  addSphereTo(
    MatrixTransformPtr top_group_ptr,
    const osg::Vec3f &color,
    const osg::Matrix &transform
  )
{
  MatrixTransformPtr transform_ptr = createMatrixTransform();
  GeodePtr sphere_ptr = createSphere();
  setColor(sphere_ptr,color);
  transform_ptr->addChild(sphere_ptr);
  transform_ptr->setMatrix(transform);
  top_group_ptr->addChild(transform_ptr);
}


void OSGScene::create()
{
  top_node_ptr = createMatrixTransform();
  top_node_ptr->addChild(createFloor());
  top_node_ptr->setMatrix(
    osg::Matrix::rotate(worldRotation(),osg::Vec3d(1,0,0))
  );
  {
    osg::Vec3f color(1,0,0);
    osg::Matrix transform = osg::Matrix::translate(1,1,0);
    addSphereTo(top_node_ptr,color,transform);
  }
  {
    osg::Vec3f color(0,1,0);
    osg::Matrix transform = osg::Matrix::translate(-1,1,0);
    addSphereTo(top_node_ptr,color,transform);
  }
}


void OSGScene::createSphere()
{
  osg::Vec3f color(0,0,1);
  osg::Matrix transform = osg::Matrix::translate(3,1,0);
  addSphereTo(top_node_ptr,color,transform);
}
