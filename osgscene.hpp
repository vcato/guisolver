#ifndef OSGSCENEMANAGER_HPP_
#define OSGSCENEMANAGER_HPP_


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

    TransformHandle createTransform(TransformHandle parent) override;
    GeometryHandle createBox(TransformHandle parent) override;
    LineHandle createLine(TransformHandle parent) override;
    GeometryHandle createSphere(TransformHandle parent) override;
    MeshHandle createMesh(TransformHandle parent, const Mesh &) override;
    TransformHandle parentTransform(GeometryHandle) const override;
    void destroyGeometry(GeometryHandle) override;
    void destroyTransform(TransformHandle) override;
    void setGeometryScale(GeometryHandle handle,const Vec3 &v) override;
    void setGeometryCenter(GeometryHandle handle,const Point &v) override;
    void setGeometryColor(GeometryHandle handle,const Color &) override;

    Vec3 geometryScale(GeometryHandle) const override;
    Point geometryCenter(GeometryHandle) const override;
    void setTranslation(TransformHandle,Point) override;
    Point translation(TransformHandle) const override;
    void setCoordinateAxes(TransformHandle,const CoordinateAxes &) override;
    CoordinateAxes coordinateAxes(TransformHandle) const override;

    void setLineStartPoint(LineHandle, Point) override;
    void setLineEndPoint(LineHandle,Point) override;
    Optional<GeometryHandle> selectedGeometry() const override;
    Optional<TransformHandle> selectedTransform() const override;
    void selectGeometry(GeometryHandle handle);
    void selectTransform(TransformHandle handle);

    Optional<LineHandle> maybeLine(GeometryHandle handle) const override;
    void attachManipulatorToSelectedNode(ManipulatorType) override;
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
    const TransformHandle _top_transform;
    const GeometryHandle _top_geometry;
    osgViewer::CompositeViewer _composite_viewer;
    QtTimer _timer;
    std::unique_ptr<SelectionHandler> _selection_handler_ptr;

    SelectionHandler &selectionHandler();
    const SelectionHandler &selectionHandler() const;
};


#endif /* OSGSCENEMANAGER_HPP_ */
