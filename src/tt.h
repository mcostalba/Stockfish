/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad

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

#ifndef TT_H_INCLUDED
#define TT_H_INCLUDED

#include "misc.h"
#include "types.h"

/// The TTEntry is the 10 bytes transposition table entry, defined as below:
///
/// key:        16 bit
/// generation:  6 bit
/// bound type:  2 bit
/// depth:       8 bit
/// move:       16 bit
/// value:      16 bit
/// eval value: 16 bit

struct TTEntry {

  Bound bound() const      { return (Bound)(genBound8 & 0x3); }
  Depth depth() const      { return (Depth)(depth8 + DEPTH_NONE); }
  Move move() const        { return (Move)move16; }
  Value value() const      { return (Value)value16; }
  Value eval_value() const { return (Value)evalValue16; }

private:
  friend class TranspositionTable;

  void save(uint16_t k, Value v, Bound b, Depth d, Move m, uint8_t g, Value ev) {

    key16       = k;
    genBound8   = (uint8_t)(g | b);
    depth8      = (uint8_t)(d - DEPTH_NONE);
    move16      = (uint16_t)m;
    value16     = (int16_t)v;
    evalValue16 = (int16_t)ev;
  }

  uint16_t key16;
  uint8_t genBound8, depth8;
  uint16_t move16;
  int16_t value16, evalValue16;
};


/// A TranspositionTable consists of a power of 2 number of clusters and each
/// cluster consists of ClusterSize number of TTEntry. Each non-empty entry
/// contains information of exactly one position. The size of a cluster should
/// not be bigger than a cache line size. In case it is less, it should be padded
/// to guarantee always aligned accesses.

class TranspositionTable {

  static const unsigned ClusterSize = 3;

public:
 ~TranspositionTable() { free(mem); }
  void new_search() { generation += 4; } // Lower 2 bits are used by Bound

  const TTEntry* probe(const Key key) const;
  TTEntry* first_entry(const Key key) const;
  void resize(uint64_t mbSize);
  void clear();
  void store(const Key key, Value v, Bound type, Depth d, Move m, Value statV);

private:
  uint32_t hashMask;
  TTEntry* table;
  void* mem;
  uint8_t generation; // Size must be not bigger than TTEntry::generation8
};

extern TranspositionTable TT;


/// TranspositionTable::first_entry() returns a pointer to the first entry of
/// a cluster given a position. The lowest order bits of the key are used to
/// get the index of the cluster.

inline TTEntry* TranspositionTable::first_entry(const Key key) const {

  return table + ((uint32_t)key & hashMask);
}

#endif // #ifndef TT_H_INCLUDED
