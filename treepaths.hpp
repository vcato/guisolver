#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"
#include "isequal.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "optional.hpp"
#include "matchconst.hpp"

struct TreePaths {
  struct Marker;
  struct Body;
  struct DistanceError;
  struct Variable;
  using Markers = vector<Optional<Marker>>;
  using Bodies = vector<Optional<Body>>;
  using DistanceErrors = vector<DistanceError>;
  using Variables = vector<Variable>;

  TreePaths()
  : markers(),
    bodies()
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
    TreePath name;
    Position position;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Marker::path);
      f(&Marker::name);
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
    TreePath path;
    Scale scale;
    XYZ center;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Box::path);
      f(&Box::scale);
      f(&Box::center);
    }

    bool operator==(const Box &arg) const { return isEqual(*this, arg); }
  };

  struct Line {
    TreePath path;
    XYZ start;
    XYZ end;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Line::path);
      f(&Line::start);
      f(&Line::end);
    }

    bool operator==(const Line &arg) const { return isEqual(*this, arg); }
  };

  struct Body {
    TreePath path;
    TreePath name;
    Translation translation;
    Rotation rotation;
    vector<Box> boxes;
    vector<Line> lines;

    Body()
    {
    }

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Body::path);
      f(&Body::name);
      f(&Body::translation);
      f(&Body::rotation);
      f(&Body::boxes);
      f(&Body::lines);
    }

    bool operator==(const Body &arg) const
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

  struct Variable {
    TreePath path;
    TreePath name;
    TreePath value;
  };

  TreePath path;
  Markers markers;
  Bodies bodies;
  DistanceErrors distance_errors;
  Variables variables;
  TreePath total_error;

  template <typename TreePaths>
  static MatchConst_t<Marker, TreePaths> &
  marker(MarkerIndex i, TreePaths &tree_paths)
  {
    assert(tree_paths.markers[i]);
    return *tree_paths.markers[i];
  }

  template <typename TreePaths>
  static MatchConst_t<Body, TreePaths> &
  body(BodyIndex i, TreePaths &tree_paths)
  {
    assert(tree_paths.bodies[i]);
    return *tree_paths.bodies[i];
  }

  Body &body(MarkerIndex i) { return body(i, *this); }
  const Body &body(MarkerIndex i) const { return body(i, *this); }
  Marker &marker(MarkerIndex i) { return marker(i, *this); }
  const Marker &marker(MarkerIndex i) const { return marker(i, *this); }

  template <typename F>
  static void forEachMember(const F &f)
  {
    f(&TreePaths::path);
    f(&TreePaths::bodies);
    f(&TreePaths::markers);
    f(&TreePaths::distance_errors);
    f(&TreePaths::total_error);
  }

  bool operator==(const TreePaths &arg) const { return isEqual(*this, arg); }
};


#endif /* TREEPATHS_HPP_ */
