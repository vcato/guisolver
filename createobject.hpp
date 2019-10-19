#ifndef CREATEOBJECT_HPP
#define CREATEOBJECT_HPP


template <typename T,typename U>
void createObject(T& object,U &&value)
{
  new (&object) T(std::forward<U>(value));
}


template <typename T>
void createObject(T& object)
{
  new (&object) T();
}


template <typename T>
void destroyObject(T& object)
{
  object.~T();
}

#endif /* CREATEOBJECT_HPP */
