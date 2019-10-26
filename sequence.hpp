#ifndef SEQUENCE_HPP_
#define SEQUENCE_HPP_


template <typename T>
struct Sequence {
  T begin_value;
  T end_value;

  struct iterator {
    T value;

    T operator*() { return value; }

    iterator &operator++()
    {
      ++value;
      return *this;
    }

    bool operator==(const iterator &arg) const
    {
      return value == arg.value;
    }

    bool operator!=(const iterator &arg) const
    {
      return !operator==(arg);
    }
  };

  iterator begin() const { return iterator{begin_value}; }
  iterator end() const { return iterator{end_value}; }
};


#endif /* SEQUENCE_HPP_ */
