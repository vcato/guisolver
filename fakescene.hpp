#include <map>
#include "scene.hpp"


struct FakeScene : Scene {
  struct Object;
  using TransformIndex = TransformHandle::Index;
  using Objects = std::map<TransformIndex, Object>;

  struct Object {
    TransformIndex parent_index;
    Optional<Point> maybe_geometry_center;
    Optional<Point> maybe_geometry_scale;
    Optional<Point> maybe_transform_translation;
    bool is_line = false;
  };

  TransformHandle top_handle = {0};
  Objects objects;
  Optional<size_t> maybe_selected_object_index;
  Optional<size_t> maybe_dragger_index;
#if !CHANGE_MANIPULATORS
  Optional<ManipulatorType> maybe_dragger_type;
#endif

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

  TransformHandle top() const override
  {
    return top_handle;
  }

  GeometryHandle createBox(TransformHandle parent) override;
  LineHandle createLine(TransformHandle parent) override;
  GeometryHandle createSphere(TransformHandle parent) override;
  MeshHandle createMesh(TransformHandle parent, const Mesh &) override;
  TransformHandle createTransform(TransformHandle parent) override;
  TransformHandle parentTransform(GeometryHandle) const override;
  void destroyGeometry(GeometryHandle) override;
  void destroyTransform(TransformHandle) override;
  void setGeometryScale(GeometryHandle,const Vec3 &) override;

  void
  setGeometryCenter(
    GeometryHandle geometry_handle,const Point &center
  ) override;

  Vec3 geometryScale(GeometryHandle) const override;
  Point geometryCenter(GeometryHandle) const override;

  void setCoordinateAxes(TransformHandle,const CoordinateAxes &) override
  {
  }

  CoordinateAxes coordinateAxes(TransformHandle) const override;

  void setTranslation(TransformHandle,Point) override;
  Point translation(TransformHandle) const override;

  void setGeometryColor(GeometryHandle,const Color &) override
  {
  }

  void setMesh(MeshHandle, Mesh) override
  {
    assert(false); // not implemented
  }

  void setLineStartPoint(LineHandle, Point) override
  {
  }

  void setLineEndPoint(LineHandle, Point) override
  {
  }

  template <typename Objects>
  static auto &elementOf(Objects &objects, size_t index)
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

    return GeometryHandle{*maybe_selected_object_index};
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

  Optional<LineHandle> maybeLine(GeometryHandle) const override;

  TransformHandle parentTransform(TransformHandle parent_handle) const;

#if !CHANGE_MANIPULATORS
  void attachManipulatorToSelectedNode(ManipulatorType dragger_type) override;
#else
  TransformHandle
  createTranslateManipulator(TransformHandle parent) override;

  GeometryHandle
  createScaleManipulator(TransformHandle parent) override;
#endif

  private:
    GeometryHandle createGeometry(TransformHandle parent);
    int nChildren(size_t handle_index) const;
};
