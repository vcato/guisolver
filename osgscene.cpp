#include "osgscene.hpp"

#include <cassert>
#include <iostream>
#include <osg/AutoTransform>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osgManipulator/TranslateAxisDragger>
#include <osgViewer/View>
#include <osgViewer/ViewerEventHandlers>
#include "osgutil.hpp"
#include "osgpickhandler.hpp"

using std::cerr;
using std::string;
using GeodePtr = osg::ref_ptr<osg::Geode>;
using ViewPtr = osg::ref_ptr<osgViewer::View>;
using GroupPtr = osg::ref_ptr<osg::Group>;
using AutoTransformPtr = osg::ref_ptr<osg::AutoTransform>;

using TranslateAxisDraggerPtr =
  osg::ref_ptr<osgManipulator::TranslateAxisDragger>;


namespace {
struct ShapeParams {
  const string geode_name;
  ShapeParams(string geode_name_arg) : geode_name(std::move(geode_name_arg)) {}
  virtual osg::ref_ptr<osg::ShapeDrawable> createDrawable() const = 0;
};
}


struct OSGScene::Impl {
  static TransformHandle
    create(OSGScene &,TransformHandle parent,const ShapeParams &shape_params);

  static ViewPtr
    setupViewInGraphicsWindow(
      GraphicsWindowPtr graphics_window_ptr,
      ViewType view_type,
      OSGScene &
    );
};


static GraphicsWindowPtr createGraphicsWindowWithoutView()
{
  osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();

  osg::ref_ptr<osg::GraphicsContext::Traits> traits_ptr =
    new osg::GraphicsContext::Traits;

  osg::GraphicsContext::Traits &traits = *traits_ptr;
  traits.windowName = "";
  traits.windowDecoration = false;
  traits.x = 0;
  traits.y = 0;
  traits.width = 1;
  traits.height = 1;
  traits.doubleBuffer = true;
  traits.alpha = ds->getMinimumNumAlphaBits();
  traits.stencil = ds->getMinimumNumStencilBits();
  traits.sampleBuffers = ds->getMultiSamples();
  traits.samples = ds->getNumMultiSamples();
  traits.vsync = false;

  return new osgQt::GraphicsWindowQt(traits_ptr);
}


static void setupCamera(osg::Camera *camera,GraphicsWindowPtr gw)
{
  osg::Vec4 background_color(0.2, 0.2, 0.6, 1.0);
  double fovy = 30.0;
  double aspect_ratio = 1;
  double z_near = 1.0;
  double z_far = 10000.0;

  // If we don't set the graphics context, then a new one will be
  // created instead of using the one that is attached to the window.
  camera->setGraphicsContext(gw);
  camera->setClearColor(background_color);
  camera->setViewport(new osg::Viewport());

  camera->setProjectionMatrixAsPerspective(fovy,aspect_ratio,z_near,z_far);
    // Not sure why we would be setting up the camera with static parameters
    // based on the size of the window.  Is the camera set up again if
    // the window size changes, or is the width and height not really
    // necessary.
}


ViewPtr
  OSGScene::Impl::setupViewInGraphicsWindow(
    GraphicsWindowPtr graphics_window_ptr,
    ViewType view_type,
    OSGScene &scene
  )
{
  osg::ref_ptr<osgViewer::View> view(new osgViewer::View);
  setupCamera(view->getCamera(),graphics_window_ptr);
  view->addEventHandler(new osgViewer::StatsHandler);
  view->addEventHandler(new OSGPickHandler(view,&scene.selection_handler));

  osg::ref_ptr<osgGA::CameraManipulator> manipulator_ptr =
    createCameraManipulator(view_type);

  view->setCameraManipulator(manipulator_ptr);
  scene.composite_viewer.addView(view);
  return view;
}


GraphicsWindowPtr OSGScene::createGraphicsWindow(ViewType view_type)
{
  GraphicsWindowPtr window_ptr = createGraphicsWindowWithoutView();
  Impl::setupViewInGraphicsWindow(window_ptr,view_type,*this);
  {
    osgViewer::GraphicsWindow::Views views;
    window_ptr->getViews(views);
    views.front()->setSceneData(&topNode());
  }
  return window_ptr;
}


static osg::ShapeDrawable *findShapeDrawable(osg::Node *node_ptr)
{
  osg::Geode *geode_ptr = node_ptr->asGeode();

  if (geode_ptr) {
    osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);
    if (drawable_ptr) {
      osg::ShapeDrawable *shape_drawable_ptr =
        dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);
      return shape_drawable_ptr;
    }
  }

  return 0;
}


static osg::MatrixTransform &transformParentOf(osg::Node *node_ptr)
{
  assert(node_ptr);
  osg::Node *parent_ptr = node_ptr->getParent(0);
  osg::Transform *transform_ptr = parent_ptr->asTransform();
  assert(transform_ptr);
  osg::MatrixTransform *geometry_transform_ptr =
    transform_ptr->asMatrixTransform();
  assert(geometry_transform_ptr);
  return parentTransform(*geometry_transform_ptr);
}


static osg::Group &parentOf(osg::Node &t)
{
  osg::Group *group_ptr = t.getParent(0);
  assert(group_ptr);
  return *group_ptr;
}


static void removeDraggerFrom(MatrixTransformPtr transform_ptr)
{
  // * parent               * parent
  //   * group                * transform
  //     * transform   ->
  //     * dragger
  osg::Group &group = parentOf(*transform_ptr);
  osg::Group &parent = parentOf(group);
  assert(group.getNumChildren()==2);
  group.removeChild(0,2);
  parent.removeChild(&group);
  parent.addChild(transform_ptr);
}


static osg::ref_ptr<osg::Group> createGroup()
{
  return new osg::Group();
}


static void scaleAxisXY(osg::Node *x_axis_node,float scale_factor)
{
  osg::MatrixTransform *x_axis_transform = x_axis_node->asTransform()->asMatrixTransform();
  x_axis_transform->setMatrix(osg::Matrix::scale(scale_factor,scale_factor,1)*x_axis_transform->getMatrix());
}


// Dragger that maintains its size relative to the screen
static NodePtr createScreenRelativeDragger(MatrixTransformPtr transform_ptr)
{
  using TranslateAxisDragger = osgManipulator::TranslateAxisDragger;
  osg::ref_ptr<TranslateAxisDragger> my_dragger_ptr(new TranslateAxisDragger);
  TranslateAxisDraggerPtr dragger_ptr =
    new osgManipulator::TranslateAxisDragger();
  dragger_ptr->setParentDragger(my_dragger_ptr);
  dragger_ptr->setupDefaultGeometry();
  assert(dragger_ptr->getNumChildren()==3);
  float thickness = 4;
  scaleAxisXY(dragger_ptr->getChild(0),thickness);
  scaleAxisXY(dragger_ptr->getChild(1),thickness);
  scaleAxisXY(dragger_ptr->getChild(2),thickness);
  dragger_ptr->setHandleEvents(true);
  assert(isDragger(dragger_ptr));
  dragger_ptr->setParentDragger(my_dragger_ptr);
  //dragger_ptr->addTransformUpdating(transform_ptr);
  AutoTransformPtr mid_transform_ptr(new osg::AutoTransform);
  mid_transform_ptr->setAutoScaleToScreen(true);
  osg::Vec3d translation;
  osg::Quat rotation;
  osg::Vec3d scale;
  osg::Quat scale_orient;
  transform_ptr->getMatrix().decompose(translation,rotation,scale,scale_orient);
  mid_transform_ptr->setPosition(translation);
  my_dragger_ptr->addChild(mid_transform_ptr);
  mid_transform_ptr->addChild(dragger_ptr);
  my_dragger_ptr->addTransformUpdating(transform_ptr);
  dragger_ptr->setMatrix(osg::Matrix::scale(100,100,100));
  return my_dragger_ptr;
}


// Dragger that changes size based on the transform it is modifying.
static NodePtr createObjectRelativeDragger(MatrixTransformPtr transform_ptr)
{
  TranslateAxisDraggerPtr dragger_ptr =
    new osgManipulator::TranslateAxisDragger();
  dragger_ptr->setupDefaultGeometry();
  assert(dragger_ptr->getNumChildren()==3);
  float thickness = 4;
  scaleAxisXY(dragger_ptr->getChild(0),thickness);
  scaleAxisXY(dragger_ptr->getChild(1),thickness);
  scaleAxisXY(dragger_ptr->getChild(2),thickness);
  dragger_ptr->setHandleEvents(true);
  assert(isDragger(dragger_ptr));
  dragger_ptr->addTransformUpdating(transform_ptr);
  dragger_ptr->setMatrix(osg::Matrix::scale(2,2,2)*transform_ptr->getMatrix());
  return dragger_ptr;
}


static NodePtr
  createDragger(MatrixTransformPtr transform_ptr,bool screen_relative)
{
  if (screen_relative) {
    return createScreenRelativeDragger(transform_ptr);
  }
  else {
    return createObjectRelativeDragger(transform_ptr);
  }
}


static void addDraggerTo(MatrixTransformPtr transform_ptr,bool screen_relative)
{
  // * parent             * parent
  //   * transform   ->     * group
  //                          * transform
  //                          * dragger
  osg::Group &parent = parentOf(*transform_ptr);
  parent.removeChild(transform_ptr);
  GroupPtr group_ptr = createGroup();
  parent.addChild(group_ptr);
  NodePtr dragger_ptr = createDragger(transform_ptr,screen_relative);
  group_ptr->addChild(transform_ptr);
  group_ptr->addChild(dragger_ptr);
}


void
  OSGScene::SelectionHandler::nodeSelected(
    osg::Node *new_selected_node_ptr
  )
{
  if (selected_node_ptr) {
    osg::ShapeDrawable *shape_drawable_ptr =
      findShapeDrawable(selected_node_ptr);

    if (shape_drawable_ptr) {
      shape_drawable_ptr->setColor(old_color);
    }

    removeDraggerFrom(&transformParentOf(selected_node_ptr));
    selected_node_ptr = 0;
  }

  selected_node_ptr = new_selected_node_ptr;

  if (selected_node_ptr) {
    osg::ShapeDrawable *shape_drawable_ptr =
      findShapeDrawable(selected_node_ptr);

    if (shape_drawable_ptr) {
      old_color = shape_drawable_ptr->getColor();
      osg::Vec4 selection_color(1,1,0,1);
      shape_drawable_ptr->setColor(selection_color);
    }

    addDraggerTo(
      &transformParentOf(selected_node_ptr),
      use_screen_relative_dragger
    );
  }
}


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


static osg::ref_ptr<osg::ShapeDrawable> createSphereDrawable()
{
  osg::ref_ptr<osg::Sphere> sphere_ptr = new osg::Sphere;

  osg::ref_ptr<osg::TessellationHints> tesselation_hints_ptr =
    new osg::TessellationHints;
  tesselation_hints_ptr->setDetailRatio(1.0);

  osg::ref_ptr<osg::ShapeDrawable> drawable_ptr =
    new osg::ShapeDrawable(sphere_ptr,tesselation_hints_ptr);

  return drawable_ptr;
}


static osg::ref_ptr<osg::ShapeDrawable> createBoxDrawable()
{
  osg::ref_ptr<osg::Box> shape_ptr = new osg::Box;

  osg::ref_ptr<osg::TessellationHints> tesselation_hints_ptr =
    new osg::TessellationHints;
  tesselation_hints_ptr->setDetailRatio(1.0);

  osg::ref_ptr<osg::ShapeDrawable> drawable_ptr =
    new osg::ShapeDrawable(shape_ptr,tesselation_hints_ptr);

  return drawable_ptr;
}


static osg::MatrixTransform &
  addTransformTo(osg::MatrixTransform &parent)
{
  MatrixTransformPtr transform_ptr = createMatrixTransform();
  osg::MatrixTransform &matrix_transform = *transform_ptr;
  parent.addChild(transform_ptr);
  return matrix_transform;
}


namespace {
struct SphereShapeParams : ShapeParams {
  SphereShapeParams() : ShapeParams{"Sphere Geode"} {}

  osg::ref_ptr<osg::ShapeDrawable> createDrawable() const override
  {
    return createSphereDrawable();
  }
};
}


namespace {
struct BoxShapeParams : ShapeParams {
  BoxShapeParams() : ShapeParams{"Box Geode"} {}

  osg::ref_ptr<osg::ShapeDrawable> createDrawable() const override
  {
    return createBoxDrawable();
  }
};
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

  // disable the default setting of viewer.done() by pressing Escape.
  composite_viewer.setKeyEventSetsDone(0);
  composite_viewer.setThreadingModel(composite_viewer.SingleThreaded);

  // This causes paintEvent to be called regularly.
  timer.interval_in_milliseconds = 10;
  timer.callback = [this]{ composite_viewer.frame(); };
  timer.start();
}


static osg::Geode& createGeode(osg::MatrixTransform &matrix_transform)
{
  GeodePtr geode_ptr = new osg::Geode;
  osg::Geode &geode = *geode_ptr;
  osg::ref_ptr<osg::Material> material = new osg::Material;
  material->setColorMode(osg::Material::DIFFUSE);
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
  material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
  material->setShininess(osg::Material::FRONT_AND_BACK, 64.0f);

  geode.getOrCreateStateSet()->setAttributeAndModes(
    material, osg::StateAttribute::ON
  );
  matrix_transform.addChild(geode_ptr);
  return *geode_ptr;
}


auto
  OSGScene::Impl::create(
    OSGScene &scene,
    TransformHandle parent,
    const ShapeParams &shape_params
  ) -> TransformHandle
{
  osg::MatrixTransform &matrix_transform =
    addTransformTo(scene.transform(parent));
  osg::MatrixTransform &geometry_transform =
    addTransformTo(matrix_transform);
  osg::Geode &geode = createGeode(geometry_transform);
  geode.setName(shape_params.geode_name);
  geode.addDrawable(shape_params.createDrawable());
  return scene.makeHandle(geometry_transform);
}


auto OSGScene::createSphere(TransformHandle parent) -> TransformHandle
{
  return Impl::create(*this,parent,SphereShapeParams());
}


auto OSGScene::createBox(TransformHandle parent) -> TransformHandle
{
  return Impl::create(*this,parent,BoxShapeParams());
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
  ::setTranslation(parentTransform(transform(handle)),x,y,z);
}


void OSGScene::setColor(TransformHandle handle,float r,float g,float b)
{
  ::setColor(transform(handle),osg::Vec3f(r,g,b));
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
