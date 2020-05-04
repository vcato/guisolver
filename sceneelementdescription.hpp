#ifndef SCENEELEMENTDESCRIPTION_HPP_
#define SCENEELEMENTDESCRIPTION_HPP_

#include "optional.hpp"
#include "bodyindex.hpp"
#include "distanceerrorindex.hpp"
#include "meshindex.hpp"
#include "variableindex.hpp"
#include "lineindex.hpp"
#include "boxindex.hpp"
#include "markerindex.hpp"


struct SceneElementDescription {
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
    mesh_position,
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

  SceneElementDescription
    (*rotation_ancestor_function_ptr)(const SceneElementDescription &)
    = nullptr;

  SceneElementDescription
    (*translation_ancestor_function_ptr)(const SceneElementDescription &)
    = nullptr;

  SceneElementDescription
    (*scale_ancestor_function_ptr)(const SceneElementDescription &)
    = nullptr;

  bool hasRotationAncestor() const
  {
    return rotation_ancestor_function_ptr != nullptr;
  }

  bool hasTranslationAncestor() const
  {
    return translation_ancestor_function_ptr != nullptr;
  }

  bool hasScaleAncestor() const
  {
    return scale_ancestor_function_ptr != nullptr;
  }
};


#endif /* SCENEELEMENTDESCRIPTION_HPP_ */
