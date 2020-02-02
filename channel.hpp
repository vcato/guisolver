#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

struct BodyTranslationChannel;
struct BodyRotationChannel;
struct BodyBoxScaleChannel;


struct Channel {
  struct Visitor {
    virtual void visit(const BodyTranslationChannel &) const = 0;
    virtual void visit(const BodyRotationChannel &) const = 0;
    virtual void visit(const BodyBoxScaleChannel &) const = 0;
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


struct BodyBoxScaleChannel : BodyXYZComponentChannel {
  BoxIndex box_index;

  BodyBoxScaleChannel(
    BodyIndex body_index, BoxIndex box_index, XYZComponent component
  )
  : BodyXYZComponentChannel(body_index, component),
    box_index(box_index)
  {
  }

  virtual void accept(const Visitor &visitor) const
  {
    visitor.visit(*this);
  }
};


#endif /* CHANNEL_HPP_ */
