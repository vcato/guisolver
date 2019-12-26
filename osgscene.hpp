#ifndef OSGSCENEMANAGER_HPP_
#define OSGSCENEMANAGER_HPP_


#include <cassert>
#include <osg/MatrixTransform>
#include <osgViewer/CompositeViewer>
#include "osgutil.hpp"
#include "osgQtGraphicsWindowQt.hpp"
#include "osgselectionhandler.hpp"
#include "qttimer.hpp"
#include "scene.hpp"
#include "vector.hpp"

using GraphicsWindowPtr = osg::ref_ptr<osgQt::GraphicsWindowQt>;


class OSGScene : public Scene {
  public:
    OSGScene();
    TransformHandle top() const override;

    using Scene::createSphere;
    TransformHandle createSphere(TransformHandle parent) override;
    TransformHandle createBox(TransformHandle parent) override;
    LineHandle createLine(TransformHandle parent) override;
    void destroyLine(LineHandle) override;
    void destroyObject(TransformHandle) override;

    void
      setGeometryScale(TransformHandle handle,float x,float y,float z) override;

    void setTranslation(TransformHandle,Point) override;
    Point translation(TransformHandle) const override;
    void setCoordinateAxes(TransformHandle,const CoordinateAxes &) override;
    CoordinateAxes coordinateAxes(TransformHandle) const override;
    void setColor(TransformHandle handle,float r,float g,float b) override;
    void setStartPoint(LineHandle,Point) override;
    void setEndPoint(LineHandle,Point) override;
    Point worldPoint(Point p,TransformHandle t) const override;
    Optional<TransformHandle> selectedObject() const override;
    void selectObject(TransformHandle) override;

    GraphicsWindowPtr createGraphicsWindow(ViewType view_type);

  private:
    struct Impl;

    class SelectionHandler : public OSGSelectionHandler {
      public:
        bool use_screen_relative_dragger = false;
        OSGScene &scene;

        SelectionHandler(OSGScene &scene_arg);
        void setDraggerPosition(const Point &p);
        void selectNode(osg::Node *node_ptr);
        osg::Node *selectedNodePtr() const { return _selected_node_ptr; }

      private:
        osg::Node *_selected_node_ptr = nullptr;
        osg::Node *_translate_dragger_node_ptr = nullptr;
        osg::Node *_rotate_dragger_node_ptr = nullptr;
        osg::Vec3 _old_color;

        void nodeClicked(osg::Node *) override;
        void removeExistingDraggers();
        void attachTranslateDraggerTo(osg::Node *);
        void attachRotateDraggerTo(osg::Node *);
        void changeSelectedNodeTo(osg::Node *);
    };

    vector<osg::MatrixTransform *> transform_ptrs;
    const MatrixTransformPtr top_node_ptr;
    const TransformHandle top_handle;
    osgViewer::CompositeViewer composite_viewer;
    QtTimer timer;
    SelectionHandler selection_handler;
};


#endif /* OSGSCENEMANAGER_HPP_ */
