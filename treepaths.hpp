#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"


struct TreePaths {
  struct Marker;
  struct DistanceError;
  using Markers = std::vector<Marker>;
  using DistanceErrors = std::vector<DistanceError>;

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

  struct Box {
    TreePath path;
    Translation translation;
    Rotation rotation;
  };

  struct DistanceError {
    TreePath path;
    TreePath distance;
  };

  Box box;
  Markers markers;
  DistanceErrors distance_errors;
};


#endif /* TREEPATHS_HPP_ */
