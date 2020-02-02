#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

struct BodyTranslationChannel;
struct BodyRotationChannel;


struct Channel {
  struct Visitor {
    virtual void visit(const BodyTranslationChannel &) const = 0;
    virtual void visit(const BodyRotationChannel &) const = 0;
  };

  virtual void accept(const Visitor &) const = 0;
};


struct BodyXYZComponentChannel : Channel {
  BodyIndex body_index;
  XYZComponent component;

  BodyXYZComponentChannel(BodyIndex body_index, XYZComponent component)
  : body_index(body_index),
    component(component)
  {
  }
};


struct BodyTranslationChannel : BodyXYZComponentChannel {
  using BodyXYZComponentChannel::BodyXYZComponentChannel;

  virtual void accept(const Visitor &visitor) const
  {
    visitor.visit(*this);
  }
};


struct BodyRotationChannel : BodyXYZComponentChannel {
  using BodyXYZComponentChannel::BodyXYZComponentChannel;

  virtual void accept(const Visitor &visitor) const
  {
    visitor.visit(*this);
  }
};


#endif /* CHANNEL_HPP_ */
