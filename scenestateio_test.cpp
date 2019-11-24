#include "scenestateio.hpp"

#include "defaultscenestate.hpp"

using std::ostringstream;
using std::cerr;


int main()
{
  ostringstream stream;
  SceneState state = defaultSceneState();
  printSceneStateOn(stream, state);
  std::string state_string = stream.str();
}
