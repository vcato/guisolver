template <typename T>
bool isEqual(const T &a,const T &b)
{
  bool equal_so_far = true;

  T::forEachMember([&](auto T::*member_ptr){
    if (equal_so_far) {
      equal_so_far = ((a.*member_ptr) == (b.*member_ptr));
    }
  });

  return equal_so_far;
}
