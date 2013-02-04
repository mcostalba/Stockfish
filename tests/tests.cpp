#include <iostream>
#include <string>

#include "../src/bitboard.h"
#include "../src/evaluate.h"
#include "../src/position.h"
#include "../src/search.h"
#include "../src/thread.h"
#include "../src/tt.h"
#include "../src/ucioption.h"

#include "endgame.h"

int main() {
  UCI::init(Options);
  Bitboards::init();
  Zobrist::init();
  Bitbases::init_kpk();
  Search::init();
  Eval::init();
  Threads.init();
  TT.set_size(32);

  EndgameTests();

  return 0;
}
