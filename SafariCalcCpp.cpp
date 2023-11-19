/*
General idea:
  Create a graph of all branching possibilities
  Sum all catching probabilities.

  Each Node has a player action (bait, rock, ball) and a pokemon action (flee, watch).
  A state represents the bait counter, rock counter etc.
  Each Node has a state before the action and a state after the action.

  Bait 2 (20%)
    Stay (55%)
      Ball Catch (10%)
        Stay (80%)
          ...
        Flee (20%)
      Ball Miss  (90%)
        Stay (80%)
          ...
        Flee (20%)
    Flee (45%)
  Bait 3 (20%)
     ...
  Bait 4 (20%)
     ...
  Bait 5 (20%)
     ...
  Bait 6 (20%)
     ...

*/

#include <iostream>
#include <array>
#include <vector>
#include <string.h>
#include <string>
#include <algorithm>
#include <execution>

#include "Types.hpp"
#include "Constants.hpp"
#include "Fraction.hpp"
#include "State.hpp"

static constexpr int MAX_CHILD_COUNT = 10;

static const char* DebugFilename = nullptr; // "C:\\rc\\safari.txt";

auto T = PlayerAction::bait;
auto R = PlayerAction::rock;
auto L = PlayerAction::ball;

std::vector<PlayerAction> actionByTurn = {
    T, T, L, L, L,
    T, L, L, T, L, L, L,
    T, L, L, T, L, L, L,
    T, L, L, T, L, L, L,
    T, L, L, T, L, L, L,
    T, L, L, T, L, L, L, L, R, L
};

static FILE* DebugFile = nullptr;

struct Node
{
  const Node* parent = nullptr;
  Fraction playerActionProb = Fraction::ONE;
  Fraction pokemonActionProb = Fraction::ONE;

  Fraction probConsideringParent = Fraction::ONE;

  State stateAfter;
  u8 playerActionValue = 0;
  PokemonAction pokemonAction = PokemonAction::root2;
  u8 childTurn = 0;

  // Root
  Node() = default;

  Node(const Node& parent, const Fraction& playerActionProb, const Fraction& pokemonActionProb, u8 playerActionValue, PokemonAction PokemonAction, State stateBefore) :
    playerActionProb(playerActionProb),
    pokemonActionProb(pokemonActionProb),
    stateAfter(stateBefore)
  {
    this->parent = &parent;
    this->childTurn = parent.childTurn + 1;
    this->playerActionValue = playerActionValue;
    this->pokemonAction = PokemonAction;

    this->probConsideringParent = parent.probConsideringParent;
    this->probConsideringParent.Mul(playerActionProb);
    this->probConsideringParent.Mul(pokemonActionProb);

    ApplyActionsOnStateAfter();
  }

  void ApplyActionsOnStateAfter()
  {
     this->stateAfter.ApplyPlayerAction(this->GetPlayerAction(), this->playerActionValue);

     if (!this->IsCaught())
      this->stateAfter.ApplyPokemonAction(this->pokemonAction);
  }


  Fraction GenerateChildNodes_recursive() const
  {
    if (DebugFile != nullptr)
      fprintf(DebugFile, "%s\n", DebugIdWithIndent().c_str());

    Node children[MAX_CHILD_COUNT];
    size_t childCount = 0;

    GenerateChildNodes(children, childCount);
    if (childCount == 0) // Leaf
    {
      if (this->IsCaught())
        return this->probConsideringParent;
      return Fraction::ZERO;
    }

    Fraction childrenProb[MAX_CHILD_COUNT];

    std::transform(std::execution::seq, children, children + childCount, childrenProb, [&](const auto& child)
    {
      return child.GenerateChildNodes_recursive();
    });

    Fraction childrenProbSum(0);
    for (size_t i = 0; i < childCount; i++)
      childrenProbSum.Add(childrenProb[i]);

    return childrenProbSum;
  }

  void GenerateChildNodes(Node* children, size_t& childCount) const
  {
    if (this->IsCaught() || this->Fled())
      return;

    if (this->childTurn >= actionByTurn.size())
      return;

    auto childPlayerAction = actionByTurn[this->childTurn];

    const auto& [stayProb, fleeProb] = this->stateAfter.GetStayFleeProb();

    auto AddChildren = [&](u8 playerValue, const Fraction& playerActionProb)
    {
      children[childCount++] = Node(*this, playerActionProb, fleeProb, playerValue, PokemonAction::flee, this->stateAfter);
      children[childCount++] = Node(*this, playerActionProb, stayProb, playerValue, PokemonAction::watchCarefully, this->stateAfter);
    };

    static const Fraction mod5_eq0(13108, 65536); // RandomUint16 % 5 is more likely to be 0 than 1,2,3,4
    static const Fraction mod5_eq1234(13107, 65536);
    
    static const Fraction mod5_eq1234_mul2 = mod5_eq1234.MulNew(2);
    static const Fraction mod5_eq1234_mul3 = mod5_eq1234.MulNew(3);
    static const Fraction mod5_eq1234_mul4 = mod5_eq1234.MulNew(4);

    if (childPlayerAction == PlayerAction::ball)
    {
      const auto& [catchProb, missProb] = this->stateAfter.GetCatchMissProb();

      children[childCount++] = Node(*this, catchProb, Fraction::ONE, 1, PokemonAction::caught, this->stateAfter);

      AddChildren(0, missProb);
    }
    else if (childPlayerAction == PlayerAction::bait)
    {
      if (this->stateAfter.safariBaitThrowCounter == 0)
      {
        AddChildren(2, mod5_eq0);
        AddChildren(3, mod5_eq1234);
        AddChildren(4, mod5_eq1234);
        AddChildren(5, mod5_eq1234);
        AddChildren(6, mod5_eq1234);
      }
      else if (this->stateAfter.safariBaitThrowCounter == 1)
      {
        AddChildren(3, mod5_eq0);
        AddChildren(4, mod5_eq1234);
        AddChildren(5, mod5_eq1234);
        AddChildren(6, mod5_eq1234_mul2);
      }
      else if (this->stateAfter.safariBaitThrowCounter == 2)
      {
        AddChildren(4, mod5_eq0);
        AddChildren(5, mod5_eq1234);
        AddChildren(6, mod5_eq1234_mul3);
      }
      else if (this->stateAfter.safariBaitThrowCounter == 3)
      {
        AddChildren(5, mod5_eq0);
        AddChildren(6, mod5_eq1234_mul4);
      }
      else
        AddChildren(6, Fraction::ONE);
    }

    else if (childPlayerAction == PlayerAction::rock)
    {
      if (this->stateAfter.safariRockThrowCounter == 0)
      {
        AddChildren(2, mod5_eq0);
        AddChildren(3, mod5_eq1234);
        AddChildren(4, mod5_eq1234);
        AddChildren(5, mod5_eq1234);
        AddChildren(6, mod5_eq1234);
      }
      else if (this->stateAfter.safariRockThrowCounter == 1)
      {
        AddChildren(3, mod5_eq0);
        AddChildren(4, mod5_eq1234);
        AddChildren(5, mod5_eq1234);
        AddChildren(6, mod5_eq1234_mul2);
      }
      else if (this->stateAfter.safariRockThrowCounter == 2)
      {
        AddChildren(4, mod5_eq0);
        AddChildren(5, mod5_eq1234);
        AddChildren(6, mod5_eq1234_mul3);
      }
      else if (this->stateAfter.safariRockThrowCounter == 3)
      {
        AddChildren(5, mod5_eq0);
        AddChildren(6, mod5_eq1234_mul4);
      }
      else
        AddChildren(6, Fraction::ONE);
    }
  }

  PlayerAction GetPlayerAction() const
  {
    if (this->childTurn == 0)
      return PlayerAction::root;
    return actionByTurn[this->childTurn - 1];
  }

  bool Fled() const
  {
    return this->pokemonAction == PokemonAction::flee;
  }

  bool IsCaught() const
  {
    return this->pokemonAction == PokemonAction::caught;
  }

  std::string DebugId() const
  {
    if (this->parent == nullptr)
      return "ROOT";

    std::string str = "";
    auto playerAction = GetPlayerAction();
    if (playerAction == PlayerAction::ball)
      str = playerActionValue == 0 ? "Ball_Miss" : "Ball_Catch";
    else if (playerAction == PlayerAction::bait)
      str = "Bait=>" + std::to_string(this->playerActionValue) + "";
    else if (playerAction == PlayerAction::rock)
      str = "Rock=>" + std::to_string(this->playerActionValue) + "";

    str += " (" + this->playerActionProb.ToStr() + ")";

    if (!this->IsCaught())
    {
      str += this->Fled() ? " Flee" : " Watch";
      str += " (" + this->pokemonActionProb.ToStr() + ")";

      if (!this->Fled())
      {
        str += " B" + std::to_string(this->stateAfter.safariBaitThrowCounter);

        if (playerAction != PlayerAction::rock && this->stateAfter.safariRockThrowCounter != 0)
          str += " R" + std::to_string(this->stateAfter.safariRockThrowCounter);
      }
    }

    auto probParent = this->probConsideringParent;
    str += " (Abs: " + probParent.ToStr() + ")";

    return str;
  }

  std::string DebugIdWithIndent() const
  {
    std::string str = "";
    for (int i = 0; i < this->childTurn; i++)
      str += " ";
    str += this->DebugId();
    return str;
  }
};


int main()
{
  if (DebugFilename != nullptr)
    fopen_s(&DebugFile, DebugFilename,"w");

  State::catchRate = 30;
  State::safariZoneFleeRate = 125;

  Node root;
  Fraction catchProb = root.GenerateChildNodes_recursive();

  std::cout << "Catch probability = " << catchProb.ToStr() << "\n";
}
