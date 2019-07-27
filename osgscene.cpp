#include "osgscene.hpp"

#include <iostream>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include "osgutil.hpp"

using std::cerr;
typedef osg::ref_ptr<osg::Geode> GeodePtr;


static void setColor(osg::Geode &geode,const osg::Vec3f &vec)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);
  assert(shape_drawable_ptr);
  shape_drawable_ptr->setColor(osg::Vec4f(vec.x(),vec.y(),vec.z(),1));
}


static void setColor(osg::MatrixTransform &node,const osg::Vec3f &vec)
{
  assert(node.getNumChildren() == 1);
  osg::Node *child_ptr = node.getChild(0);
  assert(child_ptr);
  osg::Geode *geode_ptr = child_ptr->asGeode();

  if (geode_ptr) {
    setColor(*geode_ptr,vec);
  }
}


static void addSphereTo(osg::Geode &geode)
{
  osg::ref_ptr<osg::Sphere> sphere_ptr = new osg::Sphere;

  osg::ref_ptr<osg::TessellationHints> sphere_tesselation_hints_ptr =
    new osg::TessellationHints;
  sphere_tesselation_hints_ptr->setDetailRatio(1.0);

  osg::ref_ptr<osg::ShapeDrawable> sphere_drawable_ptr =
    new osg::ShapeDrawable(sphere_ptr,sphere_tesselation_hints_ptr);

  geode.addDrawable(sphere_drawable_ptr);
}


static void addBoxTo(osg::Geode &geode)
{
  osg::ref_ptr<osg::Box> sphere_ptr = new osg::Box;

  osg::ref_ptr<osg::TessellationHints> sphere_tesselation_hints_ptr =
    new osg::TessellationHints;
  sphere_tesselation_hints_ptr->setDetailRatio(1.0);

  osg::ref_ptr<osg::ShapeDrawable> sphere_drawable_ptr =
    new osg::ShapeDrawable(sphere_ptr,sphere_tesselation_hints_ptr);

  geode.addDrawable(sphere_drawable_ptr);
}


static GeodePtr createGeode()
{
  GeodePtr sphere_geode_ptr = new osg::Geode;
  osg::Geode &geode = *sphere_geode_ptr;
  osg::ref_ptr<osg::Material> material = new osg::Material;
  material->setColorMode(osg::Material::DIFFUSE);
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
  material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
  material->setShininess(osg::Material::FRONT_AND_BACK, 64.0f);

  geode.getOrCreateStateSet()->setAttributeAndModes(
    material, osg::StateAttribute::ON
  );

  return sphere_geode_ptr;
}


static osg::MatrixTransform &
  addTransformTo(osg::MatrixTransform &parent)
{
  MatrixTransformPtr transform_ptr = createMatrixTransform();
  osg::MatrixTransform &matrix_transform = *transform_ptr;
  parent.addChild(transform_ptr);
  return matrix_transform;
}


static osg::Geode &addGeodeTo(osg::MatrixTransform &matrix_transform)
{
  GeodePtr geode_ptr = createGeode();
  matrix_transform.addChild(geode_ptr);
  return *geode_ptr;
}


static osg::Geode &addSphereTo(osg::MatrixTransform & matrix_transform)
{
  osg::Geode &geode = addGeodeTo(matrix_transform);
  geode.setName("Sphere Geode");
  addSphereTo(geode);
  return geode;
}


static osg::Geode &addBoxTo(osg::MatrixTransform & matrix_transform)
{
  osg::Geode &geode = addGeodeTo(matrix_transform);
  geode.setName("Box Geode");
  addBoxTo(geode);
  return geode;
}


static void
  setTranslation(osg::MatrixTransform &transform,float x,float y,float z)
{
  auto m = transform.getMatrix();
  m.setTrans(x,y,z);
  transform.setMatrix(m);
}


static void
  setRotatation(osg::MatrixTransform &transform,const osg::Quat &rot)
{
  auto m = transform.getMatrix();
  m.setRotate(rot);
  transform.setMatrix(m);
}


static void
  setScale(osg::MatrixTransform &transform,float x,float y,float z)
{
  auto m = transform.getMatrix();
  setScale(m,osg::Vec3f(x,y,z));
  transform.setMatrix(m);
}


static void addFloorTo(osg::MatrixTransform &matrix_transform)
{
  matrix_transform.addChild(createFloor());
}


OSGScene::OSGScene()
: top_node_ptr(createMatrixTransform()),
  top_handle(makeHandle(*top_node_ptr))
{
  osg::MatrixTransform &node = *top_node_ptr;
  addFloorTo(node);
  setRotatation(node,worldRotation());
  node.getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
}


void OSGScene::createDefaultObjects()
{
  {
    osg::MatrixTransform &transform_node = addTransformTo(*top_node_ptr);
    ::setTranslation(transform_node,1,1,0);
    osg::Geode &sphere = addSphereTo(transform_node);
    ::setColor(sphere,osg::Vec3f(1,0,0));
  }
  {
    osg::MatrixTransform &transform_node = addTransformTo(*top_node_ptr);
    ::setTranslation(transform_node,-1,1,0);
    osg::Geode &sphere = addSphereTo(transform_node);
    ::setColor(sphere,osg::Vec3f(0,1,0));
  }
}


auto OSGScene::createSphere(TransformHandle parent) -> TransformHandle
{
  osg::MatrixTransform &matrix_transform = addTransformTo(transform(parent));
  addSphereTo(matrix_transform);
  return makeHandle(matrix_transform);
}


osg::MatrixTransform& OSGScene::transform(TransformHandle handle)
{
  assert(transform_ptrs[handle.index]);
  return *transform_ptrs[handle.index];
}


void OSGScene::setScale(TransformHandle handle,float x,float y,float z)
{
  ::setScale(transform(handle),x,y,z);
}


void OSGScene::setTranslation(TransformHandle handle,float x,float y,float z)
{
  ::setTranslation(transform(handle),x,y,z);
}


void OSGScene::setColor(TransformHandle handle,float r,float g,float b)
{
  ::setColor(transform(handle),osg::Vec3f(r,g,b));
}


auto OSGScene::createBox(TransformHandle parent) -> TransformHandle
{
  osg::MatrixTransform &matrix_transform = addTransformTo(transform(parent));
  addBoxTo(matrix_transform);
  return makeHandle(matrix_transform);
}


template <typename A,typename B>
static size_t findIndex(const std::vector<A> &v,B p)
{
  return std::find(v.begin(),v.end(),p) - v.begin();
}


template <typename T>
static bool contains(const std::vector<T*> &v,T* p)
{
  return findIndex(v,p) != v.size();
}


template <typename T>
static size_t findNull(const std::vector<T*> &v)
{
  return findIndex(v,nullptr);
}


auto OSGScene::makeHandle(osg::MatrixTransform &transform) -> TransformHandle
{
  assert(!contains(transform_ptrs,&transform));
  size_t index = findNull(transform_ptrs);

  if (index == transform_ptrs.size()) {
    transform_ptrs.push_back(&transform);
  }
  else {
    transform_ptrs[index] = &transform;
  }

  return TransformHandle{index};
}


auto OSGScene::top() -> TransformHandle
{
  return top_handle;
}
