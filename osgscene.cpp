#include "osgscene.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <osg/AutoTransform>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osgManipulator/TranslateAxisDragger>
#include <osgManipulator/TrackballDragger>
#include <osgManipulator/TabBoxDragger>
#include <osgViewer/View>
#include <osgViewer/ViewerEventHandlers>
#include <osg/io_utils>
#include <osgUtil/SmoothingVisitor>
#include "osgutil.hpp"
#include "osgpickhandler.hpp"
#include "osgcameramanipulator.hpp"
#include "contains.hpp"
#include "matchconst.hpp"
#include "eigenconv.hpp"
#include "indicesof.hpp"

namespace {
struct ScaleDragger;
struct TranslateDragger;
struct RotateDragger;
}


using std::cerr;
using std::string;
using DraggerType = Scene::ManipulatorType;
using GeodePtr = osg::ref_ptr<osg::Geode>;
using ViewPtr = osg::ref_ptr<osgViewer::View>;
using GroupPtr = osg::ref_ptr<osg::Group>;
using AutoTransformPtr = osg::ref_ptr<osg::AutoTransform>;
using DraggerPtr = osg::ref_ptr<osgManipulator::Dragger>;
using TransformHandle = Scene::TransformHandle;
using GeometryHandle = Scene::GeometryHandle;
using LineHandle = Scene::LineHandle;
using RotateDraggerPtr = osg::ref_ptr<RotateDragger>;
using TranslateDraggerPtr = osg::ref_ptr<TranslateDragger>;
using ScaleDraggerPtr = osg::ref_ptr<ScaleDragger>;


static bool
isValidDraggerEvent(const osgGA::GUIEventAdapter& ea)
{
  // Prevent the dragger from being activated if we're holding down
  // the ALT key, since we want that to be reserved for view manipulation.

  if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH) {
    if ((ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT) != 0) {
      return false;
    }
  }

  return true;
}


namespace {
struct ScaleDragger : osgManipulator::TabBoxDragger {
  bool
    handle(
      const osgManipulator::PointerInfo& pi,
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter& aa
    ) override
  {
    if (!isValidDraggerEvent(ea)) {
      return false;
    }

    return osgManipulator::TabBoxDragger::handle(pi, ea, aa);
  }
};
}


namespace {
struct TranslateDragger : osgManipulator::TranslateAxisDragger {
  bool
    handle(
      const osgManipulator::PointerInfo& pi,
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter& aa
    ) override
  {
    if (!isValidDraggerEvent(ea)) {
      return false;
    }

    return osgManipulator::TranslateAxisDragger::handle(pi, ea, aa);
  }
};
}


namespace {
struct RotateDragger : osgManipulator::TrackballDragger {
  bool
    handle(
      const osgManipulator::PointerInfo& pi,
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter& aa
    ) override
  {
    if (!isValidDraggerEvent(ea)) {
      return false;
    }

    return osgManipulator::TrackballDragger::handle(pi, ea, aa);
  }
};
}


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

  LineDrawable(
    const osg::Vec3 &start_point,
    const osg::Vec3 &end_point
  )
  : start_point(start_point),
    end_point(end_point)
  {
  }

  void setup()
  {
    LineDrawable &self = *this;
    osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array;
    points->push_back(start_point);
    points->push_back(end_point);
    osg::ref_ptr<osg::Vec4Array> color_array = new osg::Vec4Array;
    color_array->push_back(osg::Vec4(color, 1.0));

    removeAllPrimativeSets(self);
    self.setVertexArray(points.get());
    self.setColorArray(color_array.get());
    self.setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    self.addPrimitiveSet(new osg::DrawArrays(GL_LINES,/*first*/0,/*count*/2));
  }
};
}


namespace {
struct LessMeshVertex {
  bool operator()(const Mesh::Vertex &a, const Mesh::Vertex &b) const
  {
    if (a.position_index != b.position_index) {
      return a.position_index < b.position_index;
    }

    return a.normal_index < b.normal_index;
  }
};
}


namespace {
struct MeshDataBuilder {
  std::map<Mesh::Vertex, unsigned short, LessMeshVertex> vertex_to_index_map;
  using Index = unsigned short;
  const Mesh &mesh;
  osg::Vec3Array &positions;
  osg::Vec3Array &normals;

  MeshDataBuilder(
    const Mesh &mesh,
    osg::Vec3Array &positions,
    osg::Vec3Array &normals
  )
  : mesh(mesh), positions(positions), normals(normals)
  {
  }

  Index index(const Mesh::Vertex &vertex)
  {
    auto iter = vertex_to_index_map.find(vertex);

    if (iter == vertex_to_index_map.end()) {
      assert(positions.size() <= std::numeric_limits<Index>::max());
      assert(positions.size() == normals.size());
      Index index = positions.size();
      auto &position = mesh.positions[vertex.position_index];
      auto &normal = mesh.normals[vertex.normal_index];
      positions.push_back({position.x, position.y, position.z});
      normals.push_back({normal.x, normal.y, normal.z});
      vertex_to_index_map.insert({vertex, index});
      return index;
    }
    else {
      return iter->second;
    }
  }
};
}


namespace {
struct Range {
  float min = FLT_MAX, max = -FLT_MAX;
};
}


static void expand(Range &range, float value)
{
  if (value < range.min) {
    range.min = value;
  }

  if (value > range.max) {
    range.max = value;
  }
}


static osg::Vec3f calculateSize(const Mesh &mesh)
{
  Range x_range;
  Range y_range;
  Range z_range;

  for (auto &position : mesh.positions) {
    expand(x_range, position.x);
    expand(y_range, position.y);
    expand(z_range, position.z);
  }

  float x = std::max(-x_range.min, x_range.max);
  float y = std::max(-y_range.min, y_range.max);
  float z = std::max(-z_range.min, z_range.max);

  return {x,y,z};
}


namespace {
struct MeshDrawable : osg::Geometry {
  osg::Vec3f color = osg::Vec3(1,1,1);
  osg::Vec3f size = {1,1,1};
  Mesh mesh;

  void setup()
  {
    MeshDrawable &self = *this;
    osg::ref_ptr<osg::Vec3Array> points_ptr = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals_ptr = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> colors_ptr = new osg::Vec3Array;
    MeshDataBuilder builder(mesh, *points_ptr, *normals_ptr);
    vector<unsigned short> indices;

    for (auto &triangle : mesh.triangles) {
      indices.push_back(builder.index(triangle.v1));
      indices.push_back(builder.index(triangle.v2));
      indices.push_back(builder.index(triangle.v3));
    }

    colors_ptr->push_back(color);

    self.setVertexArray(points_ptr.get());
    self.setNormalArray(normals_ptr.get(), osg::Array::BIND_PER_VERTEX);
    self.setColorArray(colors_ptr.get(), osg::Array::BIND_OVERALL);

    self.addPrimitiveSet(
      new osg::DrawElementsUShort(GL_TRIANGLES, indices.begin(), indices.end())
    );

    self.size = calculateSize(mesh);
  }
};
}


class OSGScene::SelectionHandler : public OSGSelectionHandler {
  public:
    bool use_screen_relative_dragger = false;
    OSGScene &scene;

    SelectionHandler(OSGScene &);
    void updateDraggerPosition();
    void changeSelectedGeodeTo(osg::Geode *);
    void changeSelectedTransformTo(osg::MatrixTransform *);
    void clearSelection();
    void attachDragger(DraggerType);
    osg::Geode *selectedGeodePtr() const { return _selected_geode_ptr; }

    osg::MatrixTransform *selectedTransformPtr() const
    {
      return _selected_transform_ptr;
    }

  private:
    osg::Geode *_selected_geode_ptr = nullptr;
    osg::MatrixTransform *_selected_transform_ptr = nullptr;
    osg::Geode *_translate_dragger_geode_ptr = nullptr;
    osg::MatrixTransform *_translate_dragger_transform_ptr = nullptr;
    osg::Geode *_rotate_dragger_geode_ptr = nullptr;
    osg::MatrixTransform *_rotate_dragger_transform_ptr = nullptr;
    osg::Geode *_scale_dragger_geode_ptr = nullptr;
    osgManipulator::Dragger *_scale_dragger_ptr = nullptr;

    osg::Vec3 _old_color;

    void nodeClicked(osg::Node *) override;
    void removeExistingDraggers();
    void attachDragger(osg::Geode &, DraggerType);
    void attachDragger(osg::MatrixTransform &, DraggerType);
    void attachTranslateDraggerToGeode(osg::Geode &);
    void attachTranslateDraggerToTransform(osg::MatrixTransform &);

    void
      attachDraggerTo(
        osg::MatrixTransform &,
        DraggerType,
        const osg::Vec3 &shape_size
      );

    void attachRotateDraggerToGeode(osg::Geode &);
    void attachRotateDraggerToTransform(osg::MatrixTransform &);
    void attachScaleDraggerTo(osg::Geode &);
};


OSGScene::SelectionHandler &OSGScene::selectionHandler()
{
  return *_selection_handler_ptr;
}


const OSGScene::SelectionHandler &OSGScene::selectionHandler() const
{
  return *_selection_handler_ptr;
}


static osg::MatrixTransform &addTransformToGroup(osg::Group &parent)
{
  MatrixTransformPtr transform_ptr = new osg::MatrixTransform;
  osg::MatrixTransform &matrix_transform = *transform_ptr;
  parent.addChild(transform_ptr);
  return matrix_transform;
}


static const osg::MatrixTransform &
parentTransform(const osg::MatrixTransform &t)
{
  const osg::Node *parent_ptr = t.getParent(0);
  assert(parent_ptr);
  const osg::Transform *transform2_ptr = parent_ptr->asTransform();

  if (!transform2_ptr) {
    cerr << "Parent of " << t.getName() << " is not a transform\n";
    cerr << "  it is a " << parent_ptr->className() << "\n";
  }

  assert(transform2_ptr);
  const osg::MatrixTransform *matrix2_transform_ptr =
    transform2_ptr->asMatrixTransform();
  assert(matrix2_transform_ptr);
  return *matrix2_transform_ptr;
}


static osg::MatrixTransform &parentTransform(osg::MatrixTransform &t)
{
  const osg::MatrixTransform &const_t = t;
  const osg::MatrixTransform &const_result = parentTransform(const_t);
  return const_cast<osg::MatrixTransform&>(const_result);
}


struct OSGScene::Impl {
  struct DraggerCallback;

  static void clearHandle(size_t index, OSGScene &scene);
  static void destroyIndex(size_t index, OSGScene &);

  static osg::MatrixTransform &
    createTransform(OSGScene &scene, TransformHandle parent);

  static osg::MatrixTransform &
    createShape(
      osg::MatrixTransform &transform,
      const ShapeParams &shape_params
    );

  static GeometryHandle
    createGeometry(
      const ShapeParams &,
      TransformHandle parent,
      OSGScene &scene
    );

  static void
    destroyGeometryTransform(
      OSGScene &scene,
      osg::MatrixTransform &geometry_transform
    );

  static ViewPtr
    setupViewInGraphicsWindow(
      GraphicsWindowPtr graphics_window_ptr,
      ViewType view_type,
      OSGScene &
    );

  static size_t newHandleIndex(OSGScene &scene);

  template <typename OSGScene>
  static MatchConst_t<osg::MatrixTransform, OSGScene> &
  geometryTransformForHandle(
    OSGScene &scene,
    const GeometryHandle &handle
  )
  {
    size_t index = handle.index;
    assert(scene._handle_datas[index].geometry_transform_ptr);
    return *scene._handle_datas[index].geometry_transform_ptr;
  }

  template <typename MatrixTransform>
  static MatrixTransform &
  transformFromGeometryTransform(MatrixTransform &geometry_transform)
  {
    return ::parentTransform(geometry_transform);
  }

  template <typename OSGScene>
  static MatchConst_t<osg::MatrixTransform, OSGScene> &
  transformForHandle(
    OSGScene &scene,
    const TransformHandle &handle
  )
  {
    auto *transform_ptr = scene._handle_datas[handle.index].transform_ptr;
    assert(transform_ptr);
    return *transform_ptr;
  }

  static bool isTransformOfSelection(TransformHandle, OSGScene &);

  static TransformHandle
    makeHandleFromTransform(
      OSGScene &scene,
      osg::MatrixTransform &transform
    );

  static GeometryHandle
    makeHandleFromGeometryTransform(
      OSGScene &scene,
      osg::MatrixTransform &geometry_transform
    );

  static LineDrawable& lineDrawable(OSGScene &scene, LineHandle handle);

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

  static Optional<size_t>
    findGeometryTransformIndex(
      const OSGScene &,
      const osg::MatrixTransform &geometry_transform
    );

  static Optional<size_t>
    findTransformIndex(const OSGScene &,const osg::MatrixTransform &);

  template <typename Scene>
  static MatchConst_t<osg::Geode,Scene> &
  geodeForHandle(Scene &scene, GeometryHandle handle)
  {
    auto *geode_ptr =
      geometryTransformForHandle(scene, handle)
      .getChild(0)->asGeode();

    assert(geode_ptr);
    return *geode_ptr;
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


namespace {
struct HomePosition {
  const osg::Vec3d eye;
  const osg::Vec3d center;
  const osg::Vec3d up;
};
}


namespace {
struct ViewParams {
  const HomePosition home_position;
  const bool disable_rotate;
  const bool vertical_axis_fixed;
};
}


static HomePosition homePosition(ViewType view_type)
{
  osg::Quat orient = worldRotation();

  switch (view_type) {
    case ViewType::front:
    {
      osg::Vec3d eye = orient*osg::Vec3d(0,0,10);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,1,0);
      return HomePosition{eye,center,up};
    }
    case ViewType::top:
    {
      osg::Vec3d eye = orient*osg::Vec3d(0,10,0);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,0,-1);
      return HomePosition{eye,center,up};
    }
    case ViewType::side:
    {
      osg::Vec3d eye = orient*osg::Vec3d(10,0,0);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,1,0);
      return HomePosition{eye,center,up};
    }
    case ViewType::free:
    {
      osg::Vec3d eye = orient*osg::Vec3d(10,10,10);
      osg::Vec3d center = orient*osg::Vec3d(0,0,0);
      osg::Vec3d up = orient*osg::Vec3d(0,1,0);
      return HomePosition{eye,center,up};
    }
  }

  assert(false);
}


static void
  configureCameraManipatulator(
    OSGCameraManipulator &manipulator,
    const ViewParams &params
  )
{
  const HomePosition &home_position = params.home_position;

  manipulator.setHomePosition(
    home_position.eye,
    home_position.center,
    home_position.up
  );

  manipulator.setAllowThrow(false);
  manipulator.disable_rotate = params.disable_rotate;
  manipulator.setVerticalAxisFixed(params.vertical_axis_fixed);
}


static ViewParams viewParams(ViewType view_type)
{
  bool disable_rotate = view_type!=ViewType::free;
  bool vertical_axis_fixed = view_type==ViewType::free;
  HomePosition home_position = homePosition(view_type);
  ViewParams params = {home_position,disable_rotate,vertical_axis_fixed};
  return params;
}


static CameraManipulatorPtr createCameraManipulator(ViewType view_type)
{
  osg::ref_ptr<OSGCameraManipulator> manipulator_ptr(new OSGCameraManipulator);
  configureCameraManipatulator(*manipulator_ptr,viewParams(view_type));
  return manipulator_ptr;
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
  view->addEventHandler(new OSGPickHandler(view,&scene.selectionHandler()));

  osg::ref_ptr<osgGA::CameraManipulator> manipulator_ptr =
    createCameraManipulator(view_type);

  view->setCameraManipulator(manipulator_ptr);
  scene._composite_viewer.addView(view);
  return view;
}


GraphicsWindowPtr OSGScene::createGraphicsWindow(ViewType view_type)
{
  GraphicsWindowPtr window_ptr = createGraphicsWindowWithoutView();
  Impl::setupViewInGraphicsWindow(window_ptr,view_type,*this);
  osgViewer::GraphicsWindow::Views views;
  window_ptr->getViews(views);
  assert(_top_node_ptr);
  views.front()->setSceneData(&*_top_node_ptr);
  return window_ptr;
}


static osg::MatrixTransform &geometryTransformOf(osg::Geode &geode)
{
  osg::Node *parent_ptr = geode.getParent(0);
  osg::Transform *transform_ptr = parent_ptr->asTransform();
  assert(transform_ptr);
  osg::MatrixTransform *geometry_transform_ptr =
    transform_ptr->asMatrixTransform();
  assert(geometry_transform_ptr);
  return *geometry_transform_ptr;
}


static osg::MatrixTransform &bodyTransformOf(osg::Geode &geode)
{
  return parentTransform(geometryTransformOf(geode));
}


static osg::Group &parentOf(osg::Node &t)
{
  osg::Group *group_ptr = t.getParent(0);
  assert(group_ptr);
  return *group_ptr;
}


static void
replaceDraggerGroupWithTransform(osg::MatrixTransform &body_transform)
{
  MatrixTransformPtr transform_ptr = &body_transform;
    // This owns the transform after it is removed from the parent.
    // This prevents the transform from being deleted.

  // * parent               * parent
  //   * group                * body transform
  //     * body transform   ->
  //     * dragger
  osg::Group &group = parentOf(body_transform);
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


static void scaleAxisXY(osg::Node *axis_node_ptr,float scale_factor)
{
  osg::MatrixTransform *axis_transform_ptr =
    axis_node_ptr->asTransform()->asMatrixTransform();

  osg::Matrixd scale = osg::Matrix::scale(scale_factor, scale_factor, 1);
  axis_transform_ptr->setMatrix(scale*axis_transform_ptr->getMatrix());
}


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
static TranslateDraggerPtr
  createScreenRelativeTranslateDragger(
    MatrixTransformPtr transform_ptr,
    osgManipulator::DraggerCallback &
  )
{
  TranslateDraggerPtr parent_dragger_ptr(new TranslateDragger);
  TranslateDraggerPtr dragger_ptr = new TranslateDragger();
  dragger_ptr->setParentDragger(parent_dragger_ptr);
  dragger_ptr->setupDefaultGeometry();
  assert(dragger_ptr->getNumChildren()==3);
  float thickness = 4;
  scaleAxisXY(dragger_ptr->getChild(0), thickness);
  scaleAxisXY(dragger_ptr->getChild(1), thickness);
  scaleAxisXY(dragger_ptr->getChild(2), thickness);
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


static void
  setScale(osg::MatrixTransform &transform,float x,float y,float z)
{
  auto m = transform.getMatrix();
  setScale(m, osg::Vec3f(x,y,z));
  transform.setMatrix(m);
}


static Vec3 scale(const osg::MatrixTransform &transform)
{
  auto m = transform.getMatrix();
  osg::Vec3d scale = m.getScale();
  float x = scale.x();
  float y = scale.y();
  float z = scale.z();
  return Vec3{x, y, z};
}


static osg::Matrix
draggerMatrix(
  osg::MatrixTransform &transform_updating,
  DraggerType dragger_type,
  osg::Vec3 shape_size
)
{
  osg::Matrix matrix = transform_updating.getMatrix();

  if (dragger_type == DraggerType::scale) {
    osg::Vec3d translation;
    osg::Quat rotation;
    osg::Vec3d scale;
    osg::Quat scale_orient;
    matrix.decompose(translation, rotation, scale, scale_orient);
    scale.x() *= shape_size.x()*2;
    scale.y() *= shape_size.y()*2;
    scale.z() *= shape_size.z()*2;

    scale.x() += 0.1;
    scale.y() += 0.1;
    scale.z() += 0.1;

    matrix = compose(translation, rotation, scale, scale_orient);
  }
  else {
    matrix = osg::Matrix::scale(2,2,2)*matrix;
  }

  return matrix;
}


static DraggerPtr
  createObjectRelativeDragger(
    osg::MatrixTransform &transform_updating,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback,
    const osg::Vec3 &shape_size
  )
{
  DraggerPtr dragger_ptr;

  using HandleCommandMask =
    osgManipulator::DraggerTransformCallback::HandleCommandMask;

  HandleCommandMask
    handle_command_mask = HandleCommandMask::HANDLE_ALL;

  switch (dragger_type) {
    case DraggerType::translate:
      {
        TranslateDraggerPtr translate_dragger_ptr = new TranslateDragger;
        translate_dragger_ptr->setupDefaultGeometry();
        assert(translate_dragger_ptr->getNumChildren()==3);
        float thickness = 4;
        scaleAxisXY(translate_dragger_ptr->getChild(0),thickness);
        scaleAxisXY(translate_dragger_ptr->getChild(1),thickness);
        scaleAxisXY(translate_dragger_ptr->getChild(2),thickness);
        dragger_ptr = translate_dragger_ptr;
      }
      break;
    case DraggerType::rotate:
      {
        RotateDraggerPtr rotate_dragger_ptr = new RotateDragger;
        rotate_dragger_ptr->setupDefaultGeometry();
        dragger_ptr = rotate_dragger_ptr;
      }
      break;
    case DraggerType::scale:
      {
        ScaleDraggerPtr scale_dragger_ptr = new ScaleDragger;
        scale_dragger_ptr->setupDefaultGeometry();
        dragger_ptr = scale_dragger_ptr;
      }
      break;
  }

  assert(isDragger(dragger_ptr));

  {
    osgManipulator::Dragger &dragger = *dragger_ptr;

    dragger.setActivationMouseButtonMask(
      osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON
    );

    dragger.setHandleEvents(true);
    dragger.addTransformUpdating(&transform_updating, handle_command_mask);

    dragger.setMatrix(
      draggerMatrix(transform_updating, dragger_type, shape_size)
    );

    dragger.addDraggerCallback(&dragger_callback);
  }

  return dragger_ptr;
}


static DraggerPtr
  createDragger(
    osg::MatrixTransform& transform,
    bool screen_relative,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback,
    const osg::Vec3 &shape_size
  )
{
  switch (dragger_type) {
    case DraggerType::translate:
      if (screen_relative) {
        return
          createScreenRelativeTranslateDragger(&transform,dragger_callback);
      }
      else {
        return
          createObjectRelativeDragger(
            transform,
            DraggerType::translate,
            dragger_callback,
            shape_size
          );
      }
    case DraggerType::rotate:
      if (screen_relative) {
        assert(false); // not implemented
      }
      else {
        return
          createObjectRelativeDragger(
            transform,
            DraggerType::rotate,
            dragger_callback,
            shape_size
          );
      }
    case DraggerType::scale:
      if (screen_relative) {
        assert(false);
      }
      else {
        return
          createObjectRelativeDragger(
            transform,
            DraggerType::scale,
            dragger_callback,
            shape_size
          );
      }
  }

  assert(false); // shouldn't happen
  return nullptr;
}


static void
  replaceTransformWithDraggerGroup(
    osg::MatrixTransform& body_transform,
    bool screen_relative,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback,
    const osg::Vec3 &shape_size
  )
{
  // The hierarchy is changed like this:
  //
  // * parent                 * parent
  //   * body transform  ->     * group
  //                              * body transform
  //                              * dragger

  MatrixTransformPtr matrix_transform_ptr = &body_transform;
    // This pointer owns the transform while we remove it from the parent.
    // This avoids the transform being deleted.

  osg::Group &parent_body_transform = parentOf(body_transform);
  parent_body_transform.removeChild(&body_transform);
  GroupPtr group_ptr = createGroup();
  parent_body_transform.addChild(group_ptr);

  DraggerPtr dragger_ptr =
    createDragger(
      body_transform,
      screen_relative,
      dragger_type,
      dragger_callback,
      shape_size
    );

  group_ptr->addChild(&body_transform);
  group_ptr->addChild(dragger_ptr);
}


OSGScene::SelectionHandler::SelectionHandler(OSGScene &scene_arg)
: scene(scene_arg)
{
}


Optional<size_t>
  OSGScene::Impl::findGeometryTransformIndex(
    const OSGScene &scene,
    const osg::MatrixTransform &geometry_transform
  )
{
  size_t n_handles = scene._handle_datas.size();

  for (size_t index = 0; index != n_handles; ++index) {
    osg::MatrixTransform *transform_ptr =
      scene._handle_datas[index].geometry_transform_ptr;

    if (transform_ptr == &geometry_transform) {
      return index;
    }
  }

  return {};
}


Optional<size_t>
OSGScene::Impl::findTransformIndex(
  const OSGScene &scene,
  const osg::MatrixTransform &transform
)
{
  size_t n_handles = scene._handle_datas.size();

  for (size_t index = 0; index != n_handles; ++index) {
    if (scene._handle_datas[index].transform_ptr == &transform) {
      return index;
    }
  }

  cerr << "Didn't find transform for selection\n";
  return {};
}


Optional<GeometryHandle> OSGScene::selectedGeometry() const
{
  osg::Geode *geode_ptr = selectionHandler().selectedGeodePtr();

  if (!geode_ptr) {
    return {};
  }

  osg::MatrixTransform &geometry_transform = geometryTransformOf(*geode_ptr);

  Optional<size_t> maybe_geometry_index =
    Impl::findGeometryTransformIndex(*this, geometry_transform);

  if (!maybe_geometry_index) {
    cerr << "Didn't find geometry for selection\n";
    return {};
  }

  return GeometryHandle{*maybe_geometry_index};
}


Optional<TransformHandle> OSGScene::selectedTransform() const
{
  osg::MatrixTransform *transform_ptr =
    selectionHandler().selectedTransformPtr();

  if (!transform_ptr) {
    return {};
  }

  Optional<size_t> maybe_transform_index =
    Impl::findTransformIndex(*this, *transform_ptr);

  if (!maybe_transform_index) {
    cerr << "Didn't find transform for selection\n";
    return {};
  }

  return TransformHandle{*maybe_transform_index};
}


TransformHandle OSGScene::parentTransform(GeometryHandle geometry_handle) const
{
  const OSGScene &scene = *this;

  const osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(scene, geometry_handle);

  const osg::MatrixTransform &transform =
    Impl::transformFromGeometryTransform(geometry_transform);

  Optional<size_t> maybe_transform_index =
    Impl::findTransformIndex(scene, transform);

  assert(maybe_transform_index);
  size_t transform_index = *maybe_transform_index;

  TransformHandle transform_handle{transform_index};
  return transform_handle;
}


static osg::Vec3 selectionColor()
{
  return {1,1,0};
}


static osg::Vec4f vec4(const osg::Vec3f &v)
{
  return {v.x(), v.y(), v.z(), 1};
}


static osg::Vec3f vec3(const osg::Vec4f &v)
{
  return {v.x(), v.y(), v.z()};
}


static osg::ShapeDrawable *maybeShapeDrawable(osg::Drawable &drawable)
{
  return dynamic_cast<osg::ShapeDrawable*>(&drawable);
}


static LineDrawable *maybeLineDrawable(osg::Drawable &drawable)
{
  return dynamic_cast<LineDrawable*>(&drawable);
}


static const LineDrawable *maybeLineDrawable(const osg::Drawable &drawable)
{
  return dynamic_cast<const LineDrawable*>(&drawable);
}


static MeshDrawable *maybeMeshDrawable(osg::Drawable &drawable)
{
  return dynamic_cast<MeshDrawable*>(&drawable);
}


static const MeshDrawable *maybeMeshDrawable(const osg::Drawable &drawable)
{
  return dynamic_cast<const MeshDrawable*>(&drawable);
}


static osg::Vec3f geodeColor(osg::Geode &geode)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  osg::Drawable &drawable = *drawable_ptr;

  if (auto *shape_drawable_ptr = maybeShapeDrawable(drawable)) {
    return vec3(shape_drawable_ptr->getColor());
  }

  if (auto *line_drawable_ptr = maybeLineDrawable(drawable)) {
    return line_drawable_ptr->color;
  }

  if (auto *mesh_drawable_ptr = maybeMeshDrawable(drawable)) {
    return mesh_drawable_ptr->color;
  }

  cerr << "geodeColor: unknown drawable: " << drawable.className() << "\n";

  assert(false);
  return {0,0,0};
}


static void setDrawableColor(osg::Drawable &drawable, const osg::Vec3f &color)
{
  if (auto *shape_drawable_ptr = maybeShapeDrawable(drawable)) {
    shape_drawable_ptr->setColor(vec4(color));
    return;
  }

  if (auto *geometry_ptr = maybeLineDrawable(drawable)) {
    geometry_ptr->color = color;
    geometry_ptr->setup();
    return;
  }

  if (auto *mesh_ptr = maybeMeshDrawable(drawable)) {
    mesh_ptr->color = color;
    mesh_ptr->setup();
    return;
  }

  cerr << "setDrawableColor: unknown drawable type\n";
}


static void setGeodeColor(osg::Geode &geode,const osg::Vec3f &vec)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  setDrawableColor(*drawable_ptr, vec);
}


void OSGScene::SelectionHandler::clearSelection()
{
  if (_selected_geode_ptr) {
    setGeodeColor(*_selected_geode_ptr, _old_color);
    _selected_geode_ptr = nullptr;
  }

  _selected_transform_ptr = nullptr;
  removeExistingDraggers();
}


void
  OSGScene::SelectionHandler::changeSelectedGeodeTo(
    osg::Geode *new_selected_node_ptr
  )
{
  clearSelection();
  _selected_geode_ptr = new_selected_node_ptr;

  if (_selected_geode_ptr) {
    _old_color = geodeColor(*_selected_geode_ptr);
    setGeodeColor(*_selected_geode_ptr, selectionColor());
  }
}


void
OSGScene::SelectionHandler::changeSelectedTransformTo(
  osg::MatrixTransform *new_selected_node_ptr
)
{
  clearSelection();
  _selected_transform_ptr = new_selected_node_ptr;
}


void OSGScene::SelectionHandler::removeExistingDraggers()
{
  if (_translate_dragger_geode_ptr) {
    osg::MatrixTransform &body_transform =
      bodyTransformOf(*_translate_dragger_geode_ptr);

    replaceDraggerGroupWithTransform(body_transform);
    _translate_dragger_geode_ptr = 0;
  }

  if (_translate_dragger_transform_ptr) {
    replaceDraggerGroupWithTransform(*_translate_dragger_transform_ptr);
    _translate_dragger_transform_ptr = 0;
  }

  if (_rotate_dragger_geode_ptr) {
    osg::MatrixTransform &body_transform =
      bodyTransformOf(*_rotate_dragger_geode_ptr);

    replaceDraggerGroupWithTransform(body_transform);
    _rotate_dragger_geode_ptr = 0;
  }

  if (_rotate_dragger_transform_ptr) {
    replaceDraggerGroupWithTransform(*_rotate_dragger_transform_ptr);
    _rotate_dragger_transform_ptr = 0;
  }

  if (_scale_dragger_geode_ptr) {
    osg::MatrixTransform &geometry_transform =
      geometryTransformOf(*_scale_dragger_geode_ptr);

    osg::MatrixTransform &transform = ::parentTransform(geometry_transform);

    transform.removeChild(_scale_dragger_ptr);
    _scale_dragger_geode_ptr = 0;
    _scale_dragger_ptr = 0;
  }
}


void
OSGScene::SelectionHandler::attachDraggerTo(
  osg::MatrixTransform &transform,
  DraggerType dragger_type,
  const osg::Vec3 &shape_size
)
{
  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  replaceTransformWithDraggerGroup(
    transform,
    use_screen_relative_dragger,
    dragger_type,
    *dc,
    shape_size
  );
}


static osg::Vec3 shapeSize(const osg::Geode &geode)
{
  const osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);

  if (auto *mesh_drawable_ptr = maybeMeshDrawable(*drawable_ptr)) {
    return mesh_drawable_ptr->size;
  }

  return {0.5, 0.5, 0.5};
}


void
OSGScene::SelectionHandler::attachTranslateDraggerToGeode(
  osg::Geode &new_selected_node
)
{
  assert(!_translate_dragger_geode_ptr);
  _translate_dragger_geode_ptr = &new_selected_node;

  osg::MatrixTransform &transform =
    bodyTransformOf(*_translate_dragger_geode_ptr);

  attachDraggerTo(
    transform, DraggerType::translate, shapeSize(new_selected_node)
  );
}


void
OSGScene::SelectionHandler::attachTranslateDraggerToTransform(
  osg::MatrixTransform &transform
)
{
  assert(!_translate_dragger_transform_ptr);
  _translate_dragger_transform_ptr = &transform;
  attachDraggerTo(transform, DraggerType::translate, /*shape_size*/{1,1,1});
}


void
OSGScene::SelectionHandler::attachRotateDraggerToGeode(
  osg::Geode &new_selected_node
)
{
  assert(!_rotate_dragger_geode_ptr);
  _rotate_dragger_geode_ptr = &new_selected_node;

  osg::MatrixTransform &transform =
    bodyTransformOf(*_rotate_dragger_geode_ptr);

  attachDraggerTo(transform, DraggerType::rotate, /*shape_size*/{1,1,1});
}


void
OSGScene::SelectionHandler::attachRotateDraggerToTransform(
  osg::MatrixTransform &transform
)
{
  assert(!_rotate_dragger_transform_ptr);
  _rotate_dragger_transform_ptr = &transform;
  attachDraggerTo(transform, DraggerType::rotate, /*shape_size*/{1,1,1});
}


void
OSGScene::SelectionHandler::attachScaleDraggerTo(
  osg::Geode &new_selected_node
)
{
  assert(!_scale_dragger_geode_ptr);
  _scale_dragger_geode_ptr = &new_selected_node;

  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  osg::MatrixTransform &geometry_transform =
    geometryTransformOf(*_scale_dragger_geode_ptr);

  DraggerPtr dragger_ptr =
    createDragger(
      geometry_transform,
      use_screen_relative_dragger,
      DraggerType::scale,
      *dc,
      shapeSize(new_selected_node)
    );

  ::parentTransform(geometry_transform).addChild(dragger_ptr);
  assert(!_scale_dragger_ptr);
  _scale_dragger_ptr = dragger_ptr;
}


static void setColor(osg::MatrixTransform &node,const osg::Vec3f &vec)
{
  assert(node.getNumChildren() == 1);
  osg::Node *child_ptr = node.getChild(0);
  assert(child_ptr);
  osg::Geode *geode_ptr = child_ptr->asGeode();

  if (geode_ptr) {
    setGeodeColor(*geode_ptr,vec);
  }
}


static const osg::Drawable &geodeDrawable(const osg::Geode &geode)
{
  const osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  return *drawable_ptr;
}


static osg::Drawable &geodeDrawable(osg::Geode &geode)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  return *drawable_ptr;
}


static const LineDrawable* maybeGeodeLine(const osg::Geode &geode)
{
  return maybeLineDrawable(geodeDrawable(geode));
}


static osg::ShapeDrawable* maybeGeodeShape(osg::Geode &geode)
{
  return maybeShapeDrawable(geodeDrawable(geode));
}


static MeshDrawable* maybeGeodeMesh(osg::Geode &geode)
{
  return maybeMeshDrawable(geodeDrawable(geode));
}


void OSGScene::SelectionHandler::nodeClicked(osg::Node *new_selected_node_ptr)
{
  osg::Geode *new_selected_geode_ptr = nullptr;

  if (new_selected_node_ptr) {
    osg::Geode *geode_ptr = new_selected_node_ptr->asGeode();

    assert(geode_ptr);

    if (maybeGeodeLine(*geode_ptr)) {
      new_selected_geode_ptr = geode_ptr;
    }
    else if (maybeGeodeShape(*geode_ptr)) {
      new_selected_geode_ptr = geode_ptr;
    }
    else if (maybeGeodeMesh(*geode_ptr)) {
      new_selected_geode_ptr = geode_ptr;
    }
  }

  changeSelectedGeodeTo(new_selected_geode_ptr);

  if (scene.selection_changed_callback) {
    scene.selection_changed_callback();
  }
}


void OSGScene::attachManipulatorToSelectedNode(DraggerType dragger_type)
{
  selectionHandler().attachDragger(dragger_type);
}


Optional<LineHandle> OSGScene::maybeLine(GeometryHandle handle) const
{
  if (maybeGeodeLine(Impl::geodeForHandle(*this, handle))) {
    return LineHandle(handle);
  }
  else {
    return {};
  }
}


static osg::ref_ptr<osg::ShapeDrawable> createSphereDrawable()
{
  osg::ref_ptr<osg::Sphere> sphere_ptr = new osg::Sphere;

  osg::ref_ptr<osg::TessellationHints> tesselation_hints_ptr =
    new osg::TessellationHints;

  tesselation_hints_ptr->setDetailRatio(1.0);

  osg::ref_ptr<osg::ShapeDrawable> drawable_ptr =
    new osg::ShapeDrawable(sphere_ptr, tesselation_hints_ptr);

  return drawable_ptr;
}


static osg::Vec3f osgVec3f(const Scene::Color &color)
{
  return osg::Vec3f(color.red, color.green, color.blue);
}


static osg::Vec3f osgVec3f(const OSGScene::Point &p)
{
  return osg::Vec3f(p.x, p.y, p.z);
}


static osg::ref_ptr<LineDrawable>
createLineDrawable(const Scene::Point &start, const Scene::Point &end)
{
  osg::Vec3f start_vec3f = osgVec3f(start);
  osg::Vec3f end_vec3f = osgVec3f(end);
  osg::ref_ptr<LineDrawable> line_ptr(new LineDrawable(start_vec3f, end_vec3f));
  line_ptr->setup();
  return line_ptr;
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


static osg::ref_ptr<MeshDrawable> createMeshDrawable(const Mesh &mesh)
{
  osg::ref_ptr<MeshDrawable> mesh_drawable_ptr(new MeshDrawable);
  mesh_drawable_ptr->mesh = mesh;
  mesh_drawable_ptr->setup();
  return mesh_drawable_ptr;
}


static void
  removeTransformFromGroup(
    osg::Group &parent,
    osg::Group &matrix_transform
  )
{
  parent.removeChild(&matrix_transform);
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
struct MeshShapeParams : ShapeParams {
  const Mesh &mesh;

  MeshShapeParams(const Mesh &mesh)
  : ShapeParams("Mesh Geode"),
    mesh(mesh)
  {
  }

  virtual osg::ref_ptr<osg::Drawable> createDrawable() const
  {
    return createMeshDrawable(mesh);
  }
};
}


namespace {
struct LineShapeParams : ShapeParams {
  Scene::Point start;
  Scene::Point end;

  LineShapeParams(const Scene::Point &start, const Scene::Point &end)
  : ShapeParams{"Line Geode"},
    start(start),
    end(end)
  {
  }

  osg::ref_ptr<osg::Drawable> createDrawable() const override
  {
    return createLineDrawable(start, end);
  }

  osg::Material::ColorMode colorMode() const override
  {
    return osg::Material::EMISSION;
  }
};
}


static void
  setTranslation(osg::MatrixTransform &transform,const OSGScene::Point &v)
{
  osg::Matrix m = transform.getMatrix();
  m.setTrans(v.x, v.y, v.z);
  transform.setMatrix(m);
}


static OSGScene::Point translation(const osg::MatrixTransform &transform)
{
  osg::Vec3d t = transform.getMatrix().getTrans();
  float x = t.x();
  float y = t.y();
  float z = t.z();
  return OSGScene::Point{x,y,z};
}


static void
  setRotatation(osg::MatrixTransform &transform,const osg::Quat &rot)
{
  auto m = transform.getMatrix();
  m.setRotate(rot);
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
: _top_node_ptr(new osg::MatrixTransform),
  _top_transform(Impl::makeHandleFromTransform(*this, *_top_node_ptr)),
  _top_geometry(
    Impl::makeHandleFromGeometryTransform(
      *this, addTransformToGroup(*_top_node_ptr)
    )
  ),
  _selection_handler_ptr(new SelectionHandler(*this))
{
  osg::MatrixTransform &node = *_top_node_ptr;

  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, _top_geometry);

  addFloorTo(geometry_transform);
  setRotatation(node,worldRotation());
  node.getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

  // disable the default setting of viewer.done() by pressing Escape.
  _composite_viewer.setKeyEventSetsDone(0);
  _composite_viewer.setThreadingModel(_composite_viewer.SingleThreaded);

  // This causes paintEvent to be called regularly.
  _timer.interval_in_milliseconds = 10;
  _timer.callback = [this]{ _composite_viewer.frame(); };
  _timer.start();
}


OSGScene::~OSGScene() = default;


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


osg::MatrixTransform &
OSGScene::Impl::createTransform(
  OSGScene &scene,
  TransformHandle parent
)
{
  osg::MatrixTransform &parent_transform = transformForHandle(scene, parent);
  osg::MatrixTransform &transform = addTransformToGroup(parent_transform);
  return transform;
}


osg::MatrixTransform &
OSGScene::Impl::createShape(
  osg::MatrixTransform &transform,
  const ShapeParams &shape_params
)
{
  osg::MatrixTransform &geometry_transform = addTransformToGroup(transform);
  osg::Geode &geode = createGeode(geometry_transform, shape_params.colorMode());
  geode.setName(shape_params.geode_name);
  geode.addDrawable(shape_params.createDrawable());
  return geometry_transform;
}


void
  OSGScene::Impl::destroyGeometryTransform(
    OSGScene &scene,
    osg::MatrixTransform &geometry_transform
  )
{
  osg::Geode *selected_geode_ptr = scene.selectionHandler().selectedGeodePtr();

  if (selected_geode_ptr) {
    osg::Group &selected_geometry_transform = parentOf(*selected_geode_ptr);

    if (&geometry_transform == &selected_geometry_transform) {
      scene.selectionHandler().changeSelectedGeodeTo(nullptr);
    }
  }

  removeTransformFromGroup(parentOf(geometry_transform), geometry_transform);
}


TransformHandle OSGScene::createTransform(TransformHandle parent)
{
  OSGScene &scene = *this;
  osg::MatrixTransform &transform = Impl::createTransform(scene, parent);

  TransformHandle transform_handle =
    Impl::makeHandleFromTransform(scene, transform);

  return transform_handle;
}


GeometryHandle
OSGScene::Impl::createGeometry(
  const ShapeParams &shape_params,
  TransformHandle transform_handle,
  OSGScene &scene
)
{
  osg::MatrixTransform &transform =
    Impl::transformForHandle(scene, transform_handle);

  osg::MatrixTransform &geometry_transform =
    Impl::createShape(transform, shape_params);

  GeometryHandle geometry_handle =
    Impl::makeHandleFromGeometryTransform(scene, geometry_transform);

  return geometry_handle;
}


GeometryHandle OSGScene::createBox(TransformHandle transform_handle)
{
  return Impl::createGeometry(BoxShapeParams(), transform_handle, *this);
}


LineHandle OSGScene::createLine(TransformHandle transform_handle)
{
  Scene::Point start(0,0,0);
  Scene::Point end{1,1,1};

  LineShapeParams shape_params = LineShapeParams(start, end);

  GeometryHandle geometry =
    Impl::createGeometry(shape_params, transform_handle, *this);

  return LineHandle(geometry);
}


GeometryHandle OSGScene::createSphere(TransformHandle transform_handle)
{
  return Impl::createGeometry(SphereShapeParams(), transform_handle, *this);
}


GeometryHandle
OSGScene::createMesh(TransformHandle parent_handle, const Mesh &mesh)
{
  return Impl::createGeometry(MeshShapeParams(mesh), parent_handle, *this);
}


void OSGScene::Impl::clearHandle(size_t index, OSGScene &scene)
{
  scene._handle_datas[index] = HandleData{};
}


LineDrawable&
OSGScene::Impl::lineDrawable(OSGScene &scene, LineHandle handle)
{
  osg::MatrixTransform &transform =
    Impl::geometryTransformForHandle(scene, handle);

  osg::Node *child_ptr = transform.getChild(0);
  assert(child_ptr);
  osg::Geode *geode_ptr = child_ptr->asGeode();
  assert(geode_ptr);

  LineDrawable *line_drawable_ptr =
    maybeLineDrawable(geodeDrawable(*geode_ptr));

  assert(line_drawable_ptr);
  return *line_drawable_ptr;
}


void OSGScene::setGeometryScale(GeometryHandle handle,const Vec3 &v)
{
  float x = v.x;
  float y = v.y;
  float z = v.z;

  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, handle);

  ::setScale(geometry_transform, x, y, z);

  if (selectedGeometry() == handle) {
    selectionHandler().updateDraggerPosition();
  }
}


void OSGScene::setGeometryCenter(GeometryHandle handle,const Point &v)
{
  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, handle);

  ::setTranslation(geometry_transform, v);

  if (selectedGeometry() == handle) {
    selectionHandler().updateDraggerPosition();
  }
}


Vec3 OSGScene::geometryScale(GeometryHandle handle) const
{
  return ::scale(Impl::geometryTransformForHandle(*this, handle));
}


OSGScene::Point OSGScene::geometryCenter(GeometryHandle handle) const
{
  return ::translation(Impl::geometryTransformForHandle(*this, handle));
}


OSGScene::Point OSGScene::translation(TransformHandle handle) const
{
  return ::translation(Impl::transformForHandle(*this,handle));
}


void OSGScene::setGeometryColor(GeometryHandle handle,const Color &color)
{
  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this,handle);

  ::setColor(geometry_transform, osgVec3f(color));
}


void OSGScene::setStartPoint(LineHandle handle,Point p)
{
  LineDrawable &line_drawable = Impl::lineDrawable(*this, handle);
  line_drawable.start_point = osgVec3f(p);
  line_drawable.setup();
}


void OSGScene::setEndPoint(LineHandle handle,Point p)
{
  LineDrawable &line_drawable = Impl::lineDrawable(*this,handle);
  line_drawable.end_point = osgVec3f(p);
  line_drawable.setup();
}


size_t OSGScene::Impl::newHandleIndex(OSGScene &scene)
{
  size_t n = scene._handle_datas.size();

  for (size_t i=0; i!=n; ++i) {
    if (!scene._handle_datas[i].geometry_transform_ptr && !scene._handle_datas[i].transform_ptr) {
      return i;
    }
  }

  scene._handle_datas.emplace_back();
  return n;
}


TransformHandle
OSGScene::Impl::makeHandleFromTransform(
  OSGScene &scene,
  osg::MatrixTransform &transform
)
{
  size_t transform_index = newHandleIndex(scene);
  scene._handle_datas[transform_index].transform_ptr = &transform;
  TransformHandle transform_handle{transform_index};
  return transform_handle;
}


GeometryHandle
OSGScene::Impl::makeHandleFromGeometryTransform(
  OSGScene &scene,
  osg::MatrixTransform &geometry_transform
)
{
  size_t geometry_index = newHandleIndex(scene);

  scene._handle_datas[geometry_index].geometry_transform_ptr =
    &geometry_transform;

  return GeometryHandle{geometry_index};
}


void OSGScene::Impl::destroyIndex(size_t index, OSGScene &scene)
{
  HandleData &handle_data = scene._handle_datas[index];

  if (handle_data.geometry_transform_ptr) {
    Impl::destroyGeometryTransform(scene, *handle_data.geometry_transform_ptr);
    handle_data.geometry_transform_ptr = nullptr;
  }

  if (handle_data.transform_ptr) {
    osg::MatrixTransform &transform = *handle_data.transform_ptr;
    removeTransformFromGroup(parentOf(transform), transform);
    handle_data.transform_ptr = nullptr;
  }

  Impl::clearHandle(index, scene);
}


void OSGScene::destroyGeometry(GeometryHandle handle)
{
  Optional<GeometryHandle> maybe_selected_geometry = selectedGeometry();

  if (maybe_selected_geometry) {
    if (maybe_selected_geometry->index == handle.index) {
      selectionHandler().clearSelection();
    }
  }

  size_t geometry_index = handle.index;
  Impl::destroyIndex(geometry_index, *this);
}


void OSGScene::destroyTransform(TransformHandle handle)
{
  Optional<TransformHandle> maybe_selected_transform = selectedTransform();

  if (maybe_selected_transform) {
    if (maybe_selected_transform->index == handle.index) {
      selectionHandler().clearSelection();
    }
  }

  Impl::destroyIndex(handle.index, *this);
}


auto OSGScene::top() const -> TransformHandle
{
  return _top_transform;
}


void
OSGScene::SelectionHandler::attachDragger(
  osg::Geode &dragger_node,
  DraggerType dragger_type
)
{
  switch (dragger_type) {
    case DraggerType::translate:
      attachTranslateDraggerToGeode(dragger_node);
      break;
    case DraggerType::rotate:
      attachRotateDraggerToGeode(dragger_node);
      break;
    case DraggerType::scale:
      attachScaleDraggerTo(dragger_node);
      break;
  }
}


void
OSGScene::SelectionHandler::attachDragger(
  osg::MatrixTransform &dragger_node,
  DraggerType dragger_type
)
{
  switch (dragger_type) {
    case DraggerType::translate:
      attachTranslateDraggerToTransform(dragger_node);
      break;
    case DraggerType::rotate:
      attachRotateDraggerToTransform(dragger_node);
      break;
    case DraggerType::scale:
      cerr << "need scene\n";
      break;
  }
}


void OSGScene::SelectionHandler::attachDragger(DraggerType dragger_type)
{
  removeExistingDraggers();

  if (_selected_geode_ptr) {
    attachDragger(*_selected_geode_ptr, dragger_type);
  }
  else {
    assert(_selected_transform_ptr);
    attachDragger(*_selected_transform_ptr, dragger_type);
  }
}


static void
  matchPose(
    osg::MatrixTransform &dragger,
    const osg::MatrixTransform &dragged_transform_node
  )
{
  osg::Matrix dragged_transform = dragged_transform_node.getMatrix();
  osg::Matrix dragger_transform = dragger.getMatrix();

  osg::Vec3f dragged_translation;
  osg::Quat dragged_rotation;
  osg::Vec3f dragged_scale;
  osg::Quat dragged_so;

  dragged_transform.decompose(
    dragged_translation, dragged_rotation, dragged_scale, dragged_so
  );

  osg::Vec3f dragger_translation;
  osg::Quat dragger_rotation;
  osg::Vec3f dragger_scale;
  osg::Quat dragger_so;

  dragger_transform.decompose(
    dragger_translation, dragger_rotation, dragger_scale, dragger_so
  );

  dragger_translation = dragged_translation;
  dragger_rotation = dragged_rotation;

  dragger_transform =
    compose(dragger_translation, dragger_rotation, dragger_scale, dragger_so);

  dragger.setMatrix(dragger_transform);
}


static void updateDraggerMatrix(osg::MatrixTransform &transform)
{
  // * parent               * parent
  //   * group                * transform
  //     * transform   ->
  //     * dragger
  osg::Group &group = parentOf(transform);
  assert(group.getNumChildren()==2);
  NodePtr dragger_node_ptr = group.getChild(1);
  using Dragger = osgManipulator::Dragger;
  auto dragger_ptr = dynamic_cast<Dragger *>(dragger_node_ptr.get());
  assert(dragger_ptr);
  NodePtr dragged_node_ptr = group.getChild(0);

  auto dragged_transform_node_ptr =
    dynamic_cast<osg::MatrixTransform *>(dragged_node_ptr.get());

  assert(dragged_transform_node_ptr);

  osg::MatrixTransform &dragged_transform_node = *dragged_transform_node_ptr;
  osg::MatrixTransform &dragger = *dragger_ptr;

  matchPose(dragger, dragged_transform_node);
}


static void updateDraggerMatrix(osg::Geode &dragged_geode)
{
  osg::MatrixTransform &transform = bodyTransformOf(dragged_geode);
  updateDraggerMatrix(transform);
}


void OSGScene::SelectionHandler::updateDraggerPosition()
{
  if (_translate_dragger_geode_ptr) {
    updateDraggerMatrix(*_translate_dragger_geode_ptr);
  }
  else if (_translate_dragger_transform_ptr) {
    updateDraggerMatrix(*_translate_dragger_transform_ptr);
  }
  else if (_rotate_dragger_geode_ptr) {
    updateDraggerMatrix(*_rotate_dragger_geode_ptr);
  }
  else if (_rotate_dragger_transform_ptr) {
    updateDraggerMatrix(*_rotate_dragger_transform_ptr);
  }
  else if (_scale_dragger_geode_ptr) {
    assert(_scale_dragger_ptr);

    _scale_dragger_ptr->setMatrix(
      draggerMatrix(
        geometryTransformOf(*_scale_dragger_geode_ptr),
        DraggerType::scale,
        shapeSize(*_scale_dragger_geode_ptr)
      )
    );
  }
  else {
    cerr << "updateDraggerPosition: no dragger\n";
  }
}


void OSGScene::selectGeometry(GeometryHandle handle)
{
  osg::Geode &geode = Impl::geodeForHandle(*this,handle);
  selectionHandler().changeSelectedGeodeTo(&geode);
}


void OSGScene::selectTransform(TransformHandle handle)
{
  osg::MatrixTransform &transform = Impl::transformForHandle(*this,handle);
  selectionHandler().changeSelectedTransformTo(&transform);
}


static osg::Vec3f osgVec(const OSGScene::Point &v)
{
  return {v.x,v.y,v.z};
}


static Vec3 vec(const osg::Vec3f &v)
{
  return { v.x(), v.y(), v.z() };
}


bool
OSGScene::Impl::isTransformOfSelection(TransformHandle handle, OSGScene &scene)
{
  Optional<GeometryHandle> maybe_selected_geometry =
    scene.selectedGeometry();

  Optional<TransformHandle> maybe_selected_transform =
    scene.selectedTransform();

  if (maybe_selected_transform == handle) {
    return true;
  }
  else if (maybe_selected_geometry) {
    if (scene.parentTransform(*maybe_selected_geometry) == handle) {
      return true;
    }
  }

  return false;
}


void OSGScene::setTranslation(TransformHandle handle, Point p)
{
  osg::MatrixTransform &transform =
    Impl::transformForHandle(*this, handle);

  ::setTranslation(transform, p);

  if (Impl::isTransformOfSelection(handle, *this)) {
    selectionHandler().updateDraggerPosition();
  }
}


void
 OSGScene::setCoordinateAxes(
   TransformHandle handle,
   const CoordinateAxes &axes
 )
{
  osg::Vec3f x = osgVec(axes.x);
  osg::Vec3f y = osgVec(axes.y);
  osg::Vec3f z = osgVec(axes.z);
  ::setCoordinateAxes(Impl::transformForHandle(*this, handle),x,y,z);

  if (Impl::isTransformOfSelection(handle, *this)) {
    selectionHandler().updateDraggerPosition();
  }
}


CoordinateAxes OSGScene::coordinateAxes(TransformHandle t) const
{
  OSGCoordinateAxes osg_ca =
    ::coordinateAxes(Impl::transformForHandle(*this,t));

  Vec3 x = vec(osg_ca.x);
  Vec3 y = vec(osg_ca.y);
  Vec3 z = vec(osg_ca.z);
  return {x,y,z};
}
