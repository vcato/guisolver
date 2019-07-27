#include <osgGA/GUIEventHandler>
#include <osgViewer/View>
#include "osgutil.hpp"

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
