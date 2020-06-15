#include "scenestate.hpp"


template <typename XYZSolveFlags, typename F>
static void
maybeVisitComponent(
  XYZSolveFlags &solve_flags,
  const F &f,
  const SceneState::XYZSolveFlags &visit,
  XYZComponent component
)
{
  switch (component) {
    case XYZComponent::x:
      if (visit.x) f(solve_flags.x);
      return;
    case XYZComponent::y:
      if (visit.y) f(solve_flags.y);
      return;
    case XYZComponent::z:
      if (visit.z) f(solve_flags.z);
      return;
  }
}


template <typename Visitor>
void forEachSolvableTransformElement(const Visitor &v)
{
  forEachXYZComponent(
    [&](XYZComponent component){ v.visitTranslationComponent(component); }
  );

  forEachXYZComponent(
    [&](XYZComponent component){ v.visitRotationComponent(component); }
  );

  v.visitScale();
}


template <typename TransformSolveFlags, typename F>
static void
forEachSolveFlagInTransform(
  TransformSolveFlags &solve_flags,
  const F &f,
  const SceneState::TransformSolveFlags &visit
)
{
  struct Visitor {
    TransformSolveFlags &solve_flags;
    const F &f;
    const SceneState::TransformSolveFlags &visit;

    void visitTranslationComponent(XYZComponent component) const
    {
      maybeVisitComponent(
        solve_flags.translation, f, visit.translation, component
      );
    }

    void visitRotationComponent(XYZComponent component) const
    {
      maybeVisitComponent(
        solve_flags.rotation, f, visit.rotation, component
      );
    }

    void visitScale() const
    {
      if (visit.scale) f(solve_flags.scale);
    }
  };

  Visitor v{solve_flags, f, visit};
  forEachSolvableTransformElement(v);
}
