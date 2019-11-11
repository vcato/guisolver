#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"
#include "isequal.hpp"

struct TreePaths {
  struct Marker;
  struct DistanceError;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  TreePaths()
  : markers()
  {
  }

  struct XYZ {
    TreePath path;
    TreePath x;
    TreePath y;
    TreePath z;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&XYZ::path);
      f(&XYZ::x);
      f(&XYZ::y);
      f(&XYZ::z);
    }

    bool operator==(const XYZ &arg) const
    {
      return isEqual(*this, arg);
    }
  };

  struct Position : XYZ {
    Position() {}
    explicit Position(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Marker {
    TreePath path;
    Position position;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Marker::path);
      f(&Marker::position);
    }

    bool operator==(const Marker &arg) const
    {
      return isEqual(*this, arg);
    }
  };

  struct Translation : XYZ
  {
    Translation() {}
    explicit Translation(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Rotation : XYZ
  {
    Rotation() {}
    explicit Rotation(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Scale : XYZ
  {
    Scale() {}
    explicit Scale(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Box {
    struct Geometry {
      TreePath path;
      Scale scale;

      template <typename F>
      static void forEachMember(const F &f)
      {
        f(&Geometry::path);
        f(&Geometry::scale);
      }

      bool operator==(const Geometry &arg) const
      {
        return isEqual(*this, arg);
      }
    };

    TreePath path;
    Translation translation;
    Rotation rotation;
    Geometry geometry;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Box::path);
      f(&Box::translation);
      f(&Box::rotation);
      f(&Box::geometry);
    }

    bool operator==(const Box &arg) const
    {
      return isEqual(*this, arg);
    }
  };

  struct DistanceError {
    TreePath path;
    TreePath start;
    TreePath end;
    TreePath distance;
    TreePath desired_distance;
    TreePath weight;
    TreePath error;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&DistanceError::path);
      f(&DistanceError::start);
      f(&DistanceError::end);
      f(&DistanceError::distance);
      f(&DistanceError::desired_distance);
      f(&DistanceError::weight);
      f(&DistanceError::error);
    }

    bool operator==(const DistanceError &arg) const
    {
      return isEqual(*this, arg);
    }
  };

  TreePath path;
  Box box;
  Markers markers;
  DistanceErrors distance_errors;
  TreePath next_distance_error_path;
  TreePath next_box_marker_path;
  TreePath next_scene_marker_path;
  TreePath total_error;

  template <typename F>
  static void forEachMember(const F &f)
  {
    f(&TreePaths::path);
    f(&TreePaths::box);
    f(&TreePaths::markers);
    f(&TreePaths::distance_errors);
    // f(&TreePaths::next_distance_error_path);
    // f(&TreePaths::next_box_marker_path);
    // f(&TreePaths::next_scene_marker_path);
    f(&TreePaths::total_error);
  }

  bool operator==(const TreePaths &arg) const { return isEqual(*this, arg); }
};


#endif /* TREEPATHS_HPP_ */
