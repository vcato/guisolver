#define USE_SOLVER 0
#define ADD_BOX_POSITION_ERROR 0

#include <Eigen/Geometry>
#include "setupscene.hpp"

struct Vec3 {
  float x,y,z;
};


struct CoordinateAxes {
  Vec3 x;
  Vec3 y;
  Vec3 z;
};


using Transform = Eigen::Transform<float,3,Eigen::AffineCompact>;
using Point = Eigen::Vector3f;


#if USE_SOLVER
extern void
  solveBoxPosition(
    const SceneSetup &scene_setup,
    const Transform &box_global_transform
  );
#endif


#if ADD_BOX_POSITION_ERROR
extern float
  boxPositionError(
    const SceneSetup &scene_setup,
    const Transform &box_global_transform
  );
#endif
