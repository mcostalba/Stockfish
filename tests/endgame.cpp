#include "endgame.h"

#include <stdexcept>
#include <vector>

#include "../src/misc.h"
#include "../src/position.h"
#include "../src/search.h"
#include "../src/thread.h"
#include "../src/tt.h"
#include "../src/ucioption.h"

void RunSearch(Position & pos) {
  Search::StateStackPtr st;
  Search::LimitsType limits;
  limits.depth = 6;

  Threads.start_thinking(pos, limits, std::vector<Move>(), st);
  Threads.wait_for_think_finished();
}

void RunCheckEval(Position & pos, int evalLimit) {
  RunSearch(pos);
  if (evalLimit > 0 && abs(Search::RootMoves[0].score) > evalLimit) {
    throw std::runtime_error("Failed: " + pos.fen());
  } else if (abs(Search::RootMoves[0].score) < -evalLimit) {
    throw std::runtime_error("Failed: " + pos.fen());
  }
}

void TestEndgame(const std::string & fen, int evalLimit) {
  Position pos(fen, false, Threads.main_thread()); // The root position
  RunCheckEval(pos, evalLimit);

  pos.flip();
  RunCheckEval(pos, evalLimit);
}

void Test_KBPKP() {
  // Bishop opposite color from pawn, file G, blocked pawns
  TestEndgame("8/8/5b2/8/8/4k1p1/6P1/5K2 b - - 6 133", 100);

  // Multiple pawns, still blocked, bishop opposite color
  TestEndgame("8/8/4b3/8/1p3k2/1p6/1P6/1K6 w - - 0 1", 100);

  // Bishop opposite color from pawn, file G, defending king far away
  TestEndgame("5k2/1p6/1P6/8/3K1B2/8/8/8 w - - 0 1", 100);

  // Same as above, king one square away == mate!
  TestEndgame("6k1/1p6/1P6/8/3K1B2/8/8/8 w - - 0 1", -500);

  // Bishop on same color as pawn == mate!
  TestEndgame("8/8/5b2/8/1p3k2/1p6/1P6/1K6 w - - 0 1", -500);
}

void EndgameTests() {
  Test_KBPKP();
}
