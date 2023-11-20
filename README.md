# Pokemon FireRed/LeafGreen Safari Calculator

This tool calculates the probability to catch the given Pokemon, assuming the player performs predetermined actions.

Ex: For Chansey, performing Bait, Bait, Ball, Ball, Ball has ~10% chance to catch it.
The best chance to catch Chansey, assuming the player owns 30 balls, is 18.9912% chance.

## Installation
Open .sln with Visual Studio with C++ Development Kit installed.

## Running
Modify catchRate, safariZoneFleeRate, actionByTurn for the wanted values.

## Implementation Details
All branching possibilities are explored (~287M for optimal setup). The sum of catching probabilities is performed using 128-bits precision floating points.

## Contact Me
Discord: RainingChain
