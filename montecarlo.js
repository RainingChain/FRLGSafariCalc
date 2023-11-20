
/*
Copy-paste this code in a browser.
Then execute

new Evaluator().evaluateActions_many('TTLLLTLLTLLLTLLTLLLTLLTLLLTLLTLLLTLLTLLLLRL')

[189957, '18.99%']

*/

class State {
  constructor(speciesInfo) {
    this.speciesInfo = speciesInfo;

    this.safariCatchFactor = (this.speciesInfo.catchRate * 100 / 1275) | 0;
    this.safariEscapeFactor = (this.speciesInfo.safariZoneFleeRate * 100 / 1275) | 0;
    if (this.safariEscapeFactor <= 1)
      this.safariEscapeFactor = 2;

    this.ballThrowCount = 0;
    this.safariRockThrowCounter = 0;
    this.safariBaitThrowCounter = 0;
  }
  Random() {
    return (Math.random() * 65536) | 0;
  }
  Cmd_if_random_safari_flee() {
    let safariFleeRate = 0;

    if (this.safariRockThrowCounter) {
      safariFleeRate = this.safariEscapeFactor * 2;
      if (safariFleeRate > 20)
        safariFleeRate = 20;
    }
    else if (this.safariBaitThrowCounter != 0) {
      safariFleeRate = (this.safariEscapeFactor / 4) | 0;
      if (safariFleeRate == 0)
        safariFleeRate = 1;
    }
    else
      safariFleeRate = this.safariEscapeFactor;
    safariFleeRate *= 5;

    // console.log(`safariFleeRate: ${safariFleeRate}%`);

    if ((this.Random() % 100) < safariFleeRate)
      return true; // fleeded
    else
      return false; // stayed (sAIScriptPtr += 5)
  }
  HandleAction_WatchesCarefully() {
    if (this.safariRockThrowCounter != 0) {
      --this.safariRockThrowCounter;
      if (this.safariRockThrowCounter == 0)
        this.safariCatchFactor = (this.speciesInfo.catchRate * 100 / 1275) | 0;
    }
    else {
      if (this.safariBaitThrowCounter != 0)
        --this.safariBaitThrowCounter;
    }
  }
  HandleAction_SafariZoneBallThrow() {
    this.ballThrowCount++;
    let ballMultiplier = 15;
    let catchRate = (this.safariCatchFactor * 1275 / 100);
    let odds = catchRate * ballMultiplier / 30;

    if (odds > 254)
      return true; //caught

    odds = Math.sqrt(Math.sqrt((16711680 / odds) | 0) | 0) | 0;
    odds = (1048560 / odds) | 0;

    const BALL_3_SHAKES_SUCCESS = 4;

    //const eachOdds = (odds / (2**16)) ** BALL_3_SHAKES_SUCCESS;
    //console.log(`odds: ${odds} (${(eachOdds * 100) | 0}%)`);

    let shakes;
    for (shakes = 0; shakes < BALL_3_SHAKES_SUCCESS && this.Random() < odds; shakes++);

    return shakes == BALL_3_SHAKES_SUCCESS;
  }

  HandleAction_ThrowBait() {
    this.safariBaitThrowCounter += this.Random() % 5 + 2;
    if (this.safariBaitThrowCounter > 6)
      this.safariBaitThrowCounter = 6;
    this.safariRockThrowCounter = 0;
    this.safariCatchFactor >>= 1;
    if (this.safariCatchFactor <= 2)
      this.safariCatchFactor = 3;
  }
  HandleAction_ThrowRock() {
    this.safariRockThrowCounter += this.Random() % 5 + 2;
    if (this.safariRockThrowCounter > 6)
      this.safariRockThrowCounter = 6;
    this.safariBaitThrowCounter = 0;
    this.safariCatchFactor <<= 1;
    if (this.safariCatchFactor > 20)
      this.safariCatchFactor = 20;
  }
  HandleAction(action){
    const willFlee = this.Cmd_if_random_safari_flee();

    if(action === ACTION.tryCatch){
      const caught = this.HandleAction_SafariZoneBallThrow();
      if (caught)
        return true; // caught
    } else if (action === ACTION.bait)
      this.HandleAction_ThrowBait();
    else if (action === ACTION.rock)
      this.HandleAction_ThrowRock();

    if (willFlee)
      return false; // fleed

    if (this.ballThrowCount >= 30)
      return false;

    this.HandleAction_WatchesCarefully();
    return null; // nothing
  }
}

const ACTION = {
  tryCatch: 'L',
  bait: 'T',
  rock: 'R',
};

const SPECIES_INFO = {
  chansey:{catchRate:30,safariZoneFleeRate:125},
};

class Evaluator {
  evaluateActions_once(actions){
    const state = new State(SPECIES_INFO.chansey);
    for(let i = 0; true; i++){
      const action = i < actions.length ? actions[i] : ACTION.tryCatch;
      const res = state.HandleAction(action);
      if (res !== null)
        return res;
    }
  }
  evaluateActions_many(actionStr, count=1000000){
    let actions = actionStr.split('');
    let caughtCount = 0;
    for(let i = 0; i < count; i++)
      if (this.evaluateActions_once(actions))
        caughtCount++;

    const pctStr = ('' + (caughtCount / count) * 100).slice(0, 5) + '%';
    return [caughtCount, pctStr];
  }
}
