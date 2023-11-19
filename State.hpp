#pragma once

#include "Types.hpp"
#include "Constants.hpp"
#include "Fraction.hpp"

struct State
{
  static u8 catchRate;
  static u8 safariZoneFleeRate;

  u8 safariEscapeFactor = 0;
  u8 safariCatchFactor = 0;
  u8 safariBaitThrowCounter = 0;
  u8 safariRockThrowCounter = 0;

  State(const State&) = default;

  State()
  {
    this->safariCatchFactor = (u8)(State::catchRate * 100 / 1275);
    this->safariEscapeFactor = (u8)(State::safariZoneFleeRate * 100 / 1275);
    if (this->safariEscapeFactor <= 1)
      this->safariEscapeFactor = 2;
  }

  const std::pair<const Fraction, const Fraction>& GetStayFleeProb() const
  {
    u8 safariFleeRate;

    if (this->safariRockThrowCounter != 0)
    {
      safariFleeRate = this->safariEscapeFactor * 2;
      if (safariFleeRate > 20)
        safariFleeRate = 20;
    }
    else if (this->safariBaitThrowCounter != 0)
    {
      safariFleeRate = this->safariEscapeFactor / 4;
      if (safariFleeRate == 0)
        safariFleeRate = 1;
    }
    else
      safariFleeRate = this->safariEscapeFactor;

    safariFleeRate *= 5;

    return StayFleeProbBySafariFleeRate[safariFleeRate];
  }

  const std::pair<const Fraction, const Fraction>& GetCatchMissProb() const
  {
    return CatchMissProbBySafariCatchFactor[this->safariCatchFactor];
  }

  void ApplyPlayerAction(PlayerAction playerAction, u8 playerActionValue)
  {
    if (playerAction == PlayerAction::bait)
      this->OnBait(playerActionValue);
    else if (playerAction == PlayerAction::rock)
      this->OnRock(playerActionValue);
    else if (playerAction == PlayerAction::ball)
      this->OnBall(playerActionValue);
  }

  void OnBall(u8 playerActionValue)
  {
    // Nothing to do. Handle by Node class
  }

  void OnRock(u8 playerActionValue)
  {
    this->safariRockThrowCounter = playerActionValue;
    this->safariBaitThrowCounter = 0;
    this->safariCatchFactor <<= 1;
    if (this->safariCatchFactor > 20)
      this->safariCatchFactor = 20;
  }

  void OnBait(u8 safariBaitThrowCounter)
  {
    this->safariBaitThrowCounter = safariBaitThrowCounter;
    this->safariRockThrowCounter = 0;
    this->safariCatchFactor >>= 1;
    if (this->safariCatchFactor <= 2)
      this->safariCatchFactor = 3;
  }

  void ApplyPokemonAction(PokemonAction PokemonAction)
  {
    //if (PokemonAction == PokemonAction::flee)
    //  Nothing to do. Handled by Node class

    if (PokemonAction == PokemonAction::watchCarefully)
    {
      if (this->safariRockThrowCounter != 0)
      {
        --this->safariRockThrowCounter;
        if (this->safariRockThrowCounter == 0)
          this->safariCatchFactor = (u8)(State::catchRate * 100 / 1275);
      }
      else if (this->safariBaitThrowCounter != 0)
        --this->safariBaitThrowCounter;
    }
  }
};

u8 State::catchRate = 0;
u8 State::safariZoneFleeRate = 0;
