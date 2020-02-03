#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

struct BodyComponent {
  BodyIndex body_index;
  XYZComponent component;

  BodyComponent(BodyIndex body_index, XYZComponent component)
  : body_index(body_index),
    component(component)
  {
  }
};


struct BodyTranslationComponent : BodyComponent {
  using BodyComponent::BodyComponent;
};


struct BodyRotationComponent : BodyComponent {
  using BodyComponent::BodyComponent;
};


struct BodyBoxScaleComponent : BodyComponent {
  BoxIndex box_index;

  BodyBoxScaleComponent(
    BodyIndex body_index, BoxIndex box_index, XYZComponent component
  )
  : BodyComponent(body_index, component),
    box_index(box_index)
  {
  }
};


struct BodyBoxCenterComponent : BodyComponent {
  BoxIndex box_index;

  BodyBoxCenterComponent(
    BodyIndex body_index, BoxIndex box_index, XYZComponent component
  )
  : BodyComponent(body_index, component),
    box_index(box_index)
  {
  }
};


struct MarkerPositionComponent {
  MarkerIndex marker_index;
  XYZComponent component;

  MarkerPositionComponent(MarkerIndex marker_index, XYZComponent component)
  : marker_index(marker_index),
    component(component)
  {
  }
};


template <typename ValueDescriptor> struct BasicChannel;


using BodyTranslationChannel = BasicChannel<BodyTranslationComponent>;
using BodyRotationChannel = BasicChannel<BodyRotationComponent>;
using BodyBoxScaleChannel = BasicChannel<BodyBoxScaleComponent>;
using BodyBoxCenterChannel = BasicChannel<BodyBoxCenterComponent>;
using MarkerPositionChannel = BasicChannel<MarkerPositionComponent>;


struct Channel {
  struct Visitor {
    virtual void visit(const BodyTranslationChannel &) const = 0;
    virtual void visit(const BodyRotationChannel &) const = 0;
    virtual void visit(const BodyBoxScaleChannel &) const = 0;
    virtual void visit(const BodyBoxCenterChannel &) const = 0;
    virtual void visit(const MarkerPositionChannel &) const = 0;
  };

  virtual void accept(const Visitor &) const = 0;
};


template <typename ValueDescriptor>
struct BasicChannel : Channel, ValueDescriptor {
  using ValueDescriptor::ValueDescriptor;

  virtual void accept(const Visitor &visitor) const
  {
    visitor.visit(*this);
  }
};


#endif /* CHANNEL_HPP_ */
