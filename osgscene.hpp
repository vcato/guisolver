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

    TransformHandle createTransform(TransformHandle parent) override;
    GeometryHandle createBox(TransformHandle parent) override;
    GeometryHandle createLine(TransformHandle parent);
    GeometryHandle createSphere(TransformHandle parent);

    SphereAndTransformHandle
      createSphereAndTransform(TransformHandle parent) override;

    BoxAndTransformHandle
      createBoxAndTransform(TransformHandle parent) override;

    LineAndTransformHandle
      createLineAndTransform(TransformHandle parent) override;

    void destroyGeometry(GeometryHandle) override;
    void destroyTransform(TransformHandle) override;
    void setGeometryScale(GeometryHandle handle,const Vec3 &v) override;
    void setGeometryCenter(GeometryHandle handle,const Point &v) override;

    Vec3 geometryScale(GeometryHandle) const override;
    Point geometryCenter(GeometryHandle) const override;
    void setTranslation(TransformHandle,Point) override;
    Point translation(TransformHandle) const override;
    void setCoordinateAxes(TransformHandle,const CoordinateAxes &) override;
    CoordinateAxes coordinateAxes(TransformHandle) const override;

    void
      setGeometryColor(
        GeometryAndTransformHandle handle,float r,float g,float b
      ) override;

    void setStartPoint(LineAndTransformHandle,Point) override;
    void setEndPoint(LineAndTransformHandle,Point) override;
    Optional<GeometryAndTransformHandle> selectedObject() const override;
    void selectGeometry(GeometryHandle handle);
    void selectTransform(TransformHandle handle);

    Optional<LineAndTransformHandle>
      maybeLineAndTransform(GeometryAndTransformHandle handle) const override;

    void attachDraggerToSelectedNode(DraggerType) override;

    GraphicsWindowPtr createGraphicsWindow(ViewType view_type);

  private:
    struct Impl;

    class SelectionHandler;

    struct HandleData {
      osg::MatrixTransform *transform_ptr = nullptr;
      osg::MatrixTransform *geometry_transform_ptr = nullptr;

      bool operator==(const HandleData &arg) const
      {
        return
          transform_ptr == arg.transform_ptr &&
          geometry_transform_ptr == arg.geometry_transform_ptr;
      }
    };

    vector<HandleData> _handle_datas;

    const MatrixTransformPtr _top_node_ptr;
    const GeometryAndTransformHandle _top_handle;
    osgViewer::CompositeViewer _composite_viewer;
    QtTimer _timer;
    std::unique_ptr<SelectionHandler> _selection_handler_ptr;

    SelectionHandler &selectionHandler();
    const SelectionHandler &selectionHandler() const;
};


#endif /* OSGSCENEMANAGER_HPP_ */
