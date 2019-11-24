#ifndef ENUMERATIONVALUE_HPP_
#define ENUMERATIONVALUE_HPP_

#include <string>


struct EnumerationValue
{
  std::string name;

  bool operator==(const EnumerationValue &arg) const
  {
    return name==arg.name;
  }
};


#endif /* ENUMERATIONVALUE_HPP_ */
