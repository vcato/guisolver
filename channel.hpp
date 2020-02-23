#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include "boxindex.hpp"


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


struct BodyScale {
  BodyIndex body_index;
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
using BodyScaleChannel = BasicChannel<BodyScale>;
using BodyBoxScaleChannel = BasicChannel<BodyBoxScaleComponent>;
using BodyBoxCenterChannel = BasicChannel<BodyBoxCenterComponent>;
using MarkerPositionChannel = BasicChannel<MarkerPositionComponent>;


struct Channel {
  struct Visitor {
    virtual void visit(const BodyTranslationChannel &) const = 0;
    virtual void visit(const BodyRotationChannel &) const = 0;
    virtual void visit(const BodyScaleChannel &) const = 0;
    virtual void visit(const BodyBoxScaleChannel &) const = 0;
    virtual void visit(const BodyBoxCenterChannel &) const = 0;
    virtual void visit(const MarkerPositionChannel &) const = 0;
  };

  virtual void accept(const Visitor &) const = 0;

  template <typename Function>
  struct FunctionVisitor : Visitor {
    const Function &function;

    FunctionVisitor(const Function &function)
    : function(function)
    {
    }

    void visit(const BodyTranslationChannel &arg) const override
    {
      function(arg);
    }

    void visit(const BodyRotationChannel &arg) const override
    {
      function(arg);
    }

    void visit(const BodyScaleChannel &arg) const override
    {
      function(arg);
    }

    void visit(const BodyBoxScaleChannel &arg) const override
    {
      function(arg);
    }

    void visit(const BodyBoxCenterChannel &arg) const override
    {
      function(arg);
    }

    void visit(const MarkerPositionChannel &arg) const override
    {
      function(arg);
    }
  };

  template <typename Function>
  void visit(const Function &f) const
  {
    accept(FunctionVisitor<Function>(f));
  }
};


template <typename ValueDescriptor>
struct BasicChannel : Channel, ValueDescriptor {
  using ValueDescriptor::ValueDescriptor;

  BasicChannel(ValueDescriptor value_descriptor)
  : ValueDescriptor(value_descriptor)
  {
  }

  virtual void accept(const Visitor &visitor) const
  {
    visitor.visit(*this);
  }
};


template <typename Element>
inline BasicChannel<Element> elementChannel(const Element &element)
{
  return {element};
}


#endif /* CHANNEL_HPP_ */
