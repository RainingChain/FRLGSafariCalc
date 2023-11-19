#pragma once

#include "Types.hpp"


using R128 = double;

/*
#define R128_IMPLEMENTATION
#include "R128.hpp"
*/



struct Fraction
{
  static const Fraction ONE;
  static const Fraction ZERO;

  R128 val = 1;

  Fraction() {}

  Fraction(double val)
  {
    this->val = val;
  }

  Fraction(double num, double denum)
  {
    this->val = num / denum;
  }

  Fraction Complement() const
  {
    return Fraction(R128(1) - this->val);
  }
  void Mul(int factor)
  {
    this->val *= R128(factor);
  }

  void Mul(const Fraction& factor)
  {
    this->val *= factor.val;
  }
  Fraction MulNew(const Fraction& factor) const
  {
    auto copy = this->Clone();
    copy.Mul(factor);
    return copy;
  }


  void Add(const Fraction& toAdd)
  {
    this->val += toAdd.val;
  }

  Fraction AddNew(const Fraction& toAdd) const
  {
    auto copy = this->Clone();
    copy.Add(toAdd);
    return copy;
  }

  Fraction Clone() const
  {
    return Fraction(this->val);
  }

  double ToFloat() const
  {
    return this->val;
  }

  std::string ToStr() const
  {
    return std::to_string((double)this->val);
  }
};

const Fraction Fraction::ONE = Fraction(1);
const Fraction Fraction::ZERO = Fraction(0);

