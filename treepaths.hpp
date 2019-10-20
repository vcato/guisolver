#ifndef TREEPATHS_HPP_
#define TREEPATHS_HPP_

#include "treepath.hpp"


struct TreePaths {
  struct Translation {
    TreePath x;
    TreePath y;
    TreePath z;
  };

  struct Box {
    Translation translation;
  };

  Box box;
};


#endif /* TREEPATHS_HPP_ */
