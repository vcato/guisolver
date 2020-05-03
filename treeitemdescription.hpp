#ifndef TREEITEMDESCRIPTION_HPP_
#define TREEITEMDESCRIPTION_HPP_

#include "optional.hpp"
#include "bodyindex.hpp"
#include "distanceerrorindex.hpp"
#include "meshindex.hpp"
#include "variableindex.hpp"
#include "lineindex.hpp"
#include "boxindex.hpp"
#include "markerindex.hpp"


struct TreeItemDescription {
  enum class Type {
    scene,
    body,
    box_geometry,
    mesh_geometry,
    line_geometry,
    marker,
    distance_error,
    translation,
    rotation,
    scale,
    variable,
    mesh_positions,
    other
  };

  Type type = Type::other;
  Optional<BodyIndex> maybe_body_index;
  Optional<MarkerIndex> maybe_marker_index;
  Optional<BoxIndex> maybe_box_index;
  Optional<MeshIndex> maybe_mesh_index;
  Optional<LineIndex> maybe_line_index;
  Optional<VariableIndex> maybe_variable_index;
  Optional<DistanceErrorIndex> maybe_distance_error_index;
  Optional<MeshPositionIndex> maybe_mesh_position_index;
  bool has_rotation_ancestor = false;
  bool has_translation_ancestor = false;
  bool has_scale_ancestor = false;
};


#endif /* TREEITEMDESCRIPTION_HPP_ */
