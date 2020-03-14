#ifndef TREEITEMDESCRIPTION_HPP_
#define TREEITEMDESCRIPTION_HPP_


struct TreeItemDescription {
  enum class Type {
    scene,
    body,
    box,
    mesh,
    line,
    marker,
    distance_error,
    translation,
    rotation,
    variable,
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
  bool has_rotation_ancestor = false;
  bool has_translation_ancesor = false;
};


#endif /* TREEITEMDESCRIPTION_HPP_ */
