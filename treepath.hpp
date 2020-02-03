#ifndef TREEPATH_HPP_
#define TREEPATH_HPP_

#include <cassert>
#include "vector.hpp"


using TreeItemIndex = int;
using TreePath = vector<TreeItemIndex>;


inline TreePath childPath(TreePath path)
{
  return path;
}


inline void
addChildToPath(TreePath &path, TreeItemIndex child_index)
{
  assert(child_index >= 0);
  path.push_back(child_index);
}


template <typename... Indices>
inline TreePath
  childPath(
    TreePath path,
    TreeItemIndex child_index1,
    Indices... rest
  )
{
  addChildToPath(path, child_index1);
  return childPath(path,rest...);
}


inline TreePath parentPath(const TreePath &path)
{
  TreePath result = path;
  result.pop_back();
  return result;
}


#endif /* TREEPATH_HPP_ */
