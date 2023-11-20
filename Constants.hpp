#pragma once

#include "Types.hpp"
#include "Prob.hpp"

/* 
StayFleeProbBySafariFleeRate[SafariFleeRate] = {StayProbability, FleeProbability} 
Ex: StayFleeProbBySafariFleeRate[45] =  {~55%, ~45%}
*/
static const std::vector<std::pair<const Prob, const Prob>> StayFleeProbBySafariFleeRate = ([]()
  {
    std::vector<std::pair<const Prob, const Prob>> arr;
    for (u32 safariFleeRate = 0; safariFleeRate <= 255; safariFleeRate++)
    {
      // RandomUint16 % 100 is more likely to be between 0-35 then 36+
      u32 count_below36 = 656;        // there are 656 possible Uint16 values such as Uint16 % 100 == 0
      u32 count_at36AndAbove = 655;   // there are 655 possible Uint16 values such as Uint16 % 100 == 36

      u32 count = 0;
      for (u32 i = 0; i < safariFleeRate; i++)
        count += i < 36 ? count_below36 : count_at36AndAbove;

      arr.emplace_back(Prob(65536 - count, 65536), Prob(count, 65536));
    }
  return arr;
  })();


/*
CatchMissProbBySafariCatchFactor[SafariCatchFactor] = {CatchProbability, MissProbability}
Ex: CatchMissProbBySafariCatchFactor[3] =  {~8%, ~92%}
*/
static const std::vector<std::pair<const Prob, const Prob>> CatchMissProbBySafariCatchFactor = ([]()
  {
    std::vector<std::pair<const Prob, const Prob>> arr;

    auto Sqrt = [](u32 val)
    {
      // gba Sqrt truncates. See http://starflakenights.net/libraries/devkitpro-libgba/docs/html/a00019.html#a72965f900a655e3dc76d2491d415d2a4
      return (u16)std::sqrt(val);
    };

    for (u32 safariCatchFactor = 0; safariCatchFactor <= 255; safariCatchFactor++)
    {
      u32 ballMultiplier = 15;
      u32 catchRate = (safariCatchFactor * 1275 / 100);
      u32 odds = catchRate * ballMultiplier / 30;

      if (odds > 254)
      {
        arr.emplace_back(Prob::ONE, Prob::ZERO);
        continue;
      }
      if (odds == 0)
      {
        arr.emplace_back(Prob::ZERO, Prob::ONE);
        continue;
      }

      odds = Sqrt(Sqrt(16711680 / odds));
      odds = 1048560 / odds;

      Prob baseProb(odds, 65536);
      Prob res(1);
      for (int i = 0; i < 4; i++)
        res.Mul(baseProb);

      arr.emplace_back(res, res.Complement());
    }
  return arr;
  })();

