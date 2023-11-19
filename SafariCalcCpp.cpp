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

#include "Types.hpp"
#include "Constants.hpp"
#include "Fraction.hpp"
#include "State.hpp"

static const bool CLEAR_CHILDREN = true;

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
  const std::vector<PlayerAction>& actionByTurn;
  std::vector<Node> children;
  const Fraction& playerActionProb;
  const Fraction& pokemonActionProb;
  PlayerAction playerAction = PlayerAction::bait;
  u8 playerActionValue = 0;
  PokemonAction pokemonAction = PokemonAction::flee;
  State stateBefore;
  State stateAfter;
  u8 childTurn = 0;

  Node(State stateBefore, const std::vector<PlayerAction>& actionByTurn) :
    playerActionProb(Fraction::ONE),
    pokemonActionProb(Fraction::ONE),
    actionByTurn(actionByTurn),
    stateBefore(stateBefore),
    stateAfter(stateBefore)
  {
    this->childTurn = 0;
  }

  Node(const Node& parent, const Fraction& playerActionProb, const Fraction& pokemonActionProb, PlayerAction playerAction, u8 playerActionValue, PokemonAction PokemonAction, State stateBefore) :
    playerActionProb(playerActionProb),
    pokemonActionProb(pokemonActionProb),
    actionByTurn(parent.actionByTurn),
    stateBefore(stateBefore),
    stateAfter(stateBefore)
  {
    this->parent = &parent;
    this->childTurn = parent.childTurn + 1;
    this->playerAction = playerAction;
    this->playerActionValue = playerActionValue;
    this->pokemonAction = PokemonAction;

    ApplyActionsOnStateAfter();
  }

  void ApplyActionsOnStateAfter()
  {
     this->stateAfter.ApplyPlayerAction((PlayerAction)this->playerAction, this->playerActionValue);

     if (!this->stateAfter.caught)
      this->stateAfter.ApplyPokemonAction((PokemonAction)this->pokemonAction);
  }


  Fraction GenerateChildNodes_recursive()
  {
    if (DebugFile != nullptr)
      fprintf(DebugFile, "%s\n", DebugIdWithIndent().c_str());

    GenerateChildNodes();
    if (this->children.empty())
    {
      if (this->stateAfter.caught)
        return this->GetProb(true);
      return Fraction::ZERO;
    }

    Fraction prob(0);
    for (auto& child : this->children)
      prob.Add(child.GenerateChildNodes_recursive());

    if (CLEAR_CHILDREN)
    {
      this->children.clear();
      this->children.shrink_to_fit();
    }

    return prob;
  }

  void GenerateChildNodes()
  {
    if (this->stateAfter.caught || this->stateAfter.fled)
      return;

    if (this->childTurn >= this->actionByTurn.size())
      return;

    auto playerAction = this->actionByTurn[this->childTurn];

    const auto& [stayProb, fleeProb] = this->stateAfter.GetStayFleeProb();

    auto AddChildren = [&](u8 playerValue, const Fraction& playerActionProb)
    {
      this->children.emplace_back(*this, playerActionProb, fleeProb, playerAction, playerValue, PokemonAction::flee, this->stateAfter);
      this->children.emplace_back(*this, playerActionProb, stayProb, playerAction, playerValue, PokemonAction::watchCarefully, this->stateAfter);
    };

    static const Fraction mod5_eq0(13108, 65536); // RandomUint16 % 5 is more likely to be 0 than 1,2,3,4
    static const Fraction mod5_eq1234(13107, 65536);
    
    static const Fraction mod5_eq1234_mul2 = mod5_eq1234.MulNew(2);
    static const Fraction mod5_eq1234_mul3 = mod5_eq1234.MulNew(3);
    static const Fraction mod5_eq1234_mul4 = mod5_eq1234.MulNew(4);

    if (playerAction == PlayerAction::ball)
    {
      const auto& [catchProb, missProb] = this->stateAfter.GetCatchMissProb();

      this->children.emplace_back(*this, catchProb, Fraction::ONE, playerAction, 1, PokemonAction::caught, this->stateAfter);

      AddChildren(0, missProb);
    }
    else if (playerAction == PlayerAction::bait)
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

    else if (playerAction == PlayerAction::rock)
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

  Fraction GetProb(bool considerParents) const
  {
    if (!considerParents)
      return this->playerActionProb.MulNew(this->pokemonActionProb);

    Fraction val(1);
    for (const Node* node = this; node != nullptr; node = node->parent)
      val.Mul(node->GetProb(false));
    return val;
  }

  std::string DebugId()
  {
    if (this->parent == nullptr)
      return "ROOT";

    std::string str = "";
    if (this->playerAction == PlayerAction::ball)
      str = this->playerActionValue == 0 ? "Ball_Miss" : "Ball_Catch";
    else if (this->playerAction == PlayerAction::bait)
      str = "Bait" + std::to_string(this->stateBefore.safariBaitThrowCounter) + "=>" + std::to_string(this->playerActionValue) + "";
    else if (this->playerAction == PlayerAction::rock)
      str = "Rock" + std::to_string(this->stateBefore.safariRockThrowCounter) + "=>" + std::to_string(this->playerActionValue) + "";

    str += " (" + this->playerActionProb.ToStr() + ")";

    if (!this->stateAfter.caught)
    {
      str += this->stateAfter.fled ? " Flee" : " Watch";
      str += " (" + this->pokemonActionProb.ToStr() + ")";

      if (!this->stateAfter.fled)
      {
        str += " B" + std::to_string(this->stateAfter.safariBaitThrowCounter);

        if (this->playerAction != PlayerAction::rock && this->stateAfter.safariRockThrowCounter != 0)
          str += " R" + std::to_string(this->stateAfter.safariRockThrowCounter);
      }
    }

    auto probParent = this->GetProb(true);
    str += " (Abs: " + probParent.ToStr() + ")";

    return str;
  }

  std::string DebugIdWithIndent()
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

  State chanseyInitialState(30, 125);

  Node root(chanseyInitialState, actionByTurn);

  Fraction catchProb = root.GenerateChildNodes_recursive();

  std::cout << "Catch probability = " << catchProb.ToStr() << "\n";
}
