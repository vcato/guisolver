#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"


struct TreePaths {
  struct Marker;
  struct DistanceError;
  using Markers = vector<Marker>;
  using DistanceErrors = vector<DistanceError>;

  TreePaths()
  : markers(6),
    distance_errors(3)
  {
  }

  struct XYZ {
    TreePath path;
    TreePath x;
    TreePath y;
    TreePath z;
  };

  struct Position : XYZ {
    Position() {}
    explicit Position(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Marker {
    TreePath path;
    Position position;
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
    };

    TreePath path;
    Translation translation;
    Rotation rotation;
    Geometry geometry;
  };

  struct DistanceError {
    TreePath path;
    TreePath start;
    TreePath end;
    TreePath distance;
    TreePath desired_distance;
    TreePath weight;
    TreePath error;
  };

  Box box;
  Markers markers;
  DistanceErrors distance_errors;
  TreePath total_error;
};


#endif /* TREEPATHS_HPP_ */
