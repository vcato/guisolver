#ifndef OSGSCENEMANAGER_HPP_
#define OSGSCENEMANAGER_HPP_


#include <cassert>
#include <memory>
#include <osg/MatrixTransform>
#include <osgViewer/CompositeViewer>
#include "osgutil.hpp"
#include "osgQtGraphicsWindowQt.hpp"
#include "qttimer.hpp"
#include "scene.hpp"
#include "vector.hpp"

using GraphicsWindowPtr = osg::ref_ptr<osgQt::GraphicsWindowQt>;


class OSGScene : public Scene {
  public:
    OSGScene();
    ~OSGScene();
    TransformHandle top() const override;

    using Scene::createSphere;
    TransformHandle createSphere(TransformHandle parent) override;
    TransformHandle createBox(TransformHandle parent) override;
    LineHandle createLine(TransformHandle parent) override;
    void destroyLine(LineHandle) override;
    void destroyObject(TransformHandle) override;

    void
      setGeometryScale(TransformHandle handle,float x,float y,float z) override;

    Vec3 geometryScale(TransformHandle) const override;
    void setTranslation(TransformHandle,Point) override;
    Point translation(TransformHandle) const override;
    void setCoordinateAxes(TransformHandle,const CoordinateAxes &) override;
    CoordinateAxes coordinateAxes(TransformHandle) const override;
    void setColor(TransformHandle handle,float r,float g,float b) override;
    void setStartPoint(LineHandle,Point) override;
    void setEndPoint(LineHandle,Point) override;
    Point worldPoint(Point p,TransformHandle t) const override;
    Optional<TransformHandle> selectedObject() const override;
    void selectObject(TransformHandle handle) override;
    Optional<LineHandle> maybeLine(TransformHandle handle) const override;
    void attachDraggerToSelectedNode(DraggerType) override;

    GraphicsWindowPtr createGraphicsWindow(ViewType view_type);

  private:
    struct Impl;

    class SelectionHandler;

    vector<osg::MatrixTransform *> _transform_ptrs;
      // These are the geometry transforms for each handle.

    const MatrixTransformPtr _top_node_ptr;
    const TransformHandle _top_handle;
    osgViewer::CompositeViewer _composite_viewer;
    QtTimer _timer;
    std::unique_ptr<SelectionHandler> _selection_handler_ptr;

    SelectionHandler &selectionHandler();
    const SelectionHandler &selectionHandler() const;
};


#endif /* OSGSCENEMANAGER_HPP_ */
