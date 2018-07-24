/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#ifndef ENDGAME_H_INCLUDED
#define ENDGAME_H_INCLUDED

#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "position.h"
#include "types.h"


/// EndgameCode lists all supported endgame functions by corresponding codes

enum EndgameCode {

  EVALUATION_FUNCTIONS,
#ifdef ANTI
  RK,
  KN,
  NN,
#endif
#ifdef ATOMIC
  KQK,
  KRK,
  KBK,
  KNK,
#endif
  KNNK,  // KNN vs K
  KXK,   // Generic "mate lone king" eval
  KBNK,  // KBN vs K
  KPK,   // KP vs K
  KRKP,  // KR vs KP
  KRKB,  // KR vs KB
  KRKN,  // KR vs KN
  KQKP,  // KQ vs KP
  KQKR,  // KQ vs KR

  SCALING_FUNCTIONS,
  KBPsK,   // KB and pawns vs K
  KQKRPs,  // KQ vs KR and pawns
  KRPKR,   // KRP vs KR
  KRPKB,   // KRP vs KB
  KRPPKRP, // KRPP vs KRP
  KPsK,    // K and pawns vs K
  KBPKB,   // KBP vs KB
  KBPPKB,  // KBPP vs KB
  KBPKN,   // KBP vs KN
  KNPK,    // KNP vs K
  KNPKB,   // KNP vs KB
  KPKP     // KP vs KP
};


/// Endgame functions can be of two types depending on whether they return a
/// Value or a ScaleFactor.

template<Variant V, EndgameCode E> using
eg_type = typename std::conditional<(E < SCALING_FUNCTIONS), Value, ScaleFactor>::type;


/// Base and derived functors for endgame evaluation and scaling functions

template<typename T>
struct EndgameBase {

  explicit EndgameBase(Color c) : strongSide(c), weakSide(~c) {}
  virtual ~EndgameBase() = default;
  virtual T operator()(const Position&) const = 0;

  const Color strongSide, weakSide;
};


template<Variant V, EndgameCode E, typename T = eg_type<V, E>>
struct Endgame : public EndgameBase<T> {

  explicit Endgame(Color c) : EndgameBase<T>(c) {}
  T operator()(const Position&) const override;
};


/// The Endgames class stores the pointers to endgame evaluation and scaling
/// base objects in two std::map. We use polymorphism to invoke the actual
/// endgame function by calling its virtual operator().

class Endgames {

  template<typename T> using Ptr = std::unique_ptr<EndgameBase<T>>;
  template<typename T> using Map = std::map<Key, Ptr<T>>;

  template<typename T>
  Map<T>& map() {
    return std::get<std::is_same<T, ScaleFactor>::value>(maps);
  }

  template<Variant V, EndgameCode E, typename T = eg_type<V, E>>
  void add(const std::string& code) {

    StateInfo st;
    map<T>()[Position().set(code, WHITE, V, &st).material_key()] = Ptr<T>(new Endgame<V, E>(WHITE));
    map<T>()[Position().set(code, BLACK, V, &st).material_key()] = Ptr<T>(new Endgame<V, E>(BLACK));
  }

  std::pair<Map<Value>, Map<ScaleFactor>> maps;

public:
  Endgames() {

    add<CHESS_VARIANT, KPK>("KPvK");
    add<CHESS_VARIANT, KNNK>("KNNvK");
    add<CHESS_VARIANT, KBNK>("KBNvK");
    add<CHESS_VARIANT, KRKP>("KRvKP");
    add<CHESS_VARIANT, KRKB>("KRvKB");
    add<CHESS_VARIANT, KRKN>("KRvKN");
    add<CHESS_VARIANT, KQKP>("KQvKP");
    add<CHESS_VARIANT, KQKR>("KQvKR");

    add<CHESS_VARIANT, KNPK>("KNPvK");
    add<CHESS_VARIANT, KNPKB>("KNPvKB");
    add<CHESS_VARIANT, KRPKR>("KRPvKR");
    add<CHESS_VARIANT, KRPKB>("KRPvKB");
    add<CHESS_VARIANT, KBPKB>("KBPvKB");
    add<CHESS_VARIANT, KBPKN>("KBPvKN");
    add<CHESS_VARIANT, KBPPKB>("KBPPvKB");
    add<CHESS_VARIANT, KRPPKRP>("KRPPvKRP");

#ifdef ANTI
    add<ANTI_VARIANT, RK>("RvK");
    add<ANTI_VARIANT, KN>("KvN");
    add<ANTI_VARIANT, NN>("NvN");
#endif
#ifdef ATOMIC
    add<ATOMIC_VARIANT, KPK>("KPvK");
    add<ATOMIC_VARIANT, KNK>("KNvK");
    add<ATOMIC_VARIANT, KBK>("KBvK");
    add<ATOMIC_VARIANT, KRK>("KRvK");
    add<ATOMIC_VARIANT, KQK>("KQvK");
    add<ATOMIC_VARIANT, KNNK>("KNNvK");
#endif
  }

  template<typename T>
  const EndgameBase<T>* probe(Key key) {
    return map<T>().count(key) ? map<T>()[key].get() : nullptr;
  }
};

#endif // #ifndef ENDGAME_H_INCLUDED
