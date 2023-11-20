/*
General idea:
  Create a graph of all branching possibilities and sum all catching probabilities.

  Node represents one possible way a turn can occur, after the player and pokemon perform an action.
  
  State represents the bait counter, rock counter, catch rate etc.

Graph example:
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
#include <atomic>

#include "Types.hpp"
#include "Constants.hpp"
#include "Prob.hpp"
#include "State.hpp"

// ------------- Config Start

u8 State::catchRate = 30;           // Chansey
u8 State::safariZoneFleeRate = 125; // Chansey

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

/* File where to print the graph of all nodes used for debugging. Not recommended when many actions are used, because the file size becomes enormous. */
static const char* DebugFilename = nullptr; // "C:\\rc\\safari.txt";

/* Whether to print the number of explored possibilities. This has a considerable impact on performance. */
const bool PRINT_NODE_COUNT = 0;

// ------------- Config End

static constexpr int MAX_CHILD_COUNT = 10;
static FILE* DebugFile = nullptr;

struct Node
{
  static std::atomic<size_t> NodeCount;

  /* Probably that the player action has the result of <playerActionValue>. 
     Ex: If the playerAction is bait and playerActionValue is 3, 
         playerActionProb is the probability that using bait will result in the bait count to become 3, considering previous turns.
  */
  Prob playerActionProb = Prob::ONE;
  /* Probably that the pokemon performs the action <pokemonAction> on this turn. */
  Prob pokemonActionProb = Prob::ONE;
  /* Absolute probability that the battle up to this turn follows exactly the outcome predicted by this node and all its parents.
    It is the product of all playerActionProb * pokemonActionProb of this node and all parents. */
  Prob probConsideringParents = Prob::ONE;

  /* State after performing player and pokemon action */
  State stateAfter;
  /* For bait/rock, playerActionValue is the number of bait/rock after the action.
     For ball, playerActionValue is 1 if catch, 0 if miss.*/
  u8 playerActionValue = 0;
  /* The action performed by the pokemon on this turn */
  PokemonAction pokemonAction = PokemonAction::root2;
  /** -1 for root */
  signed char turn = -1;

  /* Root */
  Node() = default;

  Node(const Node& parent, const Prob& playerActionProb, const Prob& pokemonActionProb, u8 playerActionValue, PokemonAction pokemonAction) :
    playerActionProb(playerActionProb),
    pokemonActionProb(pokemonActionProb),
    stateAfter(parent.stateAfter.ApplyActions(actionByTurn[parent.turn + 1], playerActionValue, pokemonAction)),
    playerActionValue(playerActionValue),
    pokemonAction(pokemonAction),
    turn(parent.turn + 1)
  {
    if (PRINT_NODE_COUNT)
      NodeCount++;
    this->probConsideringParents = parent.probConsideringParents;
    this->probConsideringParents.Mul(playerActionProb);
    this->probConsideringParents.Mul(pokemonActionProb);
  }

  /* Returns the absolute probability that one of the children of this node will catch the pokemon.*/
  Prob GetProbThatChildrenWillCatchPokemon() const
  {
    if (DebugFile != nullptr)
      fprintf(DebugFile, "%s\n", DebugIdWithIndent().c_str());

    Node children[MAX_CHILD_COUNT];
    size_t childCount = 0;

    GenerateChildNodes(children, childCount);

    if (childCount == 0) // Leaf
    {
      if (this->IsCaught())
        return this->probConsideringParents;
      return Prob::ZERO;
    }

    Prob childrenProbSum(0);

    // To improve performance, for early turns, calculate in parallel instead of in sequence
    if (this->turn < actionByTurn.size() / 2)
    {
      Prob childrenProb[MAX_CHILD_COUNT];
      std::transform(std::execution::par_unseq, children, children + childCount, childrenProb, [](const auto& child)
        {  return child.GetProbThatChildrenWillCatchPokemon(); });

      for (size_t i = 0; i < childCount; i++)
        childrenProbSum.Add(childrenProb[i]);
    }
    else
    {
      for (size_t i = 0; i < childCount; i++)
        childrenProbSum.Add(children[i].GetProbThatChildrenWillCatchPokemon());
    }

    return childrenProbSum;
  }

  void GenerateChildNodes(Node* children, size_t& childCount) const
  {
    if (this->IsCaught() || this->Fled())
      return;

    if (this->turn + 1 >= actionByTurn.size())
      return;

    PlayerAction childPlayerAction = actionByTurn[this->turn + 1];

    const auto& [stayProb, fleeProb] = this->stateAfter.GetStayFleeProb();

    auto AddChildren = [&](u8 playerValue, const Prob& playerActionProb)
    {
      children[childCount++] = Node(*this, playerActionProb, fleeProb, playerValue, PokemonAction::flee);
      children[childCount++] = Node(*this, playerActionProb, stayProb, playerValue, PokemonAction::watchCarefully);
    };

    static const Prob mod5_eq0(13108, 65536); // RandomUint16 % 5 is more likely to be 0 than 1,2,3,4
    static const Prob mod5_eq1234(13107, 65536);
    
    static const Prob mod5_eq1234_mul2 = mod5_eq1234.MulNew(2);
    static const Prob mod5_eq1234_mul3 = mod5_eq1234.MulNew(3);
    static const Prob mod5_eq1234_mul4 = mod5_eq1234.MulNew(4);

    if (childPlayerAction == PlayerAction::ball)
    {
      const auto& [catchProb, missProb] = this->stateAfter.GetCatchMissProb();

      children[childCount++] = Node(*this, catchProb, Prob::ONE, 1, PokemonAction::caught);

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
        AddChildren(6, Prob::ONE);
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
        AddChildren(6, Prob::ONE);
    }
  }

  PlayerAction GetPlayerAction() const
  {
    if (this->IsRoot())
      return PlayerAction::root;
    return actionByTurn[this->turn];
  }

  bool Fled() const
  {
    return this->pokemonAction == PokemonAction::flee;
  }

  bool IsCaught() const
  {
    return this->pokemonAction == PokemonAction::caught;
  }

  bool IsRoot() const
  {
    return this->turn == -1;
  }

  std::string DebugId() const
  {
    if (this->IsRoot())
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

    auto probParent = this->probConsideringParents;
    str += " (Abs: " + probParent.ToStr() + ")";

    return str;
  }

  std::string DebugIdWithIndent() const
  {
    std::string str = "";
    for (int i = 0; i < this->turn; i++)
      str += " ";
    str += this->DebugId();
    return str;
  }
};

std::atomic<size_t> Node::NodeCount = 0;

int main()
{
  if (DebugFilename != nullptr)
    fopen_s(&DebugFile, DebugFilename,"w");

  auto begin = std::chrono::steady_clock::now();

  Node root;
  Prob catchProb = root.GetProbThatChildrenWillCatchPokemon();

  std::cout << "Catch probability = " << catchProb.ToStr() << "\n";

  auto end = std::chrono::steady_clock::now();
  std::cout << "Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl; // ~500ms

  if (Node::NodeCount != 0)
    std::cout << Node::NodeCount << " possibilities explored.";
}
