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
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../search.h"

namespace Tablebases {

enum WDLScore {
    WDLLoss       = -2, // Loss
    WDLCursedLoss = -1, // Loss, but draw under 50-move rule
    WDLDraw       =  0, // Draw
    WDLCursedWin  =  1, // Win, but draw under 50-move rule
    WDLWin        =  2, // Win
};

extern size_t MaxCardinality;

void init(const std::string& paths);
WDLScore probe_wdl(Position& pos, int* success);
bool root_probe(Position& pos, Search::RootMoves& rootMoves, Value& score);
bool root_probe_wdl(Position& pos, Search::RootMoves& rootMoves, Value& score);

}

namespace TablebasesInst {

typedef Tablebases::WDLScore WDLScore;

struct Logger {
    explicit Logger(std::string f) : fname(f) { buf.reserve(100000); }

    ~Logger() { flush(); }
    void add(WDLScore v, int success) {
        buf.push_back(unsigned(v));
        buf.push_back(unsigned(success));
        if (buf.size() > 90000)
            flush();
    }

    void flush() {
        size_t sz = buf.size();
        std::ofstream file(fname, std::ios::out | std::ios::app | std::ofstream::binary);
        file.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
        file.write(reinterpret_cast<const char*>(&buf[0]), sz * sizeof(buf[0]));
        file.close();
        buf.clear();
    }

    std::vector<unsigned> buf;
    std::string fname;
};

inline WDLScore probe_wdl(Position& pos, int* success) {

    static Logger log("log_probe_wdl.bin");

    WDLScore v = Tablebases::probe_wdl(pos, success);
    log.add(v, *success);
    return v;
}

inline bool root_probe(Position& pos, Search::RootMoves& rootMoves, Value& score) {

    bool v = Tablebases::root_probe(pos, rootMoves, score);

    std::ofstream file("log_root_probe.txt", std::ifstream::out | std::ios::app);
    if (!file.is_open())
        exit(1);

    file << "D" << int(v) << score << "\n";

    file.close();
    return v;
}

inline bool root_probe_wdl(Position& pos, Search::RootMoves& rootMoves, Value& score) {

    bool v = Tablebases::root_probe_wdl(pos, rootMoves, score);

    std::ofstream file("log_root_probe.txt", std::ifstream::out | std::ios::app);
    if (!file.is_open())
        exit(1);

    file << "W" << int(v) << score << "\n";

    file.close();
    return v;
}


}

#endif
