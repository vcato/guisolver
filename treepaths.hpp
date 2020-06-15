#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"
#include "isequal.hpp"
#include "markerindex.hpp"
#include "bodyindex.hpp"
#include "optional.hpp"
#include "matchconst.hpp"
#include "xyzcomponent.hpp"


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

  template <typename Component>
  struct BasicXYZ {
    TreePath path;
    Component x;
    Component y;
    Component z;

    const Component &component(XYZComponent component) const
    {
      switch (component) {
        case XYZComponent::x: return x;
        case XYZComponent::y: return y;
        case XYZComponent::z: return z;
      }

      assert(false);
      return x;
    }

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&BasicXYZ::path);
      f(&BasicXYZ::x);
      f(&BasicXYZ::y);
      f(&BasicXYZ::z);
    }

    bool operator==(const BasicXYZ &arg) const
    {
      return isEqual(*this, arg);
    }
  };

  using XYZ = BasicXYZ<TreePath>;

  struct Channel {
    TreePath path;
    Optional<TreePath> maybe_solve_path;
    TreePath expression_path;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Channel::path);
      f(&Channel::maybe_solve_path);
      f(&Channel::expression_path);
    }

    bool operator==(const Channel &arg) const
    {
      return isEqual(*this, arg);
    }
  };


  using XYZChannels = BasicXYZ<Channel>;

  struct Position : XYZChannels {
    Position() {}
    explicit Position(const XYZChannels &xyz_arg) : XYZChannels(xyz_arg) {}
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

  struct Translation : XYZChannels
  {
    Translation() {}
    explicit Translation(const XYZChannels &xyz_arg) : XYZChannels(xyz_arg) {}
  };

  struct Rotation : XYZChannels
  {
    Rotation() {}
    explicit Rotation(const XYZChannels &xyz_arg) : XYZChannels(xyz_arg) {}
  };

  struct Scale : XYZChannels
  {
    Scale() {}
    explicit Scale(const XYZChannels &xyz_arg) : XYZChannels(xyz_arg) {}
  };

  struct Box {
    TreePath path;
    Scale scale;
    XYZChannels center;

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

  struct Positions {
    TreePath path;
    vector<XYZ> elements;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Positions::path);
      f(&Positions::elements);
    }

    bool operator==(const Positions &arg) const
    {
      return isEqual(*this, arg);
    }
  };

  struct Mesh {
    TreePath path;
    XYZChannels scale;
    XYZ center;
    Positions positions;

    template <typename F>
    static void forEachMember(const F &f)
    {
      f(&Mesh::path);
      f(&Mesh::scale);
      f(&Mesh::center);
      f(&Mesh::positions);
    }

    bool operator==(const Mesh &arg) const { return isEqual(*this, arg); }
  };

  struct Body {
    TreePath path;
    TreePath name;
    Translation translation;
    Rotation rotation;
    Channel scale;
    vector<Box> boxes;
    vector<Line> lines;
    vector<Mesh> meshes;

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
      f(&Body::scale);
      f(&Body::boxes);
      f(&Body::lines);
      f(&Body::meshes);
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

    const TreePath &valuePath() { return path; }
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


inline const TreePaths::Channel &
markerPositionComponentChannelPaths(
  MarkerIndex marker_index, XYZComponent component, const TreePaths &tree_paths
)
{
  return
    tree_paths
      .marker(marker_index)
      .position
      .component(component);
}



inline const TreePath &
markerPositionComponentPath(
  MarkerIndex marker_index, XYZComponent component, const TreePaths &tree_paths
)
{
  return
    markerPositionComponentChannelPaths(
      marker_index, component, tree_paths
    ).path;
}



#endif /* TREEPATHS_HPP_ */
