#include <osgViewer/CompositeViewer>
#include "osgutil.hpp"
#include "qttimer.hpp"
#include "osgQtGraphicsWindowQt.hpp"
#include "osgscene.hpp"


using GraphicsWindowPtr = osg::ref_ptr<osgQt::GraphicsWindowQt>;


class OSGSceneManager {
  public:
    OSGSceneManager();
    GraphicsWindowPtr createGraphicsWindow(ViewType view_type);
    OSGScene scene;
    OSGSelectionHandler selection_handler;

  private:
    struct Impl;

    osgViewer::CompositeViewer composite_viewer;
    QtTimer timer;
};
