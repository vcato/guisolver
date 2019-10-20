#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"


struct TreePaths {
  struct XYZ {
    TreePath x;
    TreePath y;
    TreePath z;
  };

  struct Translation : XYZ
  {
    Translation() {}
    explicit Translation(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Rotation : XYZ
  {
    Rotation() {}
    explicit Rotation(const XYZ &xyz_arg) : XYZ(xyz_arg) {}
  };

  struct Box {
    Translation translation;
    Rotation rotation;
  };

  Box box;
};


#endif /* TREEPATHS_HPP_ */
