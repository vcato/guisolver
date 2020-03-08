#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include "boxindex.hpp"
#include "sceneelements.hpp"


template <typename Parent>
struct ElementXYZComponent {
  Parent parent;
  XYZComponent component;
};


struct BodyXYZComponent {
  Body body;
  XYZComponent component;

  BodyXYZComponent(Body body, XYZComponent component)
  : body(body),
    component(component)
  {
  }
};


struct BodyTranslation {
  Body body;
};


struct BodyRotation {
  Body body;
};


struct BodyScale {
  Body body;
};


struct BodyBoxScale {
  BodyBox body_box;
};


using BodyTranslationComponent = ElementXYZComponent<BodyTranslation>;
using BodyRotationComponent = ElementXYZComponent<BodyRotation>;
using BodyBoxScaleComponent = ElementXYZComponent<BodyBoxScale>;


inline Body bodyOf(BodyTranslation arg)
{
  return arg.body;
}


inline Body bodyOf(BodyRotation arg)
{
  return arg.body;
}


inline Body bodyOf(BodyScale arg)
{
  return arg.body;
}


inline Body bodyOf(BodyBox arg)
{
  return arg.body;
}


template <typename Parent>
inline Body bodyOf(ElementXYZComponent<Parent> arg)
{
  return bodyOf(arg.parent);
}


inline Body bodyOf(BodyBoxScale arg)
{
  return bodyOf(arg.body_box);
}


inline BodyBox bodyBoxOf(BodyBoxScale arg)
{
  return arg.body_box;
}


inline BodyTranslationComponent
bodyTranslationComponent(BodyIndex body_index, XYZComponent component)
{
  return
    BodyTranslationComponent{
      BodyTranslation{Body(body_index)},
      component
    };
}


struct BodyBoxCenter {
  BodyBox body_box;
};


using BodyBoxCenterComponent = ElementXYZComponent<BodyBoxCenter>;


inline BodyBox bodyBoxOf(BodyBoxCenter arg)
{
  return arg.body_box;
}


inline Body bodyOf(BodyBoxCenter arg)
{
  return bodyOf(arg.body_box);
}


template <typename Parent>
inline BodyBox bodyBoxOf(ElementXYZComponent<Parent> arg)
{
  return bodyBoxOf(arg.parent);
}


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
