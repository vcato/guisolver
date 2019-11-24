#include "scenestateio.hpp"

using std::ostream;
using std::cerr;


void printSceneStateOn(ostream &, const SceneState &)
{
  cerr << "printSceneStateOn()\n";
}
