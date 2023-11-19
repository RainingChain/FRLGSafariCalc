#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned long;
using u64 = unsigned long long;

enum PlayerAction : char
{
  ball,
  bait,
  rock,
  root,
};

enum PokemonAction : char
{
  flee,
  watchCarefully,
  caught,
  root2,
};