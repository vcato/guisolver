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

    using Scene::createSphereAndTransform;

    SphereAndTransformHandle
      createSphereAndTransform(TransformHandle parent) override;

    BoxAndTransformHandle
      createBoxAndTransform(TransformHandle parent) override;

    LineAndTransformHandle
      createLineAndTransform(TransformHandle parent) override;

    void destroyLineAndTransform(LineAndTransformHandle) override;
    void destroyObject(TransformHandle) override;
    void setGeometryScale(TransformHandle handle,const Vec3 &) override;
    void setGeometryCenter(TransformHandle handle,const Point &) override;
    Vec3 geometryScale(TransformHandle) const override;
    Point geometryCenter(TransformHandle) const override;
    void setTranslation(TransformHandle,Point) override;
    Point translation(TransformHandle) const override;
    void setCoordinateAxes(TransformHandle,const CoordinateAxes &) override;
    CoordinateAxes coordinateAxes(TransformHandle) const override;
    void setColor(TransformHandle handle,float r,float g,float b) override;
    void setStartPoint(LineAndTransformHandle,Point) override;
    void setEndPoint(LineAndTransformHandle,Point) override;
    Optional<TransformHandle> selectedObject() const override;
    void selectObject(TransformHandle handle) override;

    Optional<LineAndTransformHandle>
      maybeLineAndTransform(TransformHandle handle) const override;

    void attachDraggerToSelectedNode(DraggerType) override;

    GraphicsWindowPtr createGraphicsWindow(ViewType view_type);

  private:
    struct Impl;

    class SelectionHandler;

    struct HandleData {
      osg::MatrixTransform *transform_ptr = nullptr;
      osg::MatrixTransform *geometry_transform_ptr = nullptr;
    };

    vector<HandleData> _handle_datas;

    const MatrixTransformPtr _top_node_ptr;
    const TransformHandle _top_handle;
    osgViewer::CompositeViewer _composite_viewer;
    QtTimer _timer;
    std::unique_ptr<SelectionHandler> _selection_handler_ptr;

    SelectionHandler &selectionHandler();
    const SelectionHandler &selectionHandler() const;
};


#endif /* OSGSCENEMANAGER_HPP_ */
