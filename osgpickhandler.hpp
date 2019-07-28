#include <osgGA/GUIEventHandler>
#include <osgViewer/View>
#include "osgutil.hpp"
#include "osgselectionhandler.hpp"


// A pick handler is created for each view.  It handles mouse events and
// detects clicks on objects.  When a click is detected, it notifies the
// selection handler.
struct OSGPickHandler : osgGA::GUIEventHandler {
  OSGPickHandler(
    osgViewer::View *view_ptr,
    OSGSelectionHandler *selection_handler_ptr
  );

  bool
    handle(
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter&
    );

  osgViewer::View *view_ptr;
  OSGSelectionHandler *selection_handler_ptr;
    // not holding a reference to avoid circular references
};
