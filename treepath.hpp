#ifndef TREEPATH_HPP_
#define TREEPATH_HPP_

#include <vector>


using TreeItemIndex = int;
using TreePath = std::vector<TreeItemIndex>;


inline TreePath childPath(TreePath path,TreeItemIndex child_index)
{
  path.push_back(child_index);
  return path;
}


inline TreePath parentPath(const TreePath &path)
{
  TreePath result = path;
  result.pop_back();
  return result;
}


#endif /* TREEPATH_HPP_ */
