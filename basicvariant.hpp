#ifndef BASICVARIANT_HPP
#define BASICVARIANT_HPP


#include <utility>
#include "createobject.hpp"
#include "noinittag.hpp"


// This class defines general things that all variants have, such as
// copy, assignment, equality and a visit() method.  The details of
// what types are supported by the variant, how the data is stored, and
// additional methods are defined by the provided Policy class.
template <typename Policy>
class BasicVariant : public Policy {
  private:
    using Self = BasicVariant;
    using PossibleValues = typename Policy::PossibleValues;

  public:
    using Type = typename Policy::Type;
    using Policy::Policy;

    BasicVariant()
    : Policy()
    {
    }

    explicit BasicVariant(const BasicVariant& arg)
    : Policy(NoInitTag{})
    {
      _create(arg);
    }

    BasicVariant(BasicVariant&& arg)
    : Policy(NoInitTag{})
    {
      _create(std::move(arg));
    }

    ~BasicVariant()
    {
      _destroy();
    }

    BasicVariant& operator=(BasicVariant&& arg)
    {
      if (this->_type != arg._type) {
        _destroy();
        _create(std::move(arg));
      }
      else {
        Self::withMemberPtrFor(
          this->_type,
          MemberMoveAssignment{
            this->_possible_values, std::move(arg._possible_values)
          }
        );
      }

      return *this;
    }

    BasicVariant& operator=(const BasicVariant &arg)
    {
      if (this->_type != arg._type) {
        _destroy();
        _create(arg);
      }
      else {
        Self::withMemberPtrFor(
          this->_type,
          MemberCopyAssignment{this->_possible_values, arg._possible_values}
        );
      }

      return *this;
    }

    bool operator==(const BasicVariant &arg) const
    {
      if (this->_type!=arg._type) return false;

      return
        Self::withMemberPtrFor(
          this->_type,
          MemberEquality{this->_possible_values,arg._possible_values}
        );
    }

    bool operator!=(const BasicVariant &arg) const
    {
      return !operator==(arg);
    }

    Type type() const { return this->_type; }

    template <typename Function>
    auto visit(const Function &f) const
    {
      Self::withMemberPtrFor(
        this->_type,
        [&](auto PossibleValues::*member_ptr){
          return f(this->_possible_values.*member_ptr);
        }
      );
    }

  private:
    struct MemberCopyAssignment {
      PossibleValues &a;
      const PossibleValues &b;

      template <typename T>
      void operator()(T PossibleValues::*p) const
      {
        (a.*p) = (b.*p);
      }
    };

    struct MemberMoveAssignment {
      PossibleValues &a;
      PossibleValues &&b;

      template <typename T>
      void operator()(T PossibleValues::*p) const
      {
        (a.*p) = std::move(b.*p);
      }
    };

    struct MemberCopy {
      PossibleValues &a;
      const PossibleValues &b;

      template <typename T>
      void operator()(T PossibleValues::*p) const
      {
        createObject(a.*p,b.*p);
      }
    };

    struct MemberMove {
      PossibleValues &a;
      PossibleValues &&b;

      template <typename T>
      void operator()(T PossibleValues::*p) const
      {
        createObject(a.*p,std::move(b.*p));
      }
    };

    struct MemberEquality {
      const PossibleValues &a,&b;

      template <typename T>
      bool operator()(T PossibleValues::*p) const
      {
        return (a.*p) == (b.*p);
      }
    };

    struct MemberDestroy {
      PossibleValues &_value;

      template <typename T>
      void operator()(T PossibleValues::*p) const
      {
        (_value.*p).~T();
      }
    };

    void _create(BasicVariant&& arg)
    {
      this->_type = arg._type;

      Self::withMemberPtrFor(
        this->_type,
        MemberMove{this->_possible_values,std::move(arg._possible_values)}
      );
    }

    void _create(const BasicVariant& arg)
    {
      this->_type = arg._type;

      Self::withMemberPtrFor(
        this->_type,
        MemberCopy{this->_possible_values, arg._possible_values}
      );
    }

    void _destroy()
    {
      Self::withMemberPtrFor(
        this->_type,
        MemberDestroy{this->_possible_values}
      );
    }
};

#endif /* BASICVARIANT_HPP */
