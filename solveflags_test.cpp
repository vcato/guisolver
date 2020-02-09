#include "solveflags.hpp"

using TransformSolveFlags = SceneState::TransformSolveFlags;


namespace {
struct Tester {
  TransformSolveFlags solve_flags;
  TransformSolveFlags visit;
  vector<bool *> visited_flag_ptrs;
  vector<bool *> expected_visited_flag_ptrs;
};
}


static void runTest(Tester &tester)
{
  TransformSolveFlags &solve_flags = tester.solve_flags;
  TransformSolveFlags &visit = tester.visit;
  vector<bool *> &visited_flag_ptrs = tester.visited_flag_ptrs;
  vector<bool *> &expected_visited_flag_ptrs = tester.expected_visited_flag_ptrs;

  forEachSolveFlagInTransform(
    solve_flags,
    [&visited_flag_ptrs](bool &arg){ visited_flag_ptrs.push_back(&arg); },
    visit
  );

  assert(visited_flag_ptrs == expected_visited_flag_ptrs);
}


static void
testWithAllVisitFlagsOff()
{
  Tester tester;
  runTest(tester);
}


static void
testWithTranslationMember(bool SceneState::XYZSolveFlags::*member_ptr)
{
  Tester tester;
  TransformSolveFlags &solve_flags = tester.solve_flags;
  (tester.visit.translation.*member_ptr) = true;

  tester.expected_visited_flag_ptrs.push_back(
    &(solve_flags.translation.*member_ptr)
  );

  runTest(tester);
}


static void
testWithRotationMember(bool SceneState::XYZSolveFlags::*member_ptr)
{
  Tester tester;
  TransformSolveFlags &solve_flags = tester.solve_flags;
  (tester.visit.rotation.*member_ptr) = true;

  tester.expected_visited_flag_ptrs.push_back(
    &(solve_flags.rotation.*member_ptr)
  );

  runTest(tester);
}


static void testWithScale()
{
  Tester tester;
  TransformSolveFlags &solve_flags = tester.solve_flags;
  tester.visit.scale = true;
  tester.expected_visited_flag_ptrs.push_back(&solve_flags.scale);
  runTest(tester);
}


int main()
{
  testWithAllVisitFlagsOff();
  testWithTranslationMember(&SceneState::XYZSolveFlags::x);
  testWithTranslationMember(&SceneState::XYZSolveFlags::y);
  testWithTranslationMember(&SceneState::XYZSolveFlags::z);
  testWithRotationMember(&SceneState::XYZSolveFlags::x);
  testWithRotationMember(&SceneState::XYZSolveFlags::y);
  testWithRotationMember(&SceneState::XYZSolveFlags::z);
  testWithScale();
}
