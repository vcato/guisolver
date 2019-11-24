#include "scenestateio.hpp"

using std::ostringstream;


int main()
{
  ostringstream stream;
  SceneState state;
  printSceneStateOn(stream, state);
}
