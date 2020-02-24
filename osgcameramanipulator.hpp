#include <osgGA/TrackballManipulator>

using OSGCameraManipulatorBase = osgGA::TrackballManipulator;


struct OSGCameraManipulator : OSGCameraManipulatorBase
{
  using Base = OSGCameraManipulatorBase;
  bool disable_rotate;

  OSGCameraManipulator() : disable_rotate(false) { }

  virtual bool
    handle(
      const osgGA::GUIEventAdapter& ea,
      osgGA::GUIActionAdapter& us
    )
  {
    // Disable view manipulation unless we're holding down the ALT key.
    auto event_type = ea.getEventType();

    if (event_type == ea.PUSH || event_type == ea.DRAG) {
      if (disable_rotate && ea.getButtonMask() & ea.LEFT_MOUSE_BUTTON) {
        return false;
      }

      if (!(ea.getModKeyMask() & ea.MODKEY_ALT)) {
        return false;
      }
    }

    return Base::handle(ea,us);
  }
};
