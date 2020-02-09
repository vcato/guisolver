#ifndef COMPONENTINDEX_HPP_
#define COMPONENTINDEX_HPP_

enum class XYZComponent { x, y, z };


template <typename F>
static void forEachXYZComponent(const F &f)
{
  f(XYZComponent::x);
  f(XYZComponent::y);
  f(XYZComponent::z);
}


#endif /* COMPONENTINDEX_HPP_ */
