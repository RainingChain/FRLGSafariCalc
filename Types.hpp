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
  /* Action for dummy root node*/
  root,
};

enum PokemonAction : char
{
  flee,
  watchCarefully,
  caught,
  /* Action for dummy root node*/
  root2,
};