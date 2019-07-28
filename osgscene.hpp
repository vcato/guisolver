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

using GraphicsWindowPtr = osg::ref_ptr<osgQt::GraphicsWindowQt>;


class OSGScene : public Scene {
  public:
    OSGScene();
    TransformHandle top() const override;

    using Scene::createSphere;
    TransformHandle createSphere(TransformHandle parent) override;
    TransformHandle createBox(TransformHandle parent) override;
    LineHandle createLine(TransformHandle parent) override;
    void setScale(TransformHandle handle,float x,float y,float z) override;
    void setTranslation(TransformHandle handle,Point) override;
    void setColor(TransformHandle handle,float r,float g,float b) override;
    void setStartPoint(LineHandle,Point) override;
    void setEndPoint(LineHandle,Point) override;
    Point worldPoint(Point p,TransformHandle t) const override;

    GraphicsWindowPtr createGraphicsWindow(ViewType view_type);

  private:
    struct Impl;

    struct SelectionHandler : OSGSelectionHandler {
      osg::Node *selected_node_ptr = nullptr;
      osg::Vec4 old_color;
      bool use_screen_relative_dragger = false;
      void nodeSelected(osg::Node *new_selected_node_ptr) override;
    };

    std::vector<osg::MatrixTransform *> transform_ptrs;
    const MatrixTransformPtr top_node_ptr;
    const TransformHandle top_handle;
    osgViewer::CompositeViewer composite_viewer;
    QtTimer timer;
    SelectionHandler selection_handler;
};


#endif /* OSGSCENEMANAGER_HPP_ */
