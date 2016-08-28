/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (c) 2013 Ronald de Man
  Copyright (C) 2016 Marco Costalba, Lucas Braesch

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TBPROBE_H
#define TBPROBE_H

#include <fstream>
#include <ostream>

#include "../search.h"

#include "tbprobe_ref.h"

namespace Tablebases {

enum WDLScore {
    WDLLoss       = -2, // Loss
    WDLCursedLoss = -1, // Loss, but draw under 50-move rule
    WDLDraw       =  0, // Draw
    WDLCursedWin  =  1, // Win, but draw under 50-move rule
    WDLWin        =  2, // Win

    WDLScoreNone  = -1000
};

// Possible states after a probing operation
enum ProbeState {
    FAIL              =  0, // Probe failed (missing file table)
    OK                =  1, // Probe succesful
    CHANGE_STM        = -1, // DTZ should check the other side
    ZEROING_BEST_MOVE =  2  // Best move zeroes DTZ (capture or pawn move)
};

extern int MaxCardinality;

void init(const std::string& paths);
WDLScore probe_wdl(Position& pos, ProbeState* result);
int probe_dtz(Position& pos, ProbeState* result);
bool root_probe(Position& pos, Search::RootMoves& rootMoves, Value& score);
bool root_probe_wdl(Position& pos, Search::RootMoves& rootMoves, Value& score);
void filter_root_moves(Position& pos, Search::RootMoves& rootMoves);

inline std::ostream& operator<<(std::ostream& os, const WDLScore v) {

    os << (v == WDLLoss       ? "Loss" :
           v == WDLCursedLoss ? "Cursed loss" :
           v == WDLDraw       ? "Draw" :
           v == WDLCursedWin  ? "Cursed win" :
           v == WDLWin        ? "Win" : "None");

    return os;
}

inline std::ostream& operator<<(std::ostream& os, const ProbeState v) {

    os << (v == FAIL              ? "Failed" :
           v == OK                ? "Success" :
           v == CHANGE_STM        ? "Probed opponent side" :
           v == ZEROING_BEST_MOVE ? "Best move zeroes DTZ" : "None");

    return os;
}

}

namespace TablebasesInst {

typedef Tablebases::WDLScore WDLScore;
typedef Tablebases::ProbeState ProbeState;

inline void init(const std::string& paths) {

    TablebasesRef::init(paths);
    Tablebases::init(paths);
}

inline int probe_dtz(Position& pos, ProbeState* result) {

    static Mutex mutex;

    int s1, success = *result;
    {
        std::unique_lock<Mutex> lk(mutex);
        s1 = TablebasesRef::probe_dtz(pos, &success);
    }
    int s2 = Tablebases::probe_dtz(pos, result);

    dbg_hit_on(s1 != s2 || !!success != !!(*result));

    if (s1 != s2 || !!success != !!(*result))
    {
        std::ofstream log("tb_dbg.log", std::ios::out | std::ios::app);
        if (log.is_open())
        {
            log << pos
                << "DTZ: ref = (" << s1 << ", " << !!(success)
                <<   "), new = (" << s2 << ", " << !!(*result) << std::endl;
            log.close();
        }
    }

    return s2;
}

inline WDLScore probe_wdl(Position& pos, ProbeState* result) {

    int success = *result;
    WDLScore s1 = WDLScore(TablebasesRef::probe_wdl(pos, &success));
    WDLScore s2 = Tablebases::probe_wdl(pos, result);

    dbg_hit_on(s1 != s2 || !!success != !!(*result));

    if (s1 != s2 || !!success != !!(*result))
    {
        std::ofstream log("tb_dbg.log", std::ios::out | std::ios::app);
        if (log.is_open())
        {
            log << pos
                << "WDL: ref = (" << s1 << ", " << !!(success)
                <<   "), new = (" << s2 << ", " << !!(*result) << std::endl;
            log.close();
        }
    }

    // Full test DTZ in every position where WDL is called: slow but exaustive
    TablebasesInst::probe_dtz(pos, result);

    return s2;
}

inline bool root_probe(Position& pos, Search::RootMoves& rootMoves, Value& score) {

    Value score2 = score;
    Search::RootMoves rootMoves2 = rootMoves;
    bool s1 = TablebasesRef::root_probe(pos, rootMoves2, score2);
    bool s2 = Tablebases::root_probe(pos, rootMoves, score);

    dbg_hit_on(s1 != s2 || score != score2 || rootMoves.size() != rootMoves2.size());

    ProbeState result;
    TablebasesInst::probe_dtz(pos, &result);

    return s2;
}

inline bool root_probe_wdl(Position& pos, Search::RootMoves& rootMoves, Value& score) {

    Value score2 = score;
    Search::RootMoves rootMoves2 = rootMoves;
    bool s1 = TablebasesRef::root_probe_wdl(pos, rootMoves2, score2);
    bool s2 = Tablebases::root_probe_wdl(pos, rootMoves, score);

    dbg_hit_on(s1 != s2 || score != score2 || rootMoves.size() != rootMoves2.size());

    ProbeState result;
    TablebasesInst::probe_dtz(pos, &result);

    return s2;
}

}

#endif
