#include "osgscenemanager.hpp"

#include <osgViewer/ViewerEventHandlers>
#include "osgpickhandler.hpp"


using ViewPtr = osg::ref_ptr<osgViewer::View>;


struct OSGSceneManager::Impl {
  static ViewPtr
    setupViewInGraphicsWindow(
      GraphicsWindowPtr graphics_window_ptr,
      ViewType view_type,
      OSGSceneManager &scene_manager
    );
};


OSGSceneManager::OSGSceneManager()
{
  // disable the default setting of viewer.done() by pressing Escape.
  composite_viewer.setKeyEventSetsDone(0);
  composite_viewer.setThreadingModel(composite_viewer.SingleThreaded);

  // This causes paintEvent to be called regularly.
  timer.interval_in_milliseconds = 10;
  timer.callback = [this]{ composite_viewer.frame(); };
  timer.start();
}


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
  OSGSceneManager::Impl::setupViewInGraphicsWindow(
    GraphicsWindowPtr graphics_window_ptr,
    ViewType view_type,
    OSGSceneManager &scene_manager
  )
{
  osg::ref_ptr<osgViewer::View> view(new osgViewer::View);
  setupCamera(view->getCamera(),graphics_window_ptr);
  view->addEventHandler(new osgViewer::StatsHandler);
  view->addEventHandler(new OSGPickHandler(view,&scene_manager.selection_handler));

  osg::ref_ptr<osgGA::CameraManipulator> manipulator_ptr =
    createCameraManipulator(view_type);

  view->setCameraManipulator(manipulator_ptr);
  scene_manager.composite_viewer.addView(view);
  return view;
}


GraphicsWindowPtr OSGSceneManager::createGraphicsWindow(ViewType view_type)
{
  GraphicsWindowPtr window_ptr = createGraphicsWindowWithoutView();
  Impl::setupViewInGraphicsWindow(window_ptr,view_type,*this);
  {
    osgViewer::GraphicsWindow::Views views;
    window_ptr->getViews(views);
    views.front()->setSceneData(&scene.topNode());
  }
  return window_ptr;
}
