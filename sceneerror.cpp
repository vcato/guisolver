#include "sceneerror.hpp"


using Vector3f = Eigen::Vector3f;


namespace {


struct RotationValues {
  float x,y,z;
};


}


namespace {

struct SceneState {
  struct Line {
    Point start;
    Point end;
  };

  std::vector<Line> lines;
  Transform box_global;
};

}


#if ADD_BOX_POSITION_ERROR
static float dot(Vector3f a,Vector3f b)
{
  return a.dot(b);
}
#endif


#if ADD_BOX_POSITION_ERROR
float
  boxPositionError(
    const SceneState &scene_state
  )
{
  using Scalar = float;

  Scalar error = 0;

  // Add an error for each line
  for (auto &line : scene_state.lines) {
    Point start_predicted = scene_state.box_global * line.start;
    Point end_predicted = line.end;
    Point delta = start_predicted - end_predicted;
    error += dot(delta,delta);
  }

  return error;
}
#endif


#if USE_SOLVER
void
  solveBoxPosition(
    const SceneSetup &scene_setup,
    const Transform &box_global_transform
  )
{
  using Scalar = float;
  template <typename T> using Expr = ExprGraph::Node<T>;

  ExprGraph graph;
  Expr<Scalar> error = graph.add(0);

  // Create the global transform of the box
  auto order = RotAxes{Axis::x, Axis::y, Axis::z};
  CoordinateAxes box_axes = scene.coordinateAxes(box);
  RotationValues box_rotation_values = decompose(axes, order);
  Vec3 box_trans = scene.translation(box);
  Expr<Scalar> box_rx = graph.addVar(box_rotation_values.x);
  Expr<Scalar> box_ry = graph.addVar(box_rotation_values.y);
  Expr<Scalar> box_rz = graph.addVar(box_rotation_values.z);
  Expr<Scalar> box_tx = graph.addVar(box_trans.x);
  Expr<Scalar> box_ty = graph.addVar(box_trans.y);
  Expr<Scalar> box_tz = graph.addVar(box_trans.z);

  Expr<RotationValues> rotation_values =
    graph.add(RotationValues{start_body_rx, start_body_ry, start_body_rz});

  Expr<RotationMatrix> start_body_rot =
    graph.add(compose(rotation_values, order));

  Expr<Vec3> start_body_trans =
    graph.add(
      vec3(start_box_tx, start_body_ty, start_body_tz)
    );

  Expr<Transform> box_global =
    graph.add(Transform(start_body_rot, start_body_trans));

  // Add an error for each line
  for (int i=0; i!=n_lines; ++i) {
    Expr<Vec3> end_local = graph.add(line.end);
    Expr<Vec3> start_predicted = graph.add(box_global * line.start);
    Expr<Vec3> end_predicted = end_local;
    Expr<Vec3> delta = graph.add(start_predicted - end_predicted);
    error = graph.add(error,dot(delta,delta));
  }

  // Optimize.
  minimize(error);

  // Extract the resulting box position.
  Scalar new_rx = graph.get(box_rx);
  Scalar new_ry = graph.get(box_ry);
  Scalar new_rz = graph.get(box_rz);
  Scalar new_tx = graph.get(box_tx);
  Scalar new_ty = graph.get(box_ty);
  Scalar new_tz = graph.get(box_tz);

  RotationMatrix new_rot = compose(rotations(new_rx,new_ry,new_rz), order);
  Vec3 new_trans = {new_tx, new_ty, new_tz};
  return Transform(new_rot, new_trans);
}
#endif
