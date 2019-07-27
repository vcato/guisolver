#ifndef VIEWTYPE_HPP_
#define VIEWTYPE_HPP_

struct ViewType {
  enum Value {
    free,
    front,
    top,
    side
  };

  Value value;

  ViewType(Value value) : value(value) { }
  operator Value() const { return value; }
};

#endif /* VIEWTYPE_HPP_ */
