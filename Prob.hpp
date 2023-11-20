#pragma once

#include "Types.hpp"

#define R128_IMPLEMENTATION
#include "R128.hpp"

// There are two implementations for Prob: 
//    - double (64-bits precision, faster)
//    - R128 (128-bits precision, slower)
// In most cases, both implementations return very similar results.

using ProbImplType = double;
// using ProbImplType = R128;

struct Prob
{
  static const Prob ONE;
  static const Prob ZERO;

  ProbImplType val = 1;

  Prob() {}

  Prob(double val)
  {
    this->val = val;
  }

  /* Represents a fraction (num / denom) */
  Prob(double num, double denom)
  {
    this->val = num / denom;
  }

  Prob Complement() const
  {
    return Prob(ProbImplType(1) - this->val);
  }

  void Mul(int factor)
  {
    this->val *= ProbImplType(factor);
  }

  void Mul(const Prob& factor)
  {
    this->val *= factor.val;
  }
  Prob MulNew(const Prob& factor) const
  {
    auto copy = this->Clone();
    copy.Mul(factor);
    return copy;
  }

  void Add(const Prob& toAdd)
  {
    this->val += toAdd.val;
  }

  Prob AddNew(const Prob& toAdd) const
  {
    auto copy = this->Clone();
    copy.Add(toAdd);
    return copy;
  }

  Prob Clone() const
  {
    return Prob(this->val);
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

const Prob Prob::ONE = Prob(1);
const Prob Prob::ZERO = Prob(0);

