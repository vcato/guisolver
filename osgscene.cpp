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

namespace {
struct ScaleDragger;
}


using std::cerr;
using std::string;
using DraggerType = Scene::DraggerType;
using GeodePtr = osg::ref_ptr<osg::Geode>;
using ViewPtr = osg::ref_ptr<osgViewer::View>;
using GroupPtr = osg::ref_ptr<osg::Group>;
using AutoTransformPtr = osg::ref_ptr<osg::AutoTransform>;
using TransformHandle = Scene::TransformHandle;
using LineHandle = Scene::LineHandle;

using TranslateAxisDraggerPtr =
  osg::ref_ptr<osgManipulator::TranslateAxisDragger>;

using TrackballDraggerPtr =
  osg::ref_ptr<osgManipulator::TrackballDragger>;

using ScaleDraggerPtr = osg::ref_ptr<ScaleDragger>;

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


namespace {
struct ScaleDragger : osgManipulator::TabBoxDragger {
  struct Constraint : osgManipulator::Constraint {
    using TranslateInLineCommand = osgManipulator::TranslateInLineCommand;
    using TranslateInPlaneCommand = osgManipulator::TranslateInPlaneCommand;

    bool constrain(osgManipulator::Scale1DCommand& command) const override
    {
      command.setScaleCenter(0);
      return true;
    }

    bool constrain(osgManipulator::Scale2DCommand& command) const override
    {
      command.setScaleCenter({0,0});
      return true;
    }

    bool constrain(TranslateInLineCommand& command) const override
    {
      command.setTranslation({0,0,0});
      return true;
    }

    bool constrain(TranslateInPlaneCommand& command) const override
    {
      command.setTranslation({0,0,0});
      return true;
    }
  };

  osg::ref_ptr<Constraint> constraint_ptr;

  ScaleDragger()
  : constraint_ptr(new Constraint)
  {
    addConstraint(constraint_ptr);
  }
};
}


class OSGScene::SelectionHandler : public OSGSelectionHandler {
  public:
    bool use_screen_relative_dragger = false;
    OSGScene &scene;

    SelectionHandler(OSGScene &);
    void updateDraggerPosition();
    void selectNodeWithoutDragger(osg::Node *);
    void attachDragger(DraggerType);
    osg::Node *selectedNodePtr() const { return _selected_node_ptr; }

  private:
    osg::Node *_selected_node_ptr = nullptr;

    // These are Geodes.
    osg::Node *_translate_dragger_node_ptr = nullptr;
    osg::Node *_rotate_dragger_node_ptr = nullptr;
    osg::Node *_scale_dragger_node_ptr = nullptr;

    osg::Vec3 _old_color;

    void nodeClicked(osg::Node *) override;
    void removeExistingDraggers();
    void attachDragger(osg::Node &, DraggerType);
    void attachTranslateDraggerTo(osg::Node &);
    void attachRotateDraggerTo(osg::Node &);
    void attachScaleDraggerTo(osg::Node &);
    void changeSelectedNodeTo(osg::Node *);
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
    geometryTransform(OSGScene &,const TransformHandle &);

  static const osg::MatrixTransform &
    geometryTransform(const OSGScene &,const TransformHandle &);

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
    return parentTransform(geometryTransform(scene,t));
  }

  static osg::MatrixTransform &
    transform(OSGScene &scene,TransformHandle t)
  {
    return parentTransform(geometryTransform(scene,t));
  }

  static Optional<TransformHandle> selectedTransform(const OSGScene &scene);

  static Optional<size_t>
    findTransformIndex(const OSGScene &,const osg::Node &);
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


static osg::MatrixTransform &geometryTransformOf(osg::Node *node_ptr)
{
  assert(node_ptr);
  osg::Node *parent_ptr = node_ptr->getParent(0);
  osg::Transform *transform_ptr = parent_ptr->asTransform();
  assert(transform_ptr);
  osg::MatrixTransform *geometry_transform_ptr =
    transform_ptr->asMatrixTransform();
  assert(geometry_transform_ptr);
  return *geometry_transform_ptr;
}


static osg::MatrixTransform &transformParentOf(osg::Node *node_ptr)
{
  return parentTransform(geometryTransformOf(node_ptr));
}


static osg::Group &parentOf(osg::Node &t)
{
  osg::Group *group_ptr = t.getParent(0);
  assert(group_ptr);
  return *group_ptr;
}


static void replaceDraggerGroupWithTransform(MatrixTransformPtr transform_ptr)
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
static NodePtr
  createScreenRelativeTranslateDragger(
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


static NodePtr
  createObjectRelativeDragger(
    MatrixTransformPtr transform_ptr,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback
  )
{
  osg::ref_ptr<osgManipulator::Dragger> dragger_ptr;
  osg::MatrixTransform *transform_updating_ptr = transform_ptr.get();
  osg::Matrix matrix = transform_ptr->getMatrix();

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

  using HandleCommandMask =
    osgManipulator::DraggerTransformCallback::HandleCommandMask;

  HandleCommandMask
    handle_command_mask = HandleCommandMask::HANDLE_ALL;

  switch (dragger_type) {
    case DraggerType::translate:
      {
        TranslateAxisDraggerPtr translate_dragger_ptr =
          new osgManipulator::TranslateAxisDragger();

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
        TrackballDraggerPtr rotate_dragger_ptr =
          new osgManipulator::TrackballDragger();

        rotate_dragger_ptr->setupDefaultGeometry();
        dragger_ptr = rotate_dragger_ptr;
      }
      break;
    case DraggerType::scale:
      {
        ScaleDraggerPtr scale_dragger_ptr = new ScaleDragger;
        scale_dragger_ptr->setupDefaultGeometry();
        //matrix = osg::Matrix::identity();

        handle_command_mask =
          HandleCommandMask(
            HandleCommandMask::HANDLE_SCALED_1D |
            HandleCommandMask::HANDLE_SCALED_2D
          );

        dragger_ptr = scale_dragger_ptr;
      }
      break;
  }

  assert(isDragger(dragger_ptr));

  {
    osgManipulator::Dragger &dragger = *dragger_ptr;
    dragger.setHandleEvents(true);

    if (transform_updating_ptr) {
      dragger.addTransformUpdating(
        transform_updating_ptr, handle_command_mask
      );
    }

    dragger.setMatrix(matrix);
    dragger.addDraggerCallback(&dragger_callback);
  }

  return dragger_ptr;
}


static NodePtr
  createDragger(
    MatrixTransformPtr transform_ptr,
    bool screen_relative,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback
  )
{
  switch (dragger_type) {
    case DraggerType::translate:
      if (screen_relative) {
        return
          createScreenRelativeTranslateDragger(transform_ptr,dragger_callback);
      }
      else {
        return
          createObjectRelativeDragger(
            transform_ptr, DraggerType::translate, dragger_callback
          );
      }
    case DraggerType::rotate:
      if (screen_relative) {
        assert(false); // not implemented
      }
      else {
        return
          createObjectRelativeDragger(
            transform_ptr, DraggerType::rotate, dragger_callback
          );
      }
    case DraggerType::scale:
      if (screen_relative) {
        assert(false);
      }
      else {
        return
          createObjectRelativeDragger(
            transform_ptr, DraggerType::scale, dragger_callback
          );
      }
  }

  assert(false); // shouldn't happen
  return nullptr;
}


static void
  replaceTransformWithDraggerGroup(
    MatrixTransformPtr transform_ptr,
    bool screen_relative,
    DraggerType dragger_type,
    osgManipulator::DraggerCallback &dragger_callback
  )
{
  // The hierarchy is changed like this:
  //
  // * parent             * parent
  //   * transform   ->     * group
  //                          * transform
  //                          * dragger

  osg::Group &parent = parentOf(*transform_ptr);
  parent.removeChild(transform_ptr);
  GroupPtr group_ptr = createGroup();
  parent.addChild(group_ptr);

  NodePtr dragger_ptr =
    createDragger(
      transform_ptr, screen_relative, dragger_type, dragger_callback
    );

  group_ptr->addChild(transform_ptr);
  group_ptr->addChild(dragger_ptr);
}


OSGScene::SelectionHandler::SelectionHandler(OSGScene &scene_arg)
: scene(scene_arg)
{
}


Optional<size_t>
  OSGScene::Impl::findTransformIndex(
    const OSGScene &scene,
    const osg::Node &const_node
  )
{
  auto &node = const_cast<osg::Node &>(const_node);
  osg::MatrixTransform &transform = geometryTransformOf(&node);
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
  osg::Node *node_ptr = scene.selectionHandler().selectedNodePtr();

  if (!node_ptr) {
    return {};
  }

  Optional<size_t> maybe_index = findTransformIndex(scene, *node_ptr);

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
    osg::Node *new_selected_node_ptr
  )
{
  if (_selected_node_ptr) {
    osg::Geode *geode_ptr = _selected_node_ptr->asGeode();

    if (geode_ptr) {
      setGeodeColor(*geode_ptr, _old_color);
    }
  }

  _selected_node_ptr = new_selected_node_ptr;

  if (_selected_node_ptr) {
    osg::Geode *geode_ptr = _selected_node_ptr->asGeode();

    if (geode_ptr) {
      _old_color = geodeColor(*geode_ptr);
      setGeodeColor(*geode_ptr, selectionColor());
    }
  }
}


void OSGScene::SelectionHandler::removeExistingDraggers()
{
  if (_translate_dragger_node_ptr) {
    osg::MatrixTransform &parent =
      transformParentOf(_translate_dragger_node_ptr);

    replaceDraggerGroupWithTransform(&parent);
    _translate_dragger_node_ptr = 0;
  }

  if (_rotate_dragger_node_ptr) {
    osg::MatrixTransform &parent =
      transformParentOf(_rotate_dragger_node_ptr);

    replaceDraggerGroupWithTransform(&parent);
    _rotate_dragger_node_ptr = 0;
  }

  if (_scale_dragger_node_ptr) {
    osg::MatrixTransform &geometry_transform =
      geometryTransformOf(_scale_dragger_node_ptr);

    osg::MatrixTransform &transform =
      parentTransform(geometry_transform);

    transform.removeChild(transform.getNumChildren()-1, 1);
      // The dragger is going to be the last child.

    _scale_dragger_node_ptr = 0;
  }
}


void
OSGScene::SelectionHandler::attachTranslateDraggerTo(
  osg::Node &new_selected_node
)
{
  assert(!_translate_dragger_node_ptr);
  _translate_dragger_node_ptr = &new_selected_node;

  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  replaceTransformWithDraggerGroup(
    &transformParentOf(_translate_dragger_node_ptr),
    use_screen_relative_dragger,
    DraggerType::translate,
    *dc
  );
}


void
OSGScene::SelectionHandler::attachRotateDraggerTo(
  osg::Node &new_selected_node
)
{
  assert(new_selected_node.asGeode());
  assert(!_rotate_dragger_node_ptr);
  _rotate_dragger_node_ptr = &new_selected_node;

  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  replaceTransformWithDraggerGroup(
    &transformParentOf(_rotate_dragger_node_ptr),
    use_screen_relative_dragger,
    DraggerType::rotate,
    *dc
  );
}


void
OSGScene::SelectionHandler::attachScaleDraggerTo(
  osg::Node &new_selected_node
)
{
  assert(new_selected_node.asGeode());
  assert(!_scale_dragger_node_ptr);
  _scale_dragger_node_ptr = &new_selected_node;

  osg::ref_ptr<osgManipulator::DraggerCallback> dc =
    new Impl::DraggerCallback(scene);

  osg::MatrixTransform &geometry_transform =
    geometryTransformOf(_scale_dragger_node_ptr);

  NodePtr dragger_ptr =
    createDragger(
      &geometry_transform,
      use_screen_relative_dragger,
      DraggerType::scale,
      *dc
    );

  parentTransform(geometry_transform).addChild(dragger_ptr);
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


static bool nodeIsLine(const osg::Node &node)
{
  const osg::Geode *geode_ptr = node.asGeode();
  if (!geode_ptr) return false;
  const osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);

  const LineDrawable *line_drawable_ptr =
    dynamic_cast<const LineDrawable*>(drawable_ptr);

  if (!line_drawable_ptr) return false;
  return true;
}


static bool nodeIsShape(osg::Node &node)
{
  osg::Geode *geode_ptr = node.asGeode();
  if (!geode_ptr) return false;
  osg::Drawable *drawable_ptr = geode_ptr->getDrawable(0);

  osg::ShapeDrawable *shape_drawable_ptr =
    dynamic_cast<osg::ShapeDrawable*>(drawable_ptr);

  if (!shape_drawable_ptr) return false;
  return true;
}


void OSGScene::SelectionHandler::nodeClicked(osg::Node *new_selected_node_ptr)
{
  if (new_selected_node_ptr) {
    if (nodeIsLine(*new_selected_node_ptr)) {
    }
    else if (nodeIsShape(*new_selected_node_ptr)) {
    }
    else {
      new_selected_node_ptr = nullptr;
    }
  }

  selectNodeWithoutDragger(new_selected_node_ptr);

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
  const osg::Node *node_ptr =
    Impl::geometryTransform(*this, handle).getChild(0);

  assert(node_ptr);

  if (nodeIsLine(*node_ptr)) {
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
    Impl::geometryTransform(*this, _top_handle);

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
    Impl::geometryTransform(scene, parent);

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
  if (scene.selectionHandler().selectedNodePtr()) {
    osg::Group &selected_geometry_transform =
      parentOf(*scene.selectionHandler().selectedNodePtr());

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
    Impl::geometryTransform(*this, handle);

  Impl::destroyGeometryTransform(*this, geometry_transform);
  _transform_ptrs[handle.index] = 0;
}


void OSGScene::destroyObject(TransformHandle handle)
{
  osg::MatrixTransform &geometry_transform =
    Impl::geometryTransform(*this, handle);

  Impl::destroyGeometryTransform(*this, geometry_transform);
  _transform_ptrs[handle.index] = 0;
}


osg::MatrixTransform&
  OSGScene::Impl::geometryTransform(
    OSGScene &scene,
    const TransformHandle &handle
  )
{
  assert(scene._transform_ptrs[handle.index]);
  return *scene._transform_ptrs[handle.index];
}


const osg::MatrixTransform&
  OSGScene::Impl::geometryTransform(
    const OSGScene &scene,
    const TransformHandle &handle
  )
{
  assert(scene._transform_ptrs[handle.index]);
  return *scene._transform_ptrs[handle.index];
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


Vec3 OSGScene::geometryScale(TransformHandle handle) const
{
  return ::scale(Impl::geometryTransform(*this, handle));
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


template <typename T>
static size_t findNull(const vector<T*> &v)
{
  return findIndex(v,nullptr);
}


size_t
  OSGScene::Impl::getNewHandleIndex(
    OSGScene &scene,
    osg::MatrixTransform &transform
  )
{
  assert(!contains(scene._transform_ptrs,&transform));
  size_t index = findNull(scene._transform_ptrs);

  if (index == scene._transform_ptrs.size()) {
    scene._transform_ptrs.push_back(&transform);
  }
  else {
    scene._transform_ptrs[index] = &transform;
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


auto OSGScene::worldPoint(Point p,TransformHandle t) const -> Point
{
  osg::Vec3f v1 =
    ::worldPoint(
      Impl::transform(*this,t),
      {p.x(),p.y(),p.z()}
    );

  osg::Vec3f v2 = ::localPoint(*_top_node_ptr,v1);
  return {v2.x(),v2.y(),v2.z()};
}


Optional<TransformHandle> OSGScene::selectedObject() const
{
  return Impl::selectedTransform(*this);
}


void
OSGScene::SelectionHandler::attachDragger(
  osg::Node &dragger_node,
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
  assert(_selected_node_ptr);
  removeExistingDraggers();
  attachDragger(*_selected_node_ptr, dragger_type);
}


void OSGScene::SelectionHandler::selectNodeWithoutDragger(osg::Node *node_ptr)
{
  changeSelectedNodeTo(node_ptr);
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


static void updateDraggerPose(osg::Node &dragger_transform_node)
{
  osg::MatrixTransform &transform =
    transformParentOf(&dragger_transform_node);

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
  if (_translate_dragger_node_ptr) {
    updateDraggerPose(*_translate_dragger_node_ptr);
  }
  else if (_rotate_dragger_node_ptr) {
    updateDraggerPose(*_rotate_dragger_node_ptr);
  }
  else if (_scale_dragger_node_ptr) {
    cerr << "updateDraggerPosition: scale dragger\n";
  }
  else {
    cerr << "updateDraggerPosition: no dragger\n";
  }
}


void OSGScene::selectObject(TransformHandle handle)
{
  osg::Node *node_ptr = Impl::geometryTransform(*this,handle).getChild(0);
  assert(node_ptr);
  selectionHandler().selectNodeWithoutDragger(node_ptr);
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
    parentTransform(Impl::geometryTransform(*this,handle));

  ::setTranslation(parent_transform, p.x(), p.y(), p.z());

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
