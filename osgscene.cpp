#include "osgscene.hpp"

#include <cassert>
#include <iostream>
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
#include "osgutil.hpp"
#include "osgpickhandler.hpp"
#include "contains.hpp"
#include "matchconst.hpp"
#include "ptrvector.hpp"

namespace {
struct ScaleDragger;
struct TranslateDragger;
struct RotateDragger;
}


using std::cerr;
using std::string;
using DraggerType = Scene::DraggerType;
using GeodePtr = osg::ref_ptr<osg::Geode>;
using ViewPtr = osg::ref_ptr<osgViewer::View>;
using GroupPtr = osg::ref_ptr<osg::Group>;
using AutoTransformPtr = osg::ref_ptr<osg::AutoTransform>;
using DraggerPtr = osg::ref_ptr<osgManipulator::Dragger>;
using TransformHandle = Scene::TransformHandle;
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


class OSGScene::SelectionHandler : public OSGSelectionHandler {
  public:
    bool use_screen_relative_dragger = false;
    OSGScene &scene;

    SelectionHandler(OSGScene &);
    void updateDraggerPosition();
    void selectNodeWithoutDragger(osg::Geode *);
    void attachDragger(DraggerType);
    osg::Geode *selectedGeodePtr() const { return _selected_geode_ptr; }

  private:
    osg::Geode *_selected_geode_ptr = nullptr;
    osg::Geode *_translate_dragger_geode_ptr = nullptr;
    osg::Geode *_rotate_dragger_geode_ptr = nullptr;
    osg::Geode *_scale_dragger_geode_ptr = nullptr;
    osgManipulator::Dragger *_scale_dragger_ptr = nullptr;

    osg::Vec3 _old_color;

    void nodeClicked(osg::Node *) override;
    void removeExistingDraggers();
    void attachDragger(osg::Geode &, DraggerType);
    void attachTranslateDraggerTo(osg::Geode &);
    void attachRotateDraggerTo(osg::Geode &);
    void attachScaleDraggerTo(osg::Geode &);
    void changeSelectedNodeTo(osg::Geode *);
};


OSGScene::SelectionHandler &OSGScene::selectionHandler()
{
  return *_selection_handler_ptr;
}


const OSGScene::SelectionHandler &OSGScene::selectionHandler() const
{
  return *_selection_handler_ptr;
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

  static void
    destroyGeometryTransform(
      OSGScene &,
      osg::MatrixTransform &geometry_transform
    );

  static ViewPtr
    setupViewInGraphicsWindow(
      GraphicsWindowPtr graphics_window_ptr,
      ViewType view_type,
      OSGScene &
    );

  static size_t getNewHandleIndex(OSGScene &scene,osg::MatrixTransform &);

  static osg::MatrixTransform &
    geometryTransformForHandle(OSGScene &,const TransformHandle &);

  static const osg::MatrixTransform &
    geometryTransformForHandle(const OSGScene &,const TransformHandle &);

  static TransformHandle makeHandle(OSGScene &,osg::MatrixTransform &);
  static LineDrawable& lineDrawable(OSGScene &, LineHandle);
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
    return parentTransform(geometryTransformForHandle(scene,t));
  }

  static osg::MatrixTransform &
    transform(OSGScene &scene,TransformHandle t)
  {
    return parentTransform(geometryTransformForHandle(scene,t));
  }

  static Optional<TransformHandle> selectedTransform(const OSGScene &scene);

  static Optional<size_t>
    findTransformIndex(const OSGScene &,const osg::Geode &);

  template <typename Scene>
  static MatchConst_t<osg::Geode,Scene> &
  geodeForHandle(Scene &scene, TransformHandle handle)
  {
    auto *geode_ptr =
      geometryTransformForHandle(scene, handle).getChild(0)->asGeode();

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
  DraggerType dragger_type
)
{
  osg::Matrix matrix = transform_updating.getMatrix();

  if (dragger_type == DraggerType::scale) {
    osg::Vec3d translation;
    osg::Quat rotation;
    osg::Vec3d scale;
    osg::Quat scale_orient;
    matrix.decompose(translation,rotation,scale,scale_orient);

    scale.x() += 0.1;
    scale.y() += 0.1;
    scale.z() += 0.1;

    matrix = compose(translation,rotation,scale,scale_orient);
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
    osgManipulator::DraggerCallback &dragger_callback
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
        TranslateDraggerPtr translate_dragger_ptr = new TranslateDragger();
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
        RotateDraggerPtr rotate_dragger_ptr = new RotateDragger();

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
    dragger.setMatrix(draggerMatrix(transform_updating, dragger_type));
    dragger.addDraggerCallback(&dragger_callback);
  }

  return dragger_ptr;
}


static DraggerPtr
  createDragger(
    osg::MatrixTransform& transform,
    bool screen_relative,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback
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
            transform, DraggerType::translate, dragger_callback
          );
      }
    case DraggerType::rotate:
      if (screen_relative) {
        assert(false); // not implemented
      }
      else {
        return
          createObjectRelativeDragger(
            transform, DraggerType::rotate, dragger_callback
          );
      }
    case DraggerType::scale:
      if (screen_relative) {
        assert(false);
      }
      else {
        return
          createObjectRelativeDragger(
            transform, DraggerType::scale, dragger_callback
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
    osgManipulator::DraggerCallback &dragger_callback
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
      body_transform, screen_relative, dragger_type, dragger_callback
    );

  group_ptr->addChild(&body_transform);
  group_ptr->addChild(dragger_ptr);
}


OSGScene::SelectionHandler::SelectionHandler(OSGScene &scene_arg)
: scene(scene_arg)
{
}


Optional<size_t>
  OSGScene::Impl::findTransformIndex(
    const OSGScene &scene,
    const osg::Geode &const_node
  )
{
  auto &node = const_cast<osg::Geode &>(const_node);
  osg::MatrixTransform &transform = geometryTransformOf(node);
  size_t index = 0;

  for (osg::MatrixTransform *transform_ptr : scene._transform_ptrs) {
    if (transform_ptr == &transform) {
      return index;
    }

    ++index;
  }

  cerr << "Didn't find transform for selection\n";
  return {};
}


Optional<TransformHandle>
  OSGScene::Impl::selectedTransform(const OSGScene &scene)
{
  osg::Geode *geode_ptr = scene.selectionHandler().selectedGeodePtr();

  if (!geode_ptr) {
    return {};
  }

  Optional<size_t> maybe_index = findTransformIndex(scene, *geode_ptr);

  if (!maybe_index) {
    cerr << "Didn't find transform for selection\n";
    return {};
  }

  return TransformHandle{*maybe_index};
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


static osg::Vec3f geodeColor(osg::Geode &geode)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);
  osg::Drawable &drawable = *drawable_ptr;

  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(&drawable);

  if (shape_drawable_ptr) {
    return vec3(shape_drawable_ptr->getColor());
  }

  LineDrawable *geometry_ptr =
    dynamic_cast<LineDrawable*>(&drawable);

  if (geometry_ptr) {
    return geometry_ptr->color;
  }

  cerr << drawable.className() << "\n";

  assert(false);
  return {0,0,0};
}


static void setGeodeColor(osg::Geode &geode,const osg::Vec3f &vec)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);
  assert(drawable_ptr);

  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);

  if (shape_drawable_ptr) {
    shape_drawable_ptr->setColor(vec4(vec));
    return;
  }

  LineDrawable *geometry_ptr =
    dynamic_cast<LineDrawable*>(drawable_ptr);

  if (geometry_ptr) {
    geometry_ptr->color = vec;
    geometry_ptr->setup();
  }
}


void
  OSGScene::SelectionHandler::changeSelectedNodeTo(
    osg::Geode *new_selected_node_ptr
  )
{
  if (_selected_geode_ptr) {
    osg::Geode *geode_ptr = _selected_geode_ptr->asGeode();

    if (geode_ptr) {
      setGeodeColor(*geode_ptr, _old_color);
    }
  }

  _selected_geode_ptr = new_selected_node_ptr;

  if (_selected_geode_ptr) {
    osg::Geode *geode_ptr = _selected_geode_ptr->asGeode();

    if (geode_ptr) {
      _old_color = geodeColor(*geode_ptr);
      setGeodeColor(*geode_ptr, selectionColor());
    }
  }
}


void OSGScene::SelectionHandler::removeExistingDraggers()
{
  if (_translate_dragger_geode_ptr) {
    osg::MatrixTransform &body_transform =
      bodyTransformOf(*_translate_dragger_geode_ptr);

    replaceDraggerGroupWithTransform(body_transform);
    _translate_dragger_geode_ptr = 0;
  }

  if (_rotate_dragger_geode_ptr) {
    osg::MatrixTransform &body_transform =
      bodyTransformOf(*_rotate_dragger_geode_ptr);

    replaceDraggerGroupWithTransform(body_transform);
    _rotate_dragger_geode_ptr = 0;
  }

  if (_scale_dragger_geode_ptr) {
    osg::MatrixTransform &geometry_transform =
      geometryTransformOf(*_scale_dragger_geode_ptr);

    osg::MatrixTransform &transform = parentTransform(geometry_transform);

    transform.removeChild(_scale_dragger_ptr);
    _scale_dragger_geode_ptr = 0;
    _scale_dragger_ptr = 0;
  }
}


void
OSGScene::SelectionHandler::attachTranslateDraggerTo(
  osg::Geode &new_selected_node
)
{
  assert(!_translate_dragger_geode_ptr);
  _translate_dragger_geode_ptr = &new_selected_node;

  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  replaceTransformWithDraggerGroup(
    bodyTransformOf(*_translate_dragger_geode_ptr),
    use_screen_relative_dragger,
    DraggerType::translate,
    *dc
  );
}


void
OSGScene::SelectionHandler::attachRotateDraggerTo(
  osg::Geode &new_selected_node
)
{
  assert(new_selected_node.asGeode());
  assert(!_rotate_dragger_geode_ptr);
  _rotate_dragger_geode_ptr = &new_selected_node;

  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  replaceTransformWithDraggerGroup(
    bodyTransformOf(*_rotate_dragger_geode_ptr),
    use_screen_relative_dragger,
    DraggerType::rotate,
    *dc
  );
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
      *dc
    );

  parentTransform(geometry_transform).addChild(dragger_ptr);
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


static bool nodeIsLine(const osg::Geode &geode)
{
  const osg::Drawable *drawable_ptr = geode.getDrawable(0);

  const LineDrawable *line_drawable_ptr =
    dynamic_cast<const LineDrawable*>(drawable_ptr);

  if (!line_drawable_ptr) return false;
  return true;
}


static bool nodeIsShape(osg::Geode &geode)
{
  osg::Drawable *drawable_ptr = geode.getDrawable(0);

  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);

  if (!shape_drawable_ptr) return false;
  return true;
}


void OSGScene::SelectionHandler::nodeClicked(osg::Node *new_selected_node_ptr)
{
  osg::Geode *new_selected_geode_ptr = nullptr;

  if (new_selected_node_ptr) {
    osg::Geode *geode_ptr = new_selected_node_ptr->asGeode();

    assert(geode_ptr);

    if (nodeIsLine(*geode_ptr)) {
      new_selected_geode_ptr = geode_ptr;
    }
    else if (nodeIsShape(*geode_ptr)) {
      new_selected_geode_ptr = geode_ptr;
    }
  }

  selectNodeWithoutDragger(new_selected_geode_ptr);

  if (scene.selection_changed_callback) {
    scene.selection_changed_callback();
  }
}


void OSGScene::attachDraggerToSelectedNode(DraggerType dragger_type)
{
  selectionHandler().attachDragger(dragger_type);
}


Optional<LineHandle> OSGScene::maybeLine(TransformHandle handle) const
{
  if (nodeIsLine(Impl::geodeForHandle(*this, handle))) {
    return LineHandle(handle.index);
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


static osg::MatrixTransform &addTransformToGroup(osg::Group &parent)
{
  MatrixTransformPtr transform_ptr = createMatrixTransform();
  osg::MatrixTransform &matrix_transform = *transform_ptr;
  parent.addChild(transform_ptr);
  return matrix_transform;
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
  setTranslation(osg::MatrixTransform &transform,const Vec3 &v)
{
  osg::Matrix m = transform.getMatrix();
  m.setTrans(v.x, v.y, v.z);
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
: _top_node_ptr(createMatrixTransform()),
  _top_handle(Impl::makeHandle(*this,addTransformToGroup(*_top_node_ptr))),
  _selection_handler_ptr(new SelectionHandler(*this))
{
  osg::MatrixTransform &node = *_top_node_ptr;

  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, _top_handle);

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


auto
  OSGScene::Impl::createGeometryTransform(
    OSGScene &scene,
    TransformHandle parent,
    const ShapeParams &shape_params
  ) -> osg::MatrixTransform &
{
  // * parent group
  //   * parent geometry transform
  //   * new transform/group
  //     * new geometry transform
  osg::MatrixTransform &parent_geometry_transform =
    Impl::geometryTransformForHandle(scene, parent);

  osg::Group &parent_group = parentOf(parent_geometry_transform);
  osg::MatrixTransform &matrix_transform = addTransformToGroup(parent_group);
  osg::Group &new_group = matrix_transform;
  osg::MatrixTransform &geometry_transform = addTransformToGroup(new_group);
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
  if (scene.selectionHandler().selectedGeodePtr()) {
    osg::Group &selected_geometry_transform =
      parentOf(*scene.selectionHandler().selectedGeodePtr());

    if (&geometry_transform == &selected_geometry_transform) {
      scene.selectionHandler().selectNodeWithoutDragger(nullptr);
    }
  }

  osg::Group &matrix_transform = parentOf(geometry_transform);
  osg::Group &parent_group = parentOf(matrix_transform);
  removeTransformFromGroup(parent_group, matrix_transform);
}


auto
  OSGScene::Impl::create(
    OSGScene &scene,
    TransformHandle parent,
    const ShapeParams &shape_params
  ) -> TransformHandle
{
  osg::MatrixTransform &geometry_transform =
    createGeometryTransform(scene, parent, shape_params);

  return makeHandle(scene, geometry_transform);
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


void OSGScene::destroyLine(LineHandle handle)
{
  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, handle);

  Impl::destroyGeometryTransform(*this, geometry_transform);
  _transform_ptrs[handle.index] = 0;
}


void OSGScene::destroyObject(TransformHandle handle)
{
  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, handle);

  Impl::destroyGeometryTransform(*this, geometry_transform);
  _transform_ptrs[handle.index] = 0;
}


osg::MatrixTransform&
  OSGScene::Impl::geometryTransformForHandle(
    OSGScene &scene,
    const TransformHandle &handle
  )
{
  assert(scene._transform_ptrs[handle.index]);
  return *scene._transform_ptrs[handle.index];
}


const osg::MatrixTransform&
  OSGScene::Impl::geometryTransformForHandle(
    const OSGScene &scene,
    const TransformHandle &handle
  )
{
  assert(scene._transform_ptrs[handle.index]);
  return *scene._transform_ptrs[handle.index];
}


LineDrawable& OSGScene::Impl::lineDrawable(OSGScene &scene,LineHandle handle)
{
  osg::MatrixTransform &transform =
    Impl::geometryTransformForHandle(scene,handle);

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


void OSGScene::setGeometryScale(TransformHandle handle,const Vec3 &v)
{
  float x = v.x;
  float y = v.y;
  float z = v.z;

  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, handle);

  ::setScale(geometry_transform, x, y, z);

  if (Impl::selectedTransform(*this) == handle) {
    selectionHandler().updateDraggerPosition();
  }
}


void OSGScene::setGeometryCenter(TransformHandle handle,const Point &v)
{
  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransformForHandle(*this, handle);

  ::setTranslation(geometry_transform, {v.x(), v.y(), v.z()});

  if (Impl::selectedTransform(*this) == handle) {
    selectionHandler().updateDraggerPosition();
  }
}


Vec3 OSGScene::geometryScale(TransformHandle handle) const
{
  return ::scale(Impl::geometryTransformForHandle(*this, handle));
}


Point OSGScene::geometryCenter(TransformHandle handle) const
{
  return ::translation(Impl::geometryTransformForHandle(*this, handle));
}


OSGScene::Point OSGScene::translation(TransformHandle handle) const
{
  return
    ::translation(
      parentTransform(Impl::geometryTransformForHandle(*this,handle))
    );
}


void OSGScene::setColor(TransformHandle handle,float r,float g,float b)
{
  ::setColor(Impl::geometryTransformForHandle(*this,handle),osg::Vec3f(r,g,b));
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


size_t
  OSGScene::Impl::getNewHandleIndex(
    OSGScene &scene,
    osg::MatrixTransform &transform
  )
{
  return storeIn(scene._transform_ptrs, transform);
}


auto
  OSGScene::Impl::makeHandle(
    OSGScene &scene,
    osg::MatrixTransform &geometry_transform
  ) -> TransformHandle
{
  return TransformHandle{getNewHandleIndex(scene,geometry_transform)};
}


auto
  OSGScene::Impl::makeLineHandle(
    OSGScene &scene,
    osg::MatrixTransform &transform
  ) -> LineHandle
{
  return LineHandle{getNewHandleIndex(scene,transform)};
}


auto OSGScene::top() const -> TransformHandle
{
  return _top_handle;
}


Optional<TransformHandle> OSGScene::selectedObject() const
{
  return Impl::selectedTransform(*this);
}


void
OSGScene::SelectionHandler::attachDragger(
  osg::Geode &dragger_node,
  DraggerType dragger_type
)
{
  switch (dragger_type) {
    case DraggerType::translate:
      attachTranslateDraggerTo(dragger_node);
      break;
    case DraggerType::rotate:
      attachRotateDraggerTo(dragger_node);
      break;
    case DraggerType::scale:
      attachScaleDraggerTo(dragger_node);
      break;
  }
}


void OSGScene::SelectionHandler::attachDragger(DraggerType dragger_type)
{
  assert(_selected_geode_ptr);
  removeExistingDraggers();
  attachDragger(*_selected_geode_ptr, dragger_type);
}


void OSGScene::SelectionHandler::selectNodeWithoutDragger(osg::Geode *geode_ptr)
{
  changeSelectedNodeTo(geode_ptr);
  removeExistingDraggers();
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


static void updateDraggerMatrix(osg::Geode &dragged_geode)
{
  osg::MatrixTransform &transform = bodyTransformOf(dragged_geode);

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


void OSGScene::SelectionHandler::updateDraggerPosition()
{
  if (_translate_dragger_geode_ptr) {
    updateDraggerMatrix(*_translate_dragger_geode_ptr);
  }
  else if (_rotate_dragger_geode_ptr) {
    updateDraggerMatrix(*_rotate_dragger_geode_ptr);
  }
  else if (_scale_dragger_geode_ptr) {
    assert(_scale_dragger_ptr);

    _scale_dragger_ptr->setMatrix(
      draggerMatrix(geometryTransformOf(*_scale_dragger_geode_ptr),
      DraggerType::scale)
    );
  }
  else {
    cerr << "updateDraggerPosition: no dragger\n";
  }
}


void OSGScene::selectObject(TransformHandle handle)
{
  osg::Geode *geode_ptr =
    Impl::geometryTransformForHandle(*this,handle).getChild(0)->asGeode();

  assert(geode_ptr);
  selectionHandler().selectNodeWithoutDragger(geode_ptr);
}


static osg::Vec3f osgVec(const OSGScene::Vector &v)
{
  return {v.x,v.y,v.z};
}


static Scene::Vector vec(const osg::Vec3f &v)
{
  return {v.x(),v.y(),v.z()};
}


void OSGScene::setTranslation(TransformHandle handle, Point p)
{
  osg::MatrixTransform &parent_transform =
    parentTransform(Impl::geometryTransformForHandle(*this,handle));

  ::setTranslation(parent_transform, {p.x(), p.y(), p.z()});

  if (Impl::selectedTransform(*this) == handle) {
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
  ::setCoordinateAxes(Impl::transform(*this, handle),x,y,z);

  if (Impl::selectedTransform(*this) == handle) {
    selectionHandler().updateDraggerPosition();
  }
}


CoordinateAxes OSGScene::coordinateAxes(TransformHandle t) const
{
  OSGCoordinateAxes osg_ca = ::coordinateAxes(Impl::transform(*this,t));
  Scene::Vector x = vec(osg_ca.x);
  Scene::Vector y = vec(osg_ca.y);
  Scene::Vector z = vec(osg_ca.z);
  return {x,y,z};
}
