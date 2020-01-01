#ifndef MATCHCONST_HPP_
#define MATCHCONST_HPP_


template<typename T>
struct AsConst {
  using type = const T;
};


template <typename T>
using AsConst_t = typename AsConst<T>::type;


template<typename T, typename Like>
struct MatchConst {
  using type = T;
};


template <typename T, typename Like>
struct MatchConst<T, const Like> {
  using type = AsConst_t<T>;
};


template <typename T, typename Like>
using MatchConst_t = typename MatchConst<T,Like>::type;


#endif /* MATCHCONST_HPP_ */
