#include "readobj.hpp"

#include <string>
#include <sstream>

using std::istringstream;
using std::cerr;


ObjData readObj(std::istream &stream)
{
  using Face = ObjData::Face;
  using Vertex = ObjData::Vertex;
  vector<Vertex> vertices;
  vector<Face> faces;
  std::string line;

  while (std::getline(stream, line)) {
    if (line.length() == 0) {
    }
    else if (line[0]=='#') {
    }
    else if (line[0]=='v') {
      istringstream line_stream(line.substr(1));
      float x,y,z;
      line_stream >> x >> y >> z;
      vertices.push_back({x,y,z});
    }
    else if (line[0]=='f') {
      istringstream line_stream(line.substr(1));

      auto readFace = [&]{
        Face face;
        int index = 0;

        while (line_stream >> index) {
          face.vertex_indices.push_back(index);
        }

        return face;
      };

      faces.push_back(readFace());
    }
    else {
      cerr << "line: [" << line << "]\n";
      break;
    }
  }

  return ObjData{vertices, faces};
}
