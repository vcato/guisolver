#include <map>
#include "scene.hpp"


struct FakeScene : Scene {
  struct Object;
  using TransformIndex = TransformHandle::Index;
  using Objects = std::map<TransformIndex, Object>;

  struct Object {
    TransformIndex parent_index;
    Optional<Point> maybe_geometry_center;
  };

  TransformHandle top_handle = {0};
  Objects objects;
  Optional<size_t> maybe_selected_object_index;
  Optional<size_t> maybe_dragger_index;
  Optional<DraggerType> maybe_dragger_type;

  bool indexIsUsed(TransformIndex i) const
  {
    return objects.count(i) != 0;
  }

  TransformIndex firstUnusedIndex() const
  {
    TransformIndex i = 1;

    while (indexIsUsed(i)) {
      ++i;
    }

    return i;
  }

  virtual TransformHandle top() const
  {
    return top_handle;
  }

  GeometryHandle createBox(TransformHandle parent) override;
  LineHandle createLine(TransformHandle parent) override;
  GeometryHandle createSphere(TransformHandle parent) override;
  TransformHandle createTransform(TransformHandle parent) override;
  TransformHandle parentTransform(GeometryHandle) const override;
  void destroyGeometry(GeometryHandle) override;
  void destroyTransform(TransformHandle) override;

  void setGeometryScale(GeometryHandle,const Vec3 &) override
  {
  }

  void
  setGeometryCenter(
    GeometryHandle geometry_handle,const Point &center
  ) override
  {
    *objects[geometry_handle.index].maybe_geometry_center = center;
  }

  virtual Vec3 geometryScale(GeometryAndTransformHandle) const
  {
    assert(false); // not needed
  }

  virtual Vec3 geometryScale(GeometryHandle) const
  {
    assert(false); // not needed
  }

  virtual Point geometryCenter(GeometryAndTransformHandle) const
  {
    assert(false); // not needed
  }

  virtual Point geometryCenter(GeometryHandle) const
  {
    assert(false); // not needed
  }

  virtual void setCoordinateAxes(TransformHandle,const CoordinateAxes &)
  {
  }

  virtual CoordinateAxes coordinateAxes(TransformHandle) const
  {
    assert(false); // not needed
  }

  virtual void setTranslation(TransformHandle,Point)
  {
  }

  virtual Point translation(TransformHandle) const
  {
    assert(false); // not needed
  }

  void setGeometryColor(GeometryHandle,const Color &) override
  {
  }

  void setStartPoint(LineHandle,Point) override
  {
  }

  void setEndPoint(LineHandle,Point) override
  {
  }

  static const Object &
  elementOf(const std::map<size_t, Object> &objects, size_t index)
  {
    auto iter = objects.find(index);
    assert(iter != objects.end());
    return iter->second;
  }

  bool objectIsGeometry(size_t index) const
  {
    return elementOf(objects, index).maybe_geometry_center.hasValue();
  }

  Optional<GeometryHandle> selectedGeometry() const override
  {
    if (!maybe_selected_object_index) {
      // Nothing is selected
      assert(false); // not needed
    }

    if (!objectIsGeometry(*maybe_selected_object_index)) {
      // The selected object isn't geometry.
      return {};
    }

    assert(false); // not needed
  }

  Optional<TransformHandle> selectedTransform() const override
  {
    if (!maybe_selected_object_index) {
      // Nothing is selected
      assert(false); // not needed
    }

    if (objectIsGeometry(*maybe_selected_object_index)) {
      assert(false); // not needed
    }

    return TransformHandle{*maybe_selected_object_index};
  }

  void selectGeometry(GeometryHandle) override
  {
    assert(false); // not needed
  }

  void selectTransform(TransformHandle transform_handle) override
  {
    maybe_selected_object_index = transform_handle.index;
  }

  Optional<LineHandle> maybeLine(GeometryHandle) const override
  {
    assert(false); // not needed
  }

  virtual void attachDraggerToSelectedNode(DraggerType dragger_type)
  {
    maybe_dragger_index = maybe_selected_object_index;
    maybe_dragger_type = dragger_type;
  }

  private:
    GeometryHandle createGeometry(TransformHandle parent);

    GeometryAndTransformHandle
      createGeometryAndTransform(TransformHandle parent_handle);

    int nChildren(size_t handle_index) const;
};
