#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include "boxindex.hpp"
#include "sceneelements.hpp"


inline Marker markerOf(MarkerPosition arg)
{
  return arg.marker;
}


template <typename Parent>
inline Marker markerOf(ElementXYZComponent<Parent> arg)
{
  return markerOf(arg.parent);
}


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



inline Body bodyOf(BodyLine arg)
{
  return arg.body;
}


inline Body bodyOf(BodyLineStart arg)
{
  return bodyOf(arg.body_line);
}


inline Body bodyOf(BodyLineEnd arg)
{
  return bodyOf(arg.body_line);
}


inline BodyLine bodyLineOf(BodyLineStart arg)
{
  return arg.body_line;
}


inline BodyMesh bodyMeshOf(BodyMeshPositions arg)
{
  return arg.body_mesh;
}


inline BodyMesh bodyMeshOf(BodyMeshPosition arg)
{
  return bodyMeshOf(arg.array);
}


inline BodyMesh bodyMeshOf(BodyMeshScale arg)
{
  return arg.body_mesh;
}


inline BodyMesh bodyMeshOf(BodyMeshCenter arg)
{
  return arg.body_mesh;
}


inline Body bodyOf(BodyMeshScale arg)
{
  return bodyMeshOf(arg).body;
}


template <typename Parent>
inline BodyMesh bodyMeshOf(ElementXYZComponent<Parent> arg)
{
  return bodyMeshOf(arg.parent);
}


inline BodyLine bodyLineOf(BodyLineEnd arg)
{
  return arg.body_line;
}


template <typename Parent>
inline BodyLine bodyLineOf(ElementXYZComponent<Parent> arg)
{
  return bodyLineOf(arg.parent);
}


using BodyBoxCenterComponent = ElementXYZComponent<BodyBoxCenter>;
using BodyLineStartComponent = ElementXYZComponent<BodyLineStart>;
using BodyLineEndComponent = ElementXYZComponent<BodyLineEnd>;


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


template <typename ValueDescriptor> struct BasicChannel;


using MarkerPositionComponent = ElementXYZComponent<MarkerPosition>;
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
