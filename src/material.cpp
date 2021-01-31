/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

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

#include <cassert>
#include <cstring>   // For std::memset

#include "material.h"
#include "thread.h"

using namespace std;

namespace {
  #define S(mg, eg) make_score(mg, eg)

  // Polynomial material imbalance parameters

  // One Score parameter for each pair (our piece, another of our pieces)
  constexpr Score QuadraticOurs[VARIANT_NB][PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#ifdef ANTI
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(-129, -129)                                                                                     }, // Bishop pair
    {S(-205, -205), S( -49,  -49)                                                                      }, // Pawn
    {S( -81,  -81), S( 436,  436), S( -81,  -81)                                                       }, // Knight      OUR PIECES
    {S(   0,    0), S(-204, -204), S(-328, -328), S(   0,    0)                                        }, // Bishop
    {S(-197, -197), S(-436, -436), S( -12,  -12), S(-183, -183), S( 92, 92)                            }, // Rook
    {S( 197,  197), S(  40,   40), S( 133,  133), S(-179, -179), S( 93, 93), S(-66,-66)                }, // Queen
    {S(   1,    1), S( -48,  -48), S(  98,   98), S(  36,   36), S( 82, 82), S(165,165), S(-168, -168) }  // King
    },
#endif
#ifdef ATOMIC
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef CRAZYHOUSE
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S( 983,  983)                                                                     }, // Bishop pair
    {S( 129,  129), S( -16,  -16)                                                      }, // Pawn
    {S(   6,    6), S( 151,  151), S(  0,   0)                                         }, // Knight      OUR PIECES
    {S( -66,  -66), S(  66,   66), S(-59, -59), S(  6,   6)                            }, // Bishop
    {S(-107, -107), S(   6,    6), S( 11,  11), S(107, 107), S( 137,  137)             }, // Rook
    {S(-198, -198), S(-112, -112), S( 83,  83), S(166, 166), S(-160, -160), S(-18,-18) }  // Queen
    },
#endif
#ifdef EXTINCTION
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef GRID
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECE 1
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef HORDE
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S( 13,  13)                                                                  }, // Bishop pair
    {S( -2,  -2), S(  0,   0)                                                     }, // Pawn
    {S(-65, -65), S( 66,  66), S( 15,  15)                                        }, // Knight      OUR PIECES
    {S(  0,   0), S( 81,  81), S( -2,  -2), S(  0,   0)                           }, // Bishop
    {S( 26,  26), S( 21,  21), S(-38, -38), S( 80,  80), S(-70, -70)              }, // Rook
    {S( 24,  24), S(-27, -27), S( 75,  85), S( 32,  32), S(  2,   2), S(-70, -70) }  // Queen
    },
#endif
#ifdef KOTH
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef LOSERS
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef RACE
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S(   0,    0), S(  0,   0)                                                     }, // Pawn
    {S(  57,   64), S(  0,   0), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(  0,   0), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S(  0,   0), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S(  0,   0), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef THREECHECK
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
#ifdef TWOKINGS
    {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECES
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
    },
#endif
  };
#ifdef CRAZYHOUSE
  const int QuadraticOursInHand[PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    //            OUR PIECES
    //empty pawn knight bishop rook queen
    {-148                               }, // Empty hand
    {   1,  -33                         }, // Pawn
    {  64,   34,   5                    }, // Knight      OUR PIECES
    { -17, -128, -35,     6             }, // Bishop
    {  14,  -18,  55,   -60,    76      }, // Rook
    { -22,   17,  39,   -20,    26,  -8 }  // Queen
  };
#endif

  // One Score parameter for each pair (our piece, their piece)
  constexpr Score QuadraticTheirs[VARIANT_NB][PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                               }, // Bishop pair
    {S(  33,  30)                                                                   }, // Pawn
    {S(  46,  18), S(106,  84)                                                      }, // Knight      OUR PIECE
    {S(  75,  35), S( 59,  44), S( 60,  15)                                         }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)                            }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225)               }  // Queen
    },
#ifdef ANTI
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                                     }, // Bishop pair
    {S(  55,  55)                                                                         }, // Pawn
    {S(  23,  23),  S(  27,   27)                                                         }, // Knight      OUR PIECES
    {S( -37, -37),  S(-248, -248), S( -18,  -18)                                          }, // Bishop
    {S(-109, -109), S(-628, -628), S(-145, -145), S(102, 102)                             }, // Rook
    {S(-156, -156), S(-133, -133), S( 134,  134), S( 78,  78), S( 48,  48)                }, // Queen
    {S(  22,   22), S( 155,  155), S(  84,   84), S( 49,  49), S(-49, -49), S(-104, -104) }  // King
    },
#endif
#ifdef ATOMIC
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(  33,  30)                                                     }, // Pawn
    {S(  46,  18), S(106,  84)                                        }, // Knight      OUR PIECES
    {S(  75,  35), S( 59,  44), S( 60,  15)                           }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)              }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225) }  // Queen
    },
#endif
#ifdef CRAZYHOUSE
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                        }, // Bishop pair
    {S(44, 44)                                               }, // Pawn
    {S(32, 32), S( 1,  1)                                    }, // Knight      OUR PIECES
    {S(97, 97), S(49, 49), S(12, 12)                         }, // Bishop
    {S(23, 23), S(46, 46), S( 0,  0), S(-2, -2)              }, // Rook
    {S(75, 75), S(43, 43), S(20, 20), S(65, 65), S(221, 221) }  // Queen
    },
#endif
#ifdef EXTINCTION
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(  33,  30)                                                     }, // Pawn
    {S(  46,  18), S(106,  84)                                        }, // Knight      OUR PIECES
    {S(  75,  35), S( 59,  44), S( 60,  15)                           }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)              }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225) }  // Queen
    },
#endif
#ifdef GRID
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(  33,  30)                                                     }, // Pawn
    {S(  46,  18), S(106,  84)                                        }, // Knight      OUR PIECES
    {S(  75,  35), S( 59,  44), S( 60,  15)                           }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)              }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225) }  // Queen
    },
#endif
#ifdef HORDE
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                                  }, // Bishop pair
    { S(0, 0)                                                                          }, // Pawn
    { S(0, 0), S(   0,    0)                                                           }, // Knight      OUR PIECES
    { S(0, 0), S(   0,    0), S(   0,    0)                                            }, // Bishop
    { S(0, 0), S(   0,    0), S(   0,    0), S(  0,   0)                               }, // Rook
    { S(0, 0), S(   0,    0), S(   0,    0), S(  0,   0), S(   0,    0)                }, // Queen
    { S(0, 0), S(-557, -557), S(-711, -711), S(-86, -86), S(-386, -386), S(-655, -655) }  // King
    },
#endif
#ifdef KOTH
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(  33,  30)                                                     }, // Pawn
    {S(  46,  18), S(106,  84)                                        }, // Knight      OUR PIECES
    {S(  75,  35), S( 59,  44), S( 60,  15)                           }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)              }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225) }  // Queen
    },
#endif
#ifdef LOSERS
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                    }, // Bishop pair
    {S(-132, -132)                                                       }, // Pawn
    {S(  -5,   -5), S(185, 185)                                          }, // Knight      OUR PIECES
    {S(  59,   59), S(440, 440), S(-106, -106)                           }, // Bishop
    {S( 277,  277), S( 30,  30), S(   5,    5), S( 27,  27)              }, // Rook
    {S( 217,  217), S(357, 357), S(   5,    5), S( 51,  51), S(254, 254) }  // Queen
    },
#endif
#ifdef RACE
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(   0,   0)                                                     }, // Pawn
    {S(   9,   9), S(  0,   0)                                        }, // Knight      OUR PIECES
    {S(  59,  59), S(  0,   0), S( 42,  42)                           }, // Bishop
    {S(  46,  46), S(  0,   0), S( 24,  24), S(-24, -24)              }, // Rook
    {S( 101, 101), S(  0,   0), S(-37, -37), S(141, 141), S(268, 268) }  // Queen
    },
#endif
#ifdef THREECHECK
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(  33,  30)                                                     }, // Pawn
    {S(  46,  18), S(106,  84)                                        }, // Knight      OUR PIECES
    {S(  75,  35), S( 59,  44), S( 60,  15)                           }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)              }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225) }  // Queen
    },
#endif
#ifdef TWOKINGS
    {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                 }, // Bishop pair
    {S(  33,  30)                                                     }, // Pawn
    {S(  46,  18), S(106,  84)                                        }, // Knight      OUR PIECES
    {S(  75,  35), S( 59,  44), S( 60,  15)                           }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)              }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225) }  // Queen
    },
#endif
  };
#ifdef CRAZYHOUSE
  const int QuadraticTheirsInHand[PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    //           THEIR PIECES
    //empty pawn knight bishop rook queen
    { -40                               }, // Empty hand
    {  41,   11                         }, // Pawn
    { -62,   -9,  26                    }, // Knight      OUR PIECES
    {  34,   33,  42,    88             }, // Bishop
    { -24,    0,  58,    90,   -38      }, // Rook
    {  78,    3,  46,    37,   -26,  -1 }  // Queen
  };
#endif

  #undef S

  // Endgame evaluation and scaling functions are accessed directly and not through
  // the function maps because they correspond to more than one material hash key.
  Endgame<CHESS_VARIANT, KXK>    EvaluateKXK[] = { Endgame<CHESS_VARIANT, KXK>(WHITE),    Endgame<CHESS_VARIANT, KXK>(BLACK) };
#ifdef ATOMIC
  Endgame<ATOMIC_VARIANT, KXK> EvaluateAtomicKXK[] = { Endgame<ATOMIC_VARIANT, KXK>(WHITE), Endgame<ATOMIC_VARIANT, KXK>(BLACK) };
#endif
#ifdef HELPMATE
  Endgame<HELPMATE_VARIANT, KXK> EvaluateHelpmateKXK[] = { Endgame<HELPMATE_VARIANT, KXK>(WHITE), Endgame<HELPMATE_VARIANT, KXK>(BLACK) };
#endif

  Endgame<CHESS_VARIANT, KBPsK>  ScaleKBPsK[]  = { Endgame<CHESS_VARIANT, KBPsK>(WHITE),  Endgame<CHESS_VARIANT, KBPsK>(BLACK) };
  Endgame<CHESS_VARIANT, KQKRPs> ScaleKQKRPs[] = { Endgame<CHESS_VARIANT, KQKRPs>(WHITE), Endgame<CHESS_VARIANT, KQKRPs>(BLACK) };
  Endgame<CHESS_VARIANT, KPsK>   ScaleKPsK[]   = { Endgame<CHESS_VARIANT, KPsK>(WHITE),   Endgame<CHESS_VARIANT, KPsK>(BLACK) };
  Endgame<CHESS_VARIANT, KPKP>   ScaleKPKP[]   = { Endgame<CHESS_VARIANT, KPKP>(WHITE),   Endgame<CHESS_VARIANT, KPKP>(BLACK) };

  // Helper used to detect a given material distribution
  bool is_KXK(const Position& pos, Color us) {
    return  !more_than_one(pos.pieces(~us))
          && pos.non_pawn_material(us) >= RookValueMg;
  }

#ifdef ATOMIC
  bool is_KXK_atomic(const Position& pos, Color us) {
    return  !more_than_one(pos.pieces(~us))
          && pos.non_pawn_material(us) >= RookValueMg + KnightValueMg;
  }
#endif

#ifdef HELPMATE
  bool is_KXK_helpmate(const Position& pos, Color us) {
    return   more_than_one(pos.pieces(us))
          && pos.non_pawn_material() >= RookValueMg;
  }
#endif

  bool is_KBPsK(const Position& pos, Color us) {
    return   pos.non_pawn_material(us) == BishopValueMg
          && pos.count<PAWN  >(us) >= 1;
  }

  bool is_KQKRPs(const Position& pos, Color us) {
    return  !pos.count<PAWN>(us)
          && pos.non_pawn_material(us) == QueenValueMg
          && pos.count<ROOK>(~us) == 1
          && pos.count<PAWN>(~us) >= 1;
  }


  /// imbalance() calculates the imbalance by comparing the piece count of each
  /// piece type for both colors.

  template<Color Us>
#ifdef CRAZYHOUSE
  Score imbalance(const Position& pos, const int pieceCount[][PIECE_TYPE_NB],
                  const int pieceCountInHand[][PIECE_TYPE_NB]) {
#else
  Score imbalance(const Position& pos, const int pieceCount[][PIECE_TYPE_NB]) {
#endif

    constexpr Color Them = ~Us;

    Score bonus = SCORE_ZERO;

    // Second-degree polynomial material imbalance, by Tord Romstad
    PieceType pt_max =
#ifdef ANTI
                      pos.is_anti() ? KING :
#endif
                      QUEEN;

    for (int pt1 = NO_PIECE_TYPE; pt1 <= pt_max; ++pt1)
    {
        if (!pieceCount[Us][pt1])
            continue;

        int v = QuadraticOurs[pos.variant()][pt1][pt1] * pieceCount[Us][pt1];

        for (int pt2 = NO_PIECE_TYPE; pt2 < pt1; ++pt2)
            v +=  QuadraticOurs[pos.variant()][pt1][pt2] * pieceCount[Us][pt2]
                + QuadraticTheirs[pos.variant()][pt1][pt2] * pieceCount[Them][pt2];

        bonus += pieceCount[Us][pt1] * v;
    }
#ifdef CRAZYHOUSE
    if (pos.is_house())
        for (int pt1 = NO_PIECE_TYPE; pt1 <= pt_max; ++pt1)
        {
            if (!pieceCountInHand[Us][pt1])
                continue;

            int v = 0;

            for (int pt2 = NO_PIECE_TYPE; pt2 <= pt1; ++pt2)
                v +=  QuadraticOursInHand[pt1][pt2] * pieceCountInHand[Us][pt2]
                    + QuadraticTheirsInHand[pt1][pt2] * pieceCountInHand[Them][pt2];

            bonus += make_score(pieceCountInHand[Us][pt1], pieceCountInHand[Us][pt1]) * v;
        }
#endif

    return bonus;
  }

} // namespace

namespace Material {


/// Material::probe() looks up the current position's material configuration in
/// the material hash table. It returns a pointer to the Entry if the position
/// is found. Otherwise a new Entry is computed and stored there, so we don't
/// have to recompute all when the same material configuration occurs again.

Entry* probe(const Position& pos) {

  Key key = pos.material_key();
  Entry* e = pos.this_thread()->materialTable[key];

  if (e->key == key)
      return e;

  std::memset(e, 0, sizeof(Entry));
  e->key = key;
  e->factor[WHITE] = e->factor[BLACK] = (uint8_t)SCALE_FACTOR_NORMAL;

  Value npm_w = pos.non_pawn_material(WHITE);
  Value npm_b = pos.non_pawn_material(BLACK);
  Value npm   = std::clamp(npm_w + npm_b, EndgameLimit, MidgameLimit);
#ifdef ANTI
  if (pos.is_anti())
      npm = 2 * std::min(npm_w, npm_b);
#endif

  // Map total non-pawn material into [PHASE_ENDGAME, PHASE_MIDGAME]
  e->gamePhase = Phase(((npm - EndgameLimit) * PHASE_MIDGAME) / (MidgameLimit - EndgameLimit));
#ifdef HORDE
  if (pos.is_horde())
      e->gamePhase = Phase(pos.count<PAWN>(pos.is_horde_color(WHITE) ? WHITE : BLACK) * PHASE_MIDGAME / 36);
#endif

  // Let's look if we have a specialized evaluation function for this particular
  // material configuration. Firstly we look for a fixed configuration one, then
  // for a generic one if the previous search failed.
  if ((e->evaluationFunction = Endgames::probe<Value>(key)) != nullptr)
      return e;

  switch (pos.subvariant())
  {
#ifdef ATOMIC
  case ATOMIC_VARIANT:
      for (Color c : { WHITE, BLACK })
          if (is_KXK_atomic(pos, c))
          {
              e->evaluationFunction = &EvaluateAtomicKXK[c];
              return e;
          }
  break;
#endif
#ifdef ANTIHELPMATE
  case ANTIHELPMATE_VARIANT:
  /* fall-through */
#endif
#ifdef HELPMATE
  case HELPMATE_VARIANT:
  {
      Color c = WHITE;
#ifdef ANTIHELPMATE
      c = pos.is_antihelpmate() ? BLACK : WHITE;
#endif
      if (is_KXK_helpmate(pos, c))
      {
          e->evaluationFunction = &EvaluateHelpmateKXK[c];
          return e;
      }
  }
  break;
#endif
  case CHESS_VARIANT:
  for (Color c : { WHITE, BLACK })
      if (is_KXK(pos, c))
      {
          e->evaluationFunction = &EvaluateKXK[c];
          return e;
      }
  break;
  default: break;
  }

  // OK, we didn't find any special evaluation function for the current material
  // configuration. Is there a suitable specialized scaling function?
  const auto* sf = Endgames::probe<ScaleFactor>(key);

  if (sf)
  {
      e->scalingFunction[sf->strongSide] = sf; // Only strong color assigned
      return e;
  }

  switch (pos.variant())
  {
#ifdef GRID
  case GRID_VARIANT:
      if (npm_w <= RookValueMg && npm_b <= RookValueMg)
          e->factor[WHITE] = e->factor[BLACK] = 10;
  break;
#endif
  case CHESS_VARIANT:
  // We didn't find any specialized scaling function, so fall back on generic
  // ones that refer to more than one material distribution. Note that in this
  // case we don't return after setting the function.
  for (Color c : { WHITE, BLACK })
  {
    if (is_KBPsK(pos, c))
        e->scalingFunction[c] = &ScaleKBPsK[c];

    else if (is_KQKRPs(pos, c))
        e->scalingFunction[c] = &ScaleKQKRPs[c];
  }

  if (npm_w + npm_b == VALUE_ZERO && pos.pieces(PAWN)) // Only pawns on the board
  {
      if (!pos.count<PAWN>(BLACK))
      {
          assert(pos.count<PAWN>(WHITE) >= 2);

          e->scalingFunction[WHITE] = &ScaleKPsK[WHITE];
      }
      else if (!pos.count<PAWN>(WHITE))
      {
          assert(pos.count<PAWN>(BLACK) >= 2);

          e->scalingFunction[BLACK] = &ScaleKPsK[BLACK];
      }
      else if (pos.count<PAWN>(WHITE) == 1 && pos.count<PAWN>(BLACK) == 1)
      {
          // This is a special case because we set scaling functions
          // for both colors instead of only one.
          e->scalingFunction[WHITE] = &ScaleKPKP[WHITE];
          e->scalingFunction[BLACK] = &ScaleKPKP[BLACK];
      }
  }
  /* fall-through */
  default:

  // Zero or just one pawn makes it difficult to win, even with a small material
  // advantage. This catches some trivial draws like KK, KBK and KNK and gives a
  // drawish scale factor for cases such as KRKBP and KmmKm (except for KBBKN).
  if (!pos.count<PAWN>(WHITE) && npm_w - npm_b <= BishopValueMg)
      e->factor[WHITE] = uint8_t(npm_w <  RookValueMg   ? SCALE_FACTOR_DRAW :
                                 npm_b <= BishopValueMg ? 4 : 14);

  if (!pos.count<PAWN>(BLACK) && npm_b - npm_w <= BishopValueMg)
      e->factor[BLACK] = uint8_t(npm_b <  RookValueMg   ? SCALE_FACTOR_DRAW :
                                 npm_w <= BishopValueMg ? 4 : 14);
  }

  // Evaluate the material imbalance. We use PIECE_TYPE_NONE as a place holder
  // for the bishop pair "extended piece", which allows us to be more flexible
  // in defining bishop pair bonuses.
  const int pieceCount[COLOR_NB][PIECE_TYPE_NB] = {
  { pos.count<BISHOP>(WHITE) > 1, pos.count<PAWN>(WHITE), pos.count<KNIGHT>(WHITE),
    pos.count<BISHOP>(WHITE)    , pos.count<ROOK>(WHITE), pos.count<QUEEN >(WHITE), pos.count<KING>(WHITE) },
  { pos.count<BISHOP>(BLACK) > 1, pos.count<PAWN>(BLACK), pos.count<KNIGHT>(BLACK),
    pos.count<BISHOP>(BLACK)    , pos.count<ROOK>(BLACK), pos.count<QUEEN >(BLACK), pos.count<KING>(BLACK) } };
#ifdef CRAZYHOUSE
  if (pos.is_house())
  {
      const int pieceCountInHand[COLOR_NB][PIECE_TYPE_NB] = {
      { pos.count_in_hand<ALL_PIECES>(WHITE) == 0, pos.count_in_hand<PAWN>(WHITE), pos.count_in_hand<KNIGHT>(WHITE),
        pos.count_in_hand<BISHOP>(WHITE)         , pos.count_in_hand<ROOK>(WHITE), pos.count_in_hand<QUEEN >(WHITE), pos.count_in_hand<KING>(WHITE) },
      { pos.count_in_hand<ALL_PIECES>(BLACK) == 0, pos.count_in_hand<PAWN>(BLACK), pos.count_in_hand<KNIGHT>(BLACK),
        pos.count_in_hand<BISHOP>(BLACK)         , pos.count_in_hand<ROOK>(BLACK), pos.count_in_hand<QUEEN >(BLACK), pos.count_in_hand<KING>(BLACK) } };

      e->score = (imbalance<WHITE>(pos, pieceCount, pieceCountInHand) - imbalance<BLACK>(pos, pieceCount, pieceCountInHand)) / 16;
  }
  else
      e->score = (imbalance<WHITE>(pos, pieceCount, NULL) - imbalance<BLACK>(pos, pieceCount, NULL)) / 16;
#else
  e->score = (imbalance<WHITE>(pos, pieceCount) - imbalance<BLACK>(pos, pieceCount)) / 16;
#endif
  return e;
}

} // namespace Material
