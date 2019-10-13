#include "osgscene.hpp"

#include <cassert>
#include <iostream>
#include <osg/AutoTransform>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osgManipulator/TranslateAxisDragger>
#include <osgViewer/View>
#include <osgViewer/ViewerEventHandlers>
#include <osg/io_utils>
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
  virtual osg::ref_ptr<osg::Drawable> createDrawable() const = 0;

  virtual osg::Material::ColorMode colorMode() const
  {
    return osg::Material::DIFFUSE;
  }
};
}


static void removeAllPrimativeSets(osg::Geometry &geometry)
{
  unsigned int n = geometry.getNumPrimitiveSets();

  for (unsigned int i = n; i!=0;) {
    --i;
    geometry.removePrimitiveSet(i);
  }
}


namespace {
struct LineDrawable : osg::Geometry {
  osg::Vec3f color = osg::Vec3(1,1,1);
  osg::Vec3 start_point = osg::Vec3f(0,0,0);
  osg::Vec3 end_point = osg::Vec3(1,1,1);

  void setup()
  {
    LineDrawable &line_geometry = *this;
    osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array;
    points->push_back(start_point);
    points->push_back(end_point);
    osg::ref_ptr<osg::Vec4Array> color_array = new osg::Vec4Array;
    color_array->push_back(osg::Vec4(color, 1.0));

    removeAllPrimativeSets(line_geometry);
    line_geometry.setVertexArray(points.get());
    line_geometry.setColorArray(color_array.get());
    line_geometry.setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    line_geometry.addPrimitiveSet(new osg::DrawArrays(GL_LINES,0,2));
  }
};
}


struct OSGScene::Impl {
  struct DraggerCallback;

  static TransformHandle
    create(OSGScene &,TransformHandle parent,const ShapeParams &shape_params);

  static osg::MatrixTransform &
    createGeometryTransform(
      OSGScene &,
      TransformHandle parent,
      const ShapeParams &shape_params
    );

  static ViewPtr
    setupViewInGraphicsWindow(
      GraphicsWindowPtr graphics_window_ptr,
      ViewType view_type,
      OSGScene &
    );

  static size_t getHandleIndex(OSGScene &scene,osg::MatrixTransform &);

  static osg::MatrixTransform &
    geometryTransform(OSGScene &,TransformHandle);

  static const osg::MatrixTransform &
    geometryTransform(const OSGScene &,TransformHandle);

  static TransformHandle makeHandle(OSGScene &,osg::MatrixTransform &);
  static LineDrawable& lineDrawable(OSGScene &,LineHandle);
  static LineHandle makeLineHandle(OSGScene &,osg::MatrixTransform &);

  static void handleDragFinish(OSGScene &scene)
  {
    if (scene.changed_callback) {
      scene.changed_callback();
    }
  }

  static void handleDragging(OSGScene &scene)
  {
    if (scene.changing_callback) {
      scene.changing_callback();
    }
  }

  static const osg::MatrixTransform &
    transform(const OSGScene &scene,TransformHandle t)
  {
    return parentTransform(geometryTransform(scene,t));
  }

  static osg::MatrixTransform &
    transform(OSGScene &scene,TransformHandle t)
  {
    return parentTransform(geometryTransform(scene,t));
  }
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
  osgViewer::GraphicsWindow::Views views;
  window_ptr->getViews(views);
  assert(top_node_ptr);
  views.front()->setSceneData(&*top_node_ptr);
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


#if 0
static string stageName(osgManipulator::MotionCommand::Stage stage)
{
  switch (stage) {
    case osgManipulator::MotionCommand::NONE:
      return "NONE";
    case osgManipulator::MotionCommand::START:
      return "START";
    case osgManipulator::MotionCommand::MOVE:
      return "MOVE";
    case osgManipulator::MotionCommand::FINISH:
      return "FINISH";
  }

  return "";
}
#endif


struct OSGScene::Impl::DraggerCallback : osgManipulator::DraggerCallback {
  OSGScene &scene;

  DraggerCallback(OSGScene &scene_arg)
  : scene(scene_arg)
  {
  }

  bool receive(const osgManipulator::MotionCommand& command) override
  {
    osgManipulator::MotionCommand::Stage stage = command.getStage();

    if (stage == osgManipulator::MotionCommand::FINISH) {
      Impl::handleDragFinish(scene);
    }

    if (stage == osgManipulator::MotionCommand::MOVE) {
      Impl::handleDragging(scene);
    }

    return false;
  }
};


// Dragger that maintains its size relative to the screen
static NodePtr
  createScreenRelativeDragger(
    MatrixTransformPtr transform_ptr,
    osgManipulator::DraggerCallback &
  )
{
  using TranslateAxisDragger = osgManipulator::TranslateAxisDragger;
  TranslateAxisDraggerPtr parent_dragger_ptr(new TranslateAxisDragger);
  TranslateAxisDraggerPtr dragger_ptr =
    new osgManipulator::TranslateAxisDragger();
  dragger_ptr->setParentDragger(parent_dragger_ptr);
  dragger_ptr->setupDefaultGeometry();
  assert(dragger_ptr->getNumChildren()==3);
  float thickness = 4;
  scaleAxisXY(dragger_ptr->getChild(0),thickness);
  scaleAxisXY(dragger_ptr->getChild(1),thickness);
  scaleAxisXY(dragger_ptr->getChild(2),thickness);
  dragger_ptr->setHandleEvents(true);
  assert(isDragger(dragger_ptr));
  dragger_ptr->setParentDragger(parent_dragger_ptr);
  AutoTransformPtr mid_transform_ptr(new osg::AutoTransform);
  mid_transform_ptr->setAutoScaleToScreen(true);
  osg::Vec3d translation;
  osg::Quat rotation;
  osg::Vec3d scale;
  osg::Quat scale_orient;
  transform_ptr->getMatrix().decompose(translation,rotation,scale,scale_orient);
  mid_transform_ptr->setPosition(translation);
  parent_dragger_ptr->addChild(mid_transform_ptr);
  mid_transform_ptr->addChild(dragger_ptr);
  parent_dragger_ptr->addTransformUpdating(transform_ptr);
  dragger_ptr->setMatrix(osg::Matrix::scale(100,100,100));
  return parent_dragger_ptr;
}


// Dragger that changes size based on the transform it is modifying.
static NodePtr
  createObjectRelativeDragger(
    MatrixTransformPtr transform_ptr,
    osgManipulator::DraggerCallback &dragger_callback
  )
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
  dragger_ptr->addDraggerCallback(&dragger_callback);
  return dragger_ptr;
}


static NodePtr
  createDragger(
    MatrixTransformPtr transform_ptr,
    bool screen_relative,
    osgManipulator::DraggerCallback &dragger_callback
  )
{
  if (screen_relative) {
    return createScreenRelativeDragger(transform_ptr,dragger_callback);
  }
  else {
    return createObjectRelativeDragger(transform_ptr,dragger_callback);
  }
}


static void
  addDraggerTo(
    MatrixTransformPtr transform_ptr,
    bool screen_relative,
    osgManipulator::DraggerCallback &dragger_callback
  )
{
  // * parent             * parent
  //   * transform   ->     * group
  //                          * transform
  //                          * dragger
  osg::Group &parent = parentOf(*transform_ptr);
  parent.removeChild(transform_ptr);
  GroupPtr group_ptr = createGroup();
  parent.addChild(group_ptr);

  NodePtr dragger_ptr =
    createDragger(transform_ptr,screen_relative,dragger_callback);

  group_ptr->addChild(transform_ptr);
  group_ptr->addChild(dragger_ptr);
}


OSGScene::SelectionHandler::SelectionHandler(OSGScene &scene_arg)
: scene(scene_arg)
{
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

    osg::ref_ptr<osgManipulator::DraggerCallback> dc =
      new Impl::DraggerCallback(scene);

    addDraggerTo(
      &transformParentOf(selected_node_ptr),
      use_screen_relative_dragger,
      *dc
    );
  }
}


static void setColor(osg::Geode &geode,const osg::Vec3f &vec)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);

  if (shape_drawable_ptr) {
    shape_drawable_ptr->setColor(osg::Vec4f(vec.x(),vec.y(),vec.z(),1));
    return;
  }

  LineDrawable *geometry_ptr =
    dynamic_cast<LineDrawable*>(drawable_ptr);

  if (geometry_ptr) {
    geometry_ptr->color = vec;
    geometry_ptr->setup();
  }
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


static osg::ref_ptr<LineDrawable> createLineDrawable()
{
  osg::ref_ptr<LineDrawable> beam_ptr(new LineDrawable);
  beam_ptr->setup();
  return beam_ptr;
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
  addTransformTo(osg::Group &parent)
{
  MatrixTransformPtr transform_ptr = createMatrixTransform();
  osg::MatrixTransform &matrix_transform = *transform_ptr;
  parent.addChild(transform_ptr);
  return matrix_transform;
}


namespace {
struct SphereShapeParams : ShapeParams {
  SphereShapeParams() : ShapeParams{"Sphere Geode"} {}

  osg::ref_ptr<osg::Drawable> createDrawable() const override
  {
    return createSphereDrawable();
  }
};
}


namespace {
struct BoxShapeParams : ShapeParams {
  BoxShapeParams() : ShapeParams{"Box Geode"} {}

  osg::ref_ptr<osg::Drawable> createDrawable() const override
  {
    return createBoxDrawable();
  }
};
}


namespace {
struct LineShapeParams : ShapeParams {
  LineShapeParams() : ShapeParams{"Line Geode"} {}

  osg::ref_ptr<osg::Drawable> createDrawable() const override
  {
    return createLineDrawable();
  }

  osg::Material::ColorMode colorMode() const override
  {
    return osg::Material::EMISSION;
  }
};
}


static void
  setTranslation(osg::MatrixTransform &transform,float x,float y,float z)
{
  osg::Matrix m = transform.getMatrix();
  m.setTrans(x,y,z);
  transform.setMatrix(m);
}


static Point translation(const osg::MatrixTransform &transform)
{
  osg::Vec3d t = transform.getMatrix().getTrans();
  float x = t.x();
  float y = t.y();
  float z = t.z();
  return Point{x,y,z};
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


static void
  setCoordinateAxes(
    osg::MatrixTransform &transform,
    const osg::Vec3f &x,
    const osg::Vec3f &y,
    const osg::Vec3f &z
  )
{
  auto m = transform.getMatrix();
  setCoordinateAxesOf(m,x,y,z);
  transform.setMatrix(m);
}


static OSGCoordinateAxes
  coordinateAxes(const osg::MatrixTransform &transform)
{
  auto m = transform.getMatrix();
  return coordinateAxesOf(m);
}


static void addFloorTo(osg::MatrixTransform &matrix_transform)
{
  matrix_transform.addChild(createFloor());
}


OSGScene::OSGScene()
: top_node_ptr(createMatrixTransform()),
  top_handle(Impl::makeHandle(*this,addTransformTo(*top_node_ptr))),
  selection_handler(*this)
{
  osg::MatrixTransform &node = *top_node_ptr;

  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransform(*this,top_handle);

  addFloorTo(geometry_transform);
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


static osg::Geode&
  createGeode(
    osg::MatrixTransform &matrix_transform,
    osg::Material::ColorMode color_mode
  )
{
  GeodePtr geode_ptr = new osg::Geode;
  osg::Geode &geode = *geode_ptr;
  osg::ref_ptr<osg::Material> material = new osg::Material;
  material->setColorMode(color_mode);

  if (color_mode == osg::Material::DIFFUSE) {
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
    material->setShininess(osg::Material::FRONT_AND_BACK, 64.0f);
  }
  else {
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
  }

  geode.getOrCreateStateSet()->setAttributeAndModes(
    material, osg::StateAttribute::ON
  );
  matrix_transform.addChild(geode_ptr);
  return *geode_ptr;
}


auto
  OSGScene::Impl::createGeometryTransform(
    OSGScene &scene,
    TransformHandle parent,
    const ShapeParams &shape_params
  ) -> osg::MatrixTransform &
{
  osg::MatrixTransform &matrix_transform =
    addTransformTo(parentOf(Impl::geometryTransform(scene,parent)));

  osg::MatrixTransform &geometry_transform =
    addTransformTo(matrix_transform);

  osg::Geode &geode = createGeode(geometry_transform, shape_params.colorMode());
  geode.setName(shape_params.geode_name);
  geode.addDrawable(shape_params.createDrawable());
  return geometry_transform;
}


auto
  OSGScene::Impl::create(
    OSGScene &scene,
    TransformHandle parent,
    const ShapeParams &shape_params
  ) -> TransformHandle
{
  osg::MatrixTransform &geometry_transform =
    createGeometryTransform(scene,parent,shape_params);

  return makeHandle(scene,geometry_transform);
}


auto OSGScene::createSphere(TransformHandle parent) -> TransformHandle
{
  return Impl::create(*this,parent,SphereShapeParams());
}


auto OSGScene::createBox(TransformHandle parent) -> TransformHandle
{
  return Impl::create(*this,parent,BoxShapeParams());
}


auto OSGScene::createLine(TransformHandle parent) -> LineHandle
{
  osg::MatrixTransform &geometry_transform =
    Impl::createGeometryTransform(*this,parent,LineShapeParams());

  return Impl::makeLineHandle(*this,geometry_transform);
}


osg::MatrixTransform&
  OSGScene::Impl::geometryTransform(OSGScene &scene,TransformHandle handle)
{
  assert(scene.transform_ptrs[handle.index]);
  return *scene.transform_ptrs[handle.index];
}


const osg::MatrixTransform&
  OSGScene::Impl::geometryTransform(const OSGScene &scene,TransformHandle handle)
{
  assert(scene.transform_ptrs[handle.index]);
  return *scene.transform_ptrs[handle.index];
}


LineDrawable& OSGScene::Impl::lineDrawable(OSGScene &scene,LineHandle handle)
{
  osg::MatrixTransform &transform = Impl::geometryTransform(scene,handle);
  osg::Node *child_ptr = transform.getChild(0);
  assert(child_ptr);
  osg::Geode *geode_ptr = child_ptr->asGeode();
  assert(geode_ptr);
  osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);
  assert(drawable_ptr);
  LineDrawable *line_drawable_ptr = dynamic_cast<LineDrawable *>(drawable_ptr);
  assert(line_drawable_ptr);
  return *line_drawable_ptr;
}


void OSGScene::setGeometryScale(TransformHandle handle,float x,float y,float z)
{
  ::setScale(Impl::geometryTransform(*this,handle),x,y,z);
}


void OSGScene::setTranslation(TransformHandle handle,Point p)
{
  ::setTranslation(
    parentTransform(Impl::geometryTransform(*this,handle)),p.x(),p.y(),p.z()
  );
}


OSGScene::Point OSGScene::translation(TransformHandle handle) const
{
  return ::translation(parentTransform(Impl::geometryTransform(*this,handle)));
}


void OSGScene::setColor(TransformHandle handle,float r,float g,float b)
{
  ::setColor(Impl::geometryTransform(*this,handle),osg::Vec3f(r,g,b));
}


void OSGScene::setStartPoint(LineHandle handle,Point p)
{
  LineDrawable &line_drawable = Impl::lineDrawable(*this,handle);
  line_drawable.start_point = osg::Vec3f(p.x(),p.y(),p.z());
  line_drawable.setup();
}


void OSGScene::setEndPoint(LineHandle handle,Point p)
{
  LineDrawable &line_drawable = Impl::lineDrawable(*this,handle);
  line_drawable.end_point = osg::Vec3f(p.x(),p.y(),p.z());
  line_drawable.setup();
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


size_t
  OSGScene::Impl::getHandleIndex(OSGScene &scene,osg::MatrixTransform &transform)
{
  // Need to refactor this with makeLineHandle
  assert(!contains(scene.transform_ptrs,&transform));
  size_t index = findNull(scene.transform_ptrs);

  if (index == scene.transform_ptrs.size()) {
    scene.transform_ptrs.push_back(&transform);
  }
  else {
    scene.transform_ptrs[index] = &transform;
  }

  return index;
}


static osg::Vec3f worldPoint(const osg::Node &node,const osg::Vec3f &local)
{
  osg::MatrixList matrices = node.getWorldMatrices();
  assert(matrices.size() == 1);
  const osg::Matrixd &matrix = matrices[0];
  return local*matrix;
}


static osg::Vec3f localPoint(const osg::Node &node,const osg::Vec3f &world_point)
{
  osg::MatrixList matrices = node.getWorldMatrices();
  assert(matrices.size() == 1);
  osg::Matrixd matrix;
  matrix.invert(matrices[0]);
  return world_point*matrix;
}


auto
  OSGScene::Impl::makeHandle(
    OSGScene &scene,osg::MatrixTransform &transform
  ) -> TransformHandle
{
  return TransformHandle{getHandleIndex(scene,transform)};
}


auto
  OSGScene::Impl::makeLineHandle(
    OSGScene &scene,osg::MatrixTransform &transform
  ) -> LineHandle
{
  return LineHandle{getHandleIndex(scene,transform)};
}


auto OSGScene::top() const -> TransformHandle
{
  return top_handle;
}


auto OSGScene::worldPoint(Point p,TransformHandle t) const -> Point
{
  osg::Vec3f v1 =
    ::worldPoint(
      Impl::transform(*this,t),
      {p.x(),p.y(),p.z()}
    );

  osg::Vec3f v2 = ::localPoint(*top_node_ptr,v1);
  return {v2.x(),v2.y(),v2.z()};
}


static osg::Vec3f osgVec(const OSGScene::Vector &v)
{
  return {v.x,v.y,v.z};
}


static Scene::Vector vec(const osg::Vec3f &v)
{
  return {v.x(),v.y(),v.z()};
}


void OSGScene::setCoordinateAxes(TransformHandle t,const CoordinateAxes &axes)
{
  osg::Vec3f x = osgVec(axes.x);
  osg::Vec3f y = osgVec(axes.y);
  osg::Vec3f z = osgVec(axes.z);
  ::setCoordinateAxes(Impl::transform(*this,t),x,y,z);
}


CoordinateAxes OSGScene::coordinateAxes(TransformHandle t) const
{
  OSGCoordinateAxes osg_ca = ::coordinateAxes(Impl::transform(*this,t));
  Scene::Vector x = vec(osg_ca.x);
  Scene::Vector y = vec(osg_ca.y);
  Scene::Vector z = vec(osg_ca.z);
  return {x,y,z};
}
