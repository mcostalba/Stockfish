/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#include <algorithm>
#include <cassert>
#include <cstring>   // For std::memset
#include <iomanip>
#include <sstream>

#include "bitboard.h"
#include "evaluate.h"
#include "material.h"
#include "pawns.h"

namespace {

  namespace Trace {

    enum Tracing {NO_TRACE, TRACE};

    enum Term { // The first 8 entries are for PieceType
      MATERIAL = 8, IMBALANCE, MOBILITY, THREAT, PASSED, SPACE, TOTAL, TERM_NB
    };

    double scores[TERM_NB][COLOR_NB][PHASE_NB];

    double to_cp(Value v) { return double(v) / PawnValueEg; }

    void add(int idx, Color c, Score s) {
      scores[idx][c][MG] = to_cp(mg_value(s));
      scores[idx][c][EG] = to_cp(eg_value(s));
    }

    void add(int idx, Score w, Score b = SCORE_ZERO) {
      add(idx, WHITE, w); add(idx, BLACK, b);
    }

    std::ostream& operator<<(std::ostream& os, Term t) {

      if (t == MATERIAL || t == IMBALANCE || t == Term(PAWN) || t == TOTAL)
          os << "  ---   --- |   ---   --- | ";
      else
          os << std::setw(5) << scores[t][WHITE][MG] << " "
             << std::setw(5) << scores[t][WHITE][EG] << " | "
             << std::setw(5) << scores[t][BLACK][MG] << " "
             << std::setw(5) << scores[t][BLACK][EG] << " | ";

      os << std::setw(5) << scores[t][WHITE][MG] - scores[t][BLACK][MG] << " "
         << std::setw(5) << scores[t][WHITE][EG] - scores[t][BLACK][EG] << " \n";

      return os;
    }
  }

  using namespace Trace;

  // Evaluation class contains various information computed and collected
  // by the evaluation functions.
  template<Tracing T = NO_TRACE>
  class Evaluation {

  public:
    Evaluation() = delete;
    Evaluation(const Position& p) : pos(p) {};
    Evaluation& operator=(const Evaluation&) = delete;

    Value value();

  private:
    // Evaluation helpers (used when calling value())
    template<Color Us> void initialize();
    template<Color Us> Score evaluate_king();
    template<Color Us> Score evaluate_threats();
    template<Color Us> Score evaluate_passer_pawns();
    template<Color Us> Score evaluate_space();
    template<Color Us, PieceType Pt> Score evaluate_pieces();
    ScaleFactor evaluate_scale_factor(Value eg);
    Score evaluate_initiative(Value eg);

    // Data members
    const Position& pos;
    Material::Entry* me;
    Pawns::Entry* pe;
    Bitboard mobilityArea[COLOR_NB];
    Score mobility[COLOR_NB] = { SCORE_ZERO, SCORE_ZERO };

    // attackedBy[color][piece type] is a bitboard representing all squares
    // attacked by a given color and piece type (can be also ALL_PIECES).
    Bitboard attackedBy[COLOR_NB][PIECE_TYPE_NB];

    // attackedBy2[color] are the squares attacked by 2 pieces of a given color,
    // possibly via x-ray or by one pawn and one piece. Diagonal x-ray through
    // pawn or squares attacked by 2 pawns are not explicitly added.
    Bitboard attackedBy2[COLOR_NB];

    // kingRing[color] is the zone around the king which is considered
    // by the king safety evaluation. This consists of the squares directly
    // adjacent to the king, and (only for a king on its first rank) the
    // squares two ranks in front of the king. For instance, if black's king
    // is on g8, kingRing[BLACK] is a bitboard containing the squares f8, h8,
    // f7, g7, h7, f6, g6 and h6.
    Bitboard kingRing[COLOR_NB];

    // kingAttackersCount[color] is the number of pieces of the given color
    // which attack a square in the kingRing of the enemy king.
    int kingAttackersCount[COLOR_NB];

    // kingAttackersWeight[color] is the sum of the "weights" of the pieces of the
    // given color which attack a square in the kingRing of the enemy king. The
    // weights of the individual piece types are given by the elements in the
    // KingAttackWeights array.
    int kingAttackersWeight[COLOR_NB];

    // kingAdjacentZoneAttacksCount[color] is the number of attacks by the given
    // color to squares directly adjacent to the enemy king. Pieces which attack
    // more than one square are counted multiple times. For instance, if there is
    // a white knight on g5 and black's king is on g8, this white knight adds 2
    // to kingAdjacentZoneAttacksCount[WHITE].
    int kingAdjacentZoneAttacksCount[COLOR_NB];
  };

  #define V(v) Value(v)
  #define S(mg, eg) make_score(mg, eg)

  // MobilityBonus[PieceType-2][attacked] contains bonuses for middle and end game,
  // indexed by piece type and number of attacked squares in the mobility area.
  const Score MobilityBonus[VARIANT_NB][4][32] = {
    {
    { S(-75,-76), S(-57,-54), S( -9,-28), S( -2,-10), S(  6,  5), S( 14, 12), // Knights
      S( 22, 26), S( 29, 29), S( 36, 29) },
    { S(-48,-59), S(-20,-23), S( 16, -3), S( 26, 13), S( 38, 24), S( 51, 42), // Bishops
      S( 55, 54), S( 63, 57), S( 63, 65), S( 68, 73), S( 81, 78), S( 81, 86),
      S( 91, 88), S( 98, 97) },
    { S(-58,-76), S(-27,-18), S(-15, 28), S(-10, 55), S( -5, 69), S( -2, 82), // Rooks
      S(  9,112), S( 16,118), S( 30,132), S( 29,142), S( 32,155), S( 38,165),
      S( 46,166), S( 48,169), S( 58,171) },
    { S(-39,-36), S(-21,-15), S(  3,  8), S(  3, 18), S( 14, 34), S( 22, 54), // Queens
      S( 28, 61), S( 41, 73), S( 43, 79), S( 48, 92), S( 56, 94), S( 60,104),
      S( 60,113), S( 66,120), S( 67,123), S( 70,126), S( 71,133), S( 73,136),
      S( 79,140), S( 88,143), S( 88,148), S( 99,166), S(102,170), S(102,175),
      S(106,184), S(109,191), S(113,206), S(116,212) }
    },
#ifdef ANTI
    {
      { S(-150,-152), S(-112,-108), S(-18,-52), S( -4,-20), S( 12, 10), S( 30, 22), // Knights
        S(  44,  52), S(  60,  56), S( 72, 58) },
      { S(-96,-116), S(-42,-38), S( 32, -4), S( 52, 24), S( 74, 44), S(102, 84), // Bishops
        S(108, 108), S(126,116), S(130,126), S(142,140), S(158,148), S(162,172),
        S(184, 180), S(194,188) },
      { S(-112,-156), S(-50,-36), S(-22, 52), S(-10,110), S( -8,140), S( -2,162), // Rooks
        S(  16, 218), S( 28,240), S( 42,256), S( 46,286), S( 62,308), S( 64,320),
        S(  86, 330), S( 98,336), S(118,338) },
      { S(-80,-70), S(-50,-24), S(  4, 14), S(  8, 38), S( 28, 74), S( 48,110), // Queens
        S( 50,124), S( 80,152), S( 86,158), S( 94,174), S(108,188), S(112,204),
        S(120,222), S(140,232), S(144,236), S(146,244), S(150,256), S(154,260),
        S(170,266), S(188,272), S(198,280), S(216,314), S(224,316), S(226,322),
        S(236,348), S(238,354), S(246,382), S(256,398) }
    },
#endif
#ifdef ATOMIC
    {
      { S(-75,-76), S(-56,-54), S( -9,-26), S( -2,-10), S(  6,  5), S( 15, 11), // Knights
        S( 22, 26), S( 30, 28), S( 36, 29) },
      { S(-48,-58), S(-21,-19), S( 16, -2), S( 26, 12), S( 37, 22), S( 51, 42), // Bishops
        S( 54, 54), S( 63, 58), S( 65, 63), S( 71, 70), S( 79, 74), S( 81, 86),
        S( 92, 90), S( 97, 94) },
      { S(-56,-78), S(-25,-18), S(-11, 26), S( -5, 55), S( -4, 70), S( -1, 81), // Rooks
        S(  8,109), S( 14,120), S( 21,128), S( 23,143), S( 31,154), S( 32,160),
        S( 43,165), S( 49,168), S( 59,169) },
      { S(-40,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 55), // Queens
        S( 25, 62), S( 40, 76), S( 43, 79), S( 47, 87), S( 54, 94), S( 56,102),
        S( 60,111), S( 70,116), S( 72,118), S( 73,122), S( 75,128), S( 77,130),
        S( 85,133), S( 94,136), S( 99,140), S(108,157), S(112,158), S(113,161),
        S(118,174), S(119,177), S(123,191), S(128,199) }
    },
#endif
#ifdef CRAZYHOUSE
    {
      { S(-115,-112), S(-94,-51), S(-90,-24), S(-38, -5), S(  6,  5), S( 15, 11), // Knights
        S(  22,  26), S( 30, 28), S( 36, 29) },
      { S(-150, -63), S(-91,-41), S( 16, -8), S( 26, 12), S( 37, 22), S( 51, 42), // Bishops
        S(  54,  54), S( 63, 58), S( 65, 63), S( 71, 70), S( 79, 74), S( 81, 86),
        S(  92,  90), S( 97, 94) },
      { S( -53, -53), S(-22, -8), S(-48, 30), S(-14, 57), S( -4, 77), S( 11, 87), // Rooks
        S(   7, 115), S( 12,123), S( 27,120), S(  6,140), S( 55,156), S( 18,161),
        S(  51, 161), S( 54,171), S( 52,166) },
      { S( -26, -56), S(-24,-14), S(  7, 14), S(  8, 15), S( 18, 34), S( 14, 41), // Queens
        S(  28,  58), S( 33, 66), S( 40, 70), S( 47, 74), S( 50,100), S( 52,106),
        S(  59, 111), S( 50, 95), S( 60,115), S( 61,126), S( 75,144), S( 82,119),
        S(  95, 137), S(102,138), S(100,142), S(119,154), S(129,156), S(107,156),
        S( 111, 177), S(115,181), S(124,197), S(124,199) }
    },
#endif
#ifdef HORDE
    {
      { S(-126,-90), S( -7,-22), S( -46,-25), S( 19,7), S( -53, 71), S( 31, -1), // Knights
        S(  -6, 51), S(-12, 47), S( -9, -56) },
      { S( -46,-2), S(30,66), S( 18, -27), S( 86, 21), S( 65, 11), S(147, 45), // Bishops
        S(  98, 38), S( 95, 52), S(122, 45), S( 95, 33), S( 89,103), S( 85, -9),
        S( 105, 70), S(131, 82) },
      { S( -56,-78), S(-25,-18), S(-11, 26), S( -5, 55), S( -4, 70), S( -1, 81), // Rooks
        S(   8,109), S( 14,120), S( 21,128), S( 23,143), S( 31,154), S( 32,160),
        S(  43,165), S( 49,168), S( 59,169) },
      { S( -40,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 55), // Queens
        S(  25, 62), S( 40, 76), S( 43, 79), S( 47, 87), S( 54, 94), S( 56,102),
        S(  60,111), S( 70,116), S( 72,118), S( 73,122), S( 75,128), S( 77,130),
        S(  85,133), S( 94,136), S( 99,140), S(108,157), S(112,158), S(113,161),
        S( 118,174), S(119,177), S(123,191), S(128,199) }
    },
#endif
#ifdef KOTH
    {
      { S(-75,-76), S(-56,-54), S( -9,-26), S( -2,-10), S(  6,  5), S( 15, 11), // Knights
        S( 22, 26), S( 30, 28), S( 36, 29) },
      { S(-48,-58), S(-21,-19), S( 16, -2), S( 26, 12), S( 37, 22), S( 51, 42), // Bishops
        S( 54, 54), S( 63, 58), S( 65, 63), S( 71, 70), S( 79, 74), S( 81, 86),
        S( 92, 90), S( 97, 94) },
      { S(-56,-78), S(-25,-18), S(-11, 26), S( -5, 55), S( -4, 70), S( -1, 81), // Rooks
        S(  8,109), S( 14,120), S( 21,128), S( 23,143), S( 31,154), S( 32,160),
        S( 43,165), S( 49,168), S( 59,169) },
      { S(-40,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 55), // Queens
        S( 25, 62), S( 40, 76), S( 43, 79), S( 47, 87), S( 54, 94), S( 56,102),
        S( 60,111), S( 70,116), S( 72,118), S( 73,122), S( 75,128), S( 77,130),
        S( 85,133), S( 94,136), S( 99,140), S(108,157), S(112,158), S(113,161),
        S(118,174), S(119,177), S(123,191), S(128,199) }
    },
#endif
#ifdef LOSERS
    {
      { S(-75,-76), S(-56,-54), S( -9,-26), S( -2,-10), S(  6,  5), S( 15, 11), // Knights
        S( 22, 26), S( 30, 28), S( 36, 29) },
      { S(-48,-58), S(-21,-19), S( 16, -2), S( 26, 12), S( 37, 22), S( 51, 42), // Bishops
        S( 54, 54), S( 63, 58), S( 65, 63), S( 71, 70), S( 79, 74), S( 81, 86),
        S( 92, 90), S( 97, 94) },
      { S(-56,-78), S(-25,-18), S(-11, 26), S( -5, 55), S( -4, 70), S( -1, 81), // Rooks
        S(  8,109), S( 14,120), S( 21,128), S( 23,143), S( 31,154), S( 32,160),
        S( 43,165), S( 49,168), S( 59,169) },
      { S(-40,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 55), // Queens
        S( 25, 62), S( 40, 76), S( 43, 79), S( 47, 87), S( 54, 94), S( 56,102),
        S( 60,111), S( 70,116), S( 72,118), S( 73,122), S( 75,128), S( 77,130),
        S( 85,133), S( 94,136), S( 99,140), S(108,157), S(112,158), S(113,161),
        S(118,174), S(119,177), S(123,191), S(128,199) }
    },
#endif
#ifdef RACE
    {
      { S(-150,-152), S(-112,-108), S(-18,-52), S( -4,-20), S( 12, 10), S( 30, 22), // Knights
        S(  44,  52), S(  60,  56), S( 72, 58) },
      { S( -96,-116), S( -42, -38), S( 32, -4), S( 52, 24), S( 74, 44), S(102, 84), // Bishops
        S( 108, 108), S( 126, 116), S(130,126), S(142,140), S(158,148), S(162,172),
        S( 184, 180), S( 194, 188) },
      { S(-112,-156), S( -50, -36), S(-22, 52), S(-10,110), S( -8,140), S( -2,162), // Rooks
        S(  16, 218), S(  28, 240), S( 42,256), S( 46,286), S( 62,308), S( 64,320),
        S(  86, 330), S(  98, 336), S(118,338) },
      { S( -80, -70), S( -50, -24), S(  4, 14), S(  8, 38), S( 28, 74), S( 48,110), // Queens
        S(  50, 124), S(  80, 152), S( 86,158), S( 94,174), S(108,188), S(112,204),
        S( 120, 222), S( 140, 232), S(144,236), S(146,244), S(150,256), S(154,260),
        S( 170, 266), S( 188, 272), S(198,280), S(216,314), S(224,316), S(226,322),
        S( 236, 348), S( 238, 354), S(246,382), S(256,398) }
    },
#endif
#ifdef RELAY
    {
      { S(-75,-76), S(-56,-54), S( -9,-26), S( -2,-10), S(  6,  5), S( 15, 11), // Knights
        S( 22, 26), S( 30, 28), S( 36, 29) },
      { S(-48,-58), S(-21,-19), S( 16, -2), S( 26, 12), S( 37, 22), S( 51, 42), // Bishops
        S( 54, 54), S( 63, 58), S( 65, 63), S( 71, 70), S( 79, 74), S( 81, 86),
        S( 92, 90), S( 97, 94) },
      { S(-56,-78), S(-25,-18), S(-11, 26), S( -5, 55), S( -4, 70), S( -1, 81), // Rooks
        S(  8,109), S( 14,120), S( 21,128), S( 23,143), S( 31,154), S( 32,160),
        S( 43,165), S( 49,168), S( 59,169) },
      { S(-40,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 55), // Queens
        S( 25, 62), S( 40, 76), S( 43, 79), S( 47, 87), S( 54, 94), S( 56,102),
        S( 60,111), S( 70,116), S( 72,118), S( 73,122), S( 75,128), S( 77,130),
        S( 85,133), S( 94,136), S( 99,140), S(108,157), S(112,158), S(113,161),
        S(118,174), S(119,177), S(123,191), S(128,199) }
    },
#endif
#ifdef THREECHECK
    {
      { S(-74,-76), S(-55,-54), S( -9,-26), S( -2,-10), S(  6,  5), S( 15, 11), // Knights
        S( 22, 26), S( 31, 27), S( 37, 29) },
      { S(-49,-56), S(-23,-18), S( 15, -2), S( 25, 12), S( 36, 22), S( 50, 42), // Bishops
        S( 53, 54), S( 64, 57), S( 67, 63), S( 71, 68), S( 84, 76), S( 79, 87),
        S( 95, 91), S( 98, 93) },
      { S(-57,-76), S(-25,-18), S(-11, 25), S( -5, 53), S( -4, 70), S( -1, 78), // Rooks
        S(  8,111), S( 14,116), S( 22,125), S( 24,148), S( 31,159), S( 31,173),
        S( 44,163), S( 50,162), S( 56,168) },
      { S(-42,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 53), // Queens
        S( 26, 63), S( 39, 80), S( 42, 77), S( 48, 88), S( 53, 96), S( 57, 96),
        S( 61,108), S( 71,116), S( 70,116), S( 74,125), S( 75,133), S( 78,133),
        S( 85,137), S( 97,135), S(103,141), S(107,165), S(109,153), S(115,162),
        S(119,164), S(121,184), S(121,192), S(131,203) }
    },
#endif
  };

  // Outpost[knight/bishop][supported by pawn] contains bonuses for minor
  // pieces if they can reach an outpost square, bigger if that square is
  // supported by a pawn. If the minor piece occupies an outpost square
  // then score is doubled.
  const Score Outpost[][2] = {
    { S(22, 6), S(33, 9) }, // Knight
    { S( 9, 2), S(14, 4) }  // Bishop
  };

  // RookOnFile[semiopen/open] contains bonuses for each rook when there is no
  // friendly pawn on the rook file.
  const Score RookOnFile[] = { S(20, 7), S(45, 20) };

  // ThreatByMinor/ByRook[attacked PieceType] contains bonuses according to
  // which piece type attacks which one. Attacks on lesser pieces which are
  // pawn-defended are not considered.
  const Score ThreatByMinor[PIECE_TYPE_NB] = {
    S(0, 0), S(0, 33), S(45, 43), S(46, 47), S(72, 107), S(48, 118)
  };

  const Score ThreatByRook[PIECE_TYPE_NB] = {
    S(0, 0), S(0, 25), S(40, 62), S(40, 59), S(0, 34), S(35, 48)
  };

  // ThreatByKing[on one/on many] contains bonuses for king attacks on
  // pawns or pieces which are not pawn-defended.
  const Score ThreatByKing[] = { S(3, 62), S(9, 138) };

  // Passed[variant][mg/eg][Rank] contains midgame and endgame bonuses for passed pawns.
  // We don't use a Score because we process the two components independently.
  const Value Passed[VARIANT_NB][2][RANK_NB] = {
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#ifdef ANTI
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
#ifdef ATOMIC
    {
      { V(95), V(118), V(94), V(142), V(196), V(204) },
      { V(86), V( 43), V(61), V( 62), V(150), V(256) }
    },
#endif
#ifdef CRAZYHOUSE
    {
      { V(15), V(23), V(13), V( 88), V(177), V(229) },
      { V(27), V(13), V(19), V(111), V(140), V(203) }
    },
#endif
#ifdef HORDE
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
#ifdef KOTH
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
#ifdef LOSERS
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
#ifdef RACE
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
#ifdef RELAY
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
#ifdef THREECHECK
    {
      { V(5), V( 5), V(31), V(73), V(166), V(252) },
      { V(7), V(14), V(38), V(73), V(166), V(252) }
    },
#endif
  };

#ifdef THREECHECK
  const Score ChecksGivenBonus[CHECKS_NB] = {
      S(0, 0),
      S(444, 181),
      S(2425, 603),
      S(0, 0)
  };
#endif

#ifdef KOTH
  const Score KothDistanceBonus[6] = {
    S(1949, 1934), S(454, 364), S(151, 158), S(75, 85), S(42, 49), S(0, 0)
  };
  const Score KothSafeCenter = S(163, 207);
#endif

#ifdef ANTI
  const Score PieceCountAnti    = S(119, 123);
  const Score ThreatsAnti[]     = { S(192, 203), S(411, 322) };
  const Score AttacksAnti[2][2][PIECE_TYPE_NB] = {
    {
      { S( 30, 141), S( 26,  94), S(161, 105), S( 70, 123), S( 61,  72), S( 78, 12), S(139, 115) },
      { S( 56,  89), S( 82, 107), S(114,  93), S(110, 115), S(188, 112), S( 73, 59), S(122,  59) }
    },
    {
      { S(119, 142), S( 99, 105), S(123, 193), S(142,  37), S(118,  96), S( 50, 12), S( 91,  85) },
      { S( 58,  81), S( 66, 110), S(105, 153), S(100, 143), S(140, 113), S(145, 73), S(153, 154) }
    }
  };
#endif

#ifdef LOSERS
  const Score ThreatsLosers[]     = { S(216, 279), S(441, 341) };
  const Score AttacksLosers[2][2][PIECE_TYPE_NB] = {
    {
      { S( 27, 140), S( 23,  95), S(160, 112), S( 78, 129), S( 65,  75), S( 70, 13), S(146, 123) },
      { S( 58,  82), S( 80, 112), S(124,  87), S(103, 110), S(185, 107), S( 72, 60), S(126,  62) }
    },
    {
      { S(111, 127), S(102,  95), S(121, 183), S(140,  37), S(120,  99), S( 55, 11), S( 88,  93) },
      { S( 56,  69), S( 72, 124), S(109, 154), S( 98, 149), S(129, 113), S(147, 72), S(157, 152) }
    }
  };
#endif

#ifdef CRAZYHOUSE
  const int KingDangerInHand[PIECE_TYPE_NB] = {
    83, 17, 203, 64, 156, 146
  };
#endif

#ifdef RACE
  // Bonus for distance of king from 8th rank
  const Score KingRaceBonus[RANK_NB] = {
    S(14282, 14493), S(6369, 5378), S(4224, 3557), S(2633, 2219),
    S( 1614,  1456), S( 975,  885), S( 528,  502), S(   0,    0)
  };
#endif

  // PassedFile[File] contains a bonus according to the file of a passed pawn
  const Score PassedFile[FILE_NB] = {
    S(  9, 10), S( 2, 10), S( 1, -8), S(-20,-12),
    S(-20,-12), S( 1, -8), S( 2, 10), S(  9, 10)
  };

  // KingProtector[PieceType-2] contains a bonus according to distance from king
  const Score KingProtector[] = { S(-3, -5), S(-4, -3), S(-3, 0), S(-1, 1) };

  // Assorted bonuses and penalties used by evaluation
  const Score MinorBehindPawn     = S( 16,  0);
  const Score BishopPawns         = S(  8, 12);
  const Score RookOnPawn          = S(  8, 24);
  const Score TrappedRook         = S( 92,  0);
  const Score WeakQueen           = S( 50, 10);
  const Score OtherCheck          = S( 10, 10);
  const Score CloseEnemies[VARIANT_NB] = {
    S( 7,  0),
#ifdef ANTI
    S( 0,  0),
#endif
#ifdef ATOMIC
    S(17,  0),
#endif
#ifdef CRAZYHOUSE
    S(13, 20),
#endif
#ifdef HORDE
    S( 7,  0),
#endif
#ifdef KOTH
    S( 7,  0),
#endif
#ifdef LOSERS
    S( 7,  0),
#endif
#ifdef RACE
    S( 0,  0),
#endif
#ifdef RELAY
    S( 7,  0),
#endif
#ifdef THREECHECK
    S(16,  9),
#endif
  };
  const Score PawnlessFlank       = S( 20, 80);
  const Score ThreatByHangingPawn = S( 71, 61);
  const Score ThreatBySafePawn    = S(182,175);
  const Score ThreatByRank        = S( 16,  3);
  const Score Hanging             = S( 48, 27);
  const Score ThreatByPawnPush    = S( 38, 22);
  const Score HinderPassedPawn    = S(  7,  0);

  // Penalty for a bishop on a1/h1 (a8/h8 for black) which is trapped by
  // a friendly pawn on b2/g2 (b7/g7 for black). This can obviously only
  // happen in Chess960 games.
  const Score TrappedBishopA1H1 = S(50, 50);

  #undef S
  #undef V

  // KingAttackWeights[PieceType] contains king attack weights by piece type
  const int KingAttackWeights[VARIANT_NB][PIECE_TYPE_NB] = {
    { 0, 0, 78, 56, 45, 11 },
#ifdef ANTI
    {},
#endif
#ifdef ATOMIC
    { 0, 0, 76, 64, 46, 11 },
#endif
#ifdef CRAZYHOUSE
    { 0, 0, 112, 97, 61, 2 },
#endif
#ifdef HORDE
    { 0, 0, 78, 56, 45, 11 },
#endif
#ifdef KOTH
    { 0, 0, 76, 48, 44, 10 },
#endif
#ifdef LOSERS
    { 0, 0, 78, 56, 45, 11 },
#endif
#ifdef RACE
    {},
#endif
#ifdef RELAY
    { 0, 0, 78, 56, 45, 11 },
#endif
#ifdef THREECHECK
    { 0, 0, 115, 64, 62, 35 },
#endif
  };

  // Per-variant king danger malus factors
  const int KingDangerParams[VARIANT_NB][7] = {
    {   102,  201,  143, -848,   -9,   40,    0 },
#ifdef ANTI
    {   101,  235,  134, -717,  -11,   -5,    0 },
#endif
#ifdef ATOMIC
    {   274,  166,  146, -654,  -12,   -7,   29 },
#endif
#ifdef CRAZYHOUSE
    {   138,  362,  170, -595,   -9,   -1,  306 },
#endif
#ifdef HORDE
    {   101,  235,  134, -717,  -11,   -5,    0 },
#endif
#ifdef KOTH
    {    85,  229,  131, -658,   -9,   -5,    0 },
#endif
#ifdef LOSERS
    {   101,  235,  134, -717, -357,   -5,    0 },
#endif
#ifdef RACE
    {   101,  235,  134, -717,  -11,   -5,    0 },
#endif
#ifdef RELAY
    {   101,  235,  134, -717,  -11,   -5,    0 },
#endif
#ifdef THREECHECK
    {    85,  136,  106, -613,   -7,  -73,  181 },
#endif
  };

  // Penalties for enemy's safe checks
  const int QueenCheck  = 780;
  const int RookCheck   = 880;
  const int BishopCheck = 435;
  const int KnightCheck = 790;
#ifdef ATOMIC
  const int IndirectKingAttack = 883;
#endif

#ifdef THREECHECK
  // In Q8 fixed point
  const int ThreeCheckKSFactors[CHECKS_NB] = { 571, 619, 858, 0 };
#endif
  // Threshold for lazy and space evaluation
  const Value LazyThreshold  = Value(1500);
  const Value SpaceThreshold[VARIANT_NB] = {
    Value(12222),
#ifdef ANTI
    Value(12222),
#endif
#ifdef ATOMIC
    Value(12222),
#endif
#ifdef CRAZYHOUSE
    Value(12222),
#endif
#ifdef HORDE
    VALUE_ZERO,
#endif
#ifdef KOTH
    VALUE_ZERO,
#endif
#ifdef LOSERS
    Value(12222),
#endif
#ifdef RACE
    Value(12222),
#endif
#ifdef RELAY
    Value(12222),
#endif
#ifdef THREECHECK
    Value(12222),
#endif
  };


  // initialize() computes king and pawn attacks, and the king ring bitboard
  // for a given color. This is done at the beginning of the evaluation.

  template<Tracing T> template<Color Us>
  void Evaluation<T>::initialize() {

    const Color  Them = (Us == WHITE ? BLACK : WHITE);
    const Square Up   = (Us == WHITE ? NORTH : SOUTH);
    const Square Down = (Us == WHITE ? SOUTH : NORTH);
    const Bitboard LowRanks = (Us == WHITE ? Rank2BB | Rank3BB: Rank7BB | Rank6BB);

    // Find our pawns on the first two ranks, and those which are blocked
    Bitboard b = pos.pieces(Us, PAWN) & (shift<Down>(pos.pieces()) | LowRanks);

    // Squares occupied by those pawns, by our king, or controlled by enemy pawns
    // are excluded from the mobility area.
#ifdef ANTI
    if (pos.is_anti())
        mobilityArea[Us] = ~0;
    else
#endif
    mobilityArea[Us] = ~(b | pos.square<KING>(Us) | pe->pawn_attacks(Them));

    // Initialise the attack bitboards with the king and pawn information
#ifdef ANTI
    if (pos.is_anti())
    {
        attackedBy[Us][KING] = 0;
        Bitboard kings = pos.pieces(Us, KING);
        while (kings)
            attackedBy[Us][KING] |= pos.attacks_from<KING>(pop_lsb(&kings));
        b = attackedBy[Us][KING];
    }
    else
#endif
    b = attackedBy[Us][KING] = pos.attacks_from<KING>(pos.square<KING>(Us));
    attackedBy[Us][PAWN] = pe->pawn_attacks(Us);

    attackedBy2[Us]            = b & attackedBy[Us][PAWN];
    attackedBy[Us][ALL_PIECES] = b | attackedBy[Us][PAWN];

    // Init our king safety tables only if we are going to use them
    if ((
#ifdef ANTI
        !pos.is_anti() &&
#endif
        (pos.non_pawn_material(Them) >= RookValueMg + KnightValueMg))
#ifdef CRAZYHOUSE
        || pos.is_house()
#endif
    )
    {
        kingRing[Us] = b;
        if (relative_rank(Us, pos.square<KING>(Us)) == RANK_1)
            kingRing[Us] |= shift<Up>(b);

        kingAttackersCount[Them] = popcount(b & pe->pawn_attacks(Them));
        kingAdjacentZoneAttacksCount[Them] = kingAttackersWeight[Them] = 0;
    }
    else
        kingRing[Us] = kingAttackersCount[Them] = 0;
  }


  // evaluate_pieces() assigns bonuses and penalties to the pieces of a given
  // color and type.

  template<Tracing T>  template<Color Us, PieceType Pt>
  Score Evaluation<T>::evaluate_pieces() {

    const Color Them = (Us == WHITE ? BLACK : WHITE);
    const Bitboard OutpostRanks = (Us == WHITE ? Rank4BB | Rank5BB | Rank6BB
                                               : Rank5BB | Rank4BB | Rank3BB);
    const Square* pl = pos.squares<Pt>(Us);

    Bitboard b, bb;
    Square s;
    Score score = SCORE_ZERO;

    attackedBy[Us][Pt] = 0;

    while ((s = *pl++) != SQ_NONE)
    {
        // Find attacked squares, including x-ray attacks for bishops and rooks
        b = Pt == BISHOP ? attacks_bb<BISHOP>(s, pos.pieces() ^ pos.pieces(Us, QUEEN))
          : Pt ==   ROOK ? attacks_bb<  ROOK>(s, pos.pieces() ^ pos.pieces(Us, ROOK, QUEEN))
                         : pos.attacks_from<Pt>(s);

        if (pos.pinned_pieces(Us) & s)
            b &= LineBB[pos.square<KING>(Us)][s];

        attackedBy2[Us] |= attackedBy[Us][ALL_PIECES] & b;
        attackedBy[Us][ALL_PIECES] |= attackedBy[Us][Pt] |= b;

        if (b & kingRing[Them])
        {
            kingAttackersCount[Us]++;
            kingAttackersWeight[Us] += KingAttackWeights[pos.variant()][Pt];
            kingAdjacentZoneAttacksCount[Us] += popcount(b & attackedBy[Them][KING]);
        }

        int mob = popcount(b & mobilityArea[Us]);

        mobility[Us] += MobilityBonus[pos.variant()][Pt - 2][mob];

#ifdef ANTI
        if (pos.is_anti())
            continue;
#endif
#ifdef HORDE
        if (pos.is_horde() && pos.is_horde_color(Us)) {} else
#endif
        // Bonus for this piece as a king protector
        score += KingProtector[Pt - 2] * distance(s, pos.square<KING>(Us));

        if (Pt == BISHOP || Pt == KNIGHT)
        {
            // Bonus for outpost squares
            bb = OutpostRanks & ~pe->pawn_attacks_span(Them);
            if (bb & s)
                score += Outpost[Pt == BISHOP][!!(attackedBy[Us][PAWN] & s)] * 2;
            else
            {
                bb &= b & ~pos.pieces(Us);
                if (bb)
                   score += Outpost[Pt == BISHOP][!!(attackedBy[Us][PAWN] & bb)];
            }

            // Bonus when behind a pawn
            if (    relative_rank(Us, s) < RANK_5
                && (pos.pieces(PAWN) & (s + pawn_push(Us))))
                score += MinorBehindPawn;

            // Penalty for pawns on the same color square as the bishop
            if (Pt == BISHOP)
                score -= BishopPawns * pe->pawns_on_same_color_squares(Us, s);

            // An important Chess960 pattern: A cornered bishop blocked by a friendly
            // pawn diagonally in front of it is a very serious problem, especially
            // when that pawn is also blocked.
            if (   Pt == BISHOP
                && pos.is_chess960()
                && (s == relative_square(Us, SQ_A1) || s == relative_square(Us, SQ_H1)))
            {
                Square d = pawn_push(Us) + (file_of(s) == FILE_A ? EAST : WEST);
                if (pos.piece_on(s + d) == make_piece(Us, PAWN))
                    score -= !pos.empty(s + d + pawn_push(Us))                ? TrappedBishopA1H1 * 4
                            : pos.piece_on(s + d + d) == make_piece(Us, PAWN) ? TrappedBishopA1H1 * 2
                                                                              : TrappedBishopA1H1;
            }
        }

        if (Pt == ROOK)
        {
            // Bonus for aligning with enemy pawns on the same rank/file
            if (relative_rank(Us, s) >= RANK_5)
                score += RookOnPawn * popcount(pos.pieces(Them, PAWN) & PseudoAttacks[ROOK][s]);

            // Bonus when on an open or semi-open file
            if (pe->semiopen_file(Us, file_of(s)))
                score += RookOnFile[!!pe->semiopen_file(Them, file_of(s))];

            // Penalty when trapped by the king, even more if the king cannot castle
            else if (mob <= 3)
            {
                Square ksq = pos.square<KING>(Us);

                if (   ((file_of(ksq) < FILE_E) == (file_of(s) < file_of(ksq)))
                    && !pe->semiopen_side(Us, file_of(ksq), file_of(s) < file_of(ksq)))
                    score -= (TrappedRook - make_score(mob * 22, 0)) * (1 + !pos.can_castle(Us));
            }
        }

        if (Pt == QUEEN)
        {
            // Penalty if any relative pin or discovered attack against the queen
            Bitboard pinners;
            if (pos.slider_blockers(pos.pieces(Them, ROOK, BISHOP), s, pinners))
                score -= WeakQueen;
        }
    }

    if (T)
        Trace::add(Pt, Us, score);

    return score;
  }


  // evaluate_king() assigns bonuses and penalties to a king of a given color

  const Bitboard QueenSide   = FileABB | FileBBB | FileCBB | FileDBB;
  const Bitboard CenterFiles = FileCBB | FileDBB | FileEBB | FileFBB;
  const Bitboard KingSide    = FileEBB | FileFBB | FileGBB | FileHBB;

  const Bitboard KingFlank[FILE_NB] = {
    QueenSide, QueenSide, QueenSide, CenterFiles, CenterFiles, KingSide, KingSide, KingSide
  };

  template<Tracing T>  template<Color Us>
  Score Evaluation<T>::evaluate_king() {

    const Color Them    = (Us == WHITE ? BLACK : WHITE);
    const Square Up     = (Us == WHITE ? NORTH : SOUTH);
    const Bitboard Camp = (Us == WHITE ? ~Bitboard(0) ^ Rank6BB ^ Rank7BB ^ Rank8BB
                                       : ~Bitboard(0) ^ Rank1BB ^ Rank2BB ^ Rank3BB);

    const Square ksq = pos.square<KING>(Us);
    Bitboard undefended, b, b1, b2, safe, other;
    int kingDanger;

    // King shelter and enemy pawns storm
    Score score = pe->king_safety<Us>(pos, ksq);

    // Main king safety evaluation
    if (kingAttackersCount[Them] > (1 - pos.count<QUEEN>(Them))
#ifdef HORDE
        // Hack to prevent segmentation fault for multi-queen positions
        && !(pos.is_horde() && ksq == SQ_NONE)
#endif
    )
    {
        // Find the attacked squares which are defended only by our king...
#ifdef ATOMIC
        if (pos.is_atomic())
            undefended =   (attackedBy[Them][ALL_PIECES]
                            | (pos.pieces(Them) ^ pos.pieces(Them, KING)))
                        &  attackedBy[Us][KING];
        else
#endif
        undefended =   attackedBy[Them][ALL_PIECES]
                    &  attackedBy[Us][KING]
                    & ~attackedBy2[Us];

        // ... and those which are not defended at all in the larger king ring
        b =  attackedBy[Them][ALL_PIECES] & ~attackedBy[Us][ALL_PIECES]
           & kingRing[Us] & ~pos.pieces(Them);

        // Initialize the 'kingDanger' variable, which will be transformed
        // later into a king danger score. The initial value is based on the
        // number and types of the enemy's attacking pieces, the number of
        // attacked and undefended squares around our king and the quality of
        // the pawn shelter (current 'score' value).
        const auto KDP = KingDangerParams[pos.variant()];
        kingDanger =           kingAttackersCount[Them] * kingAttackersWeight[Them]
                    + KDP[0] * kingAdjacentZoneAttacksCount[Them]
                    + KDP[1] * popcount(undefended)
                    + KDP[2] * (popcount(b) + !!pos.pinned_pieces(Us))
                    + KDP[3] * !pos.count<QUEEN>(Them)
                    + KDP[4] * mg_value(score) / 8
                    + KDP[5];
        Bitboard h = 0;

#ifdef CRAZYHOUSE
        if (pos.is_house())
        {
            kingDanger += KingDangerInHand[ALL_PIECES] * pos.count_in_hand<ALL_PIECES>(Them);
            kingDanger += KingDangerInHand[PAWN] * pos.count_in_hand<PAWN>(Them);
            kingDanger += KingDangerInHand[KNIGHT] * pos.count_in_hand<KNIGHT>(Them);
            kingDanger += KingDangerInHand[BISHOP] * pos.count_in_hand<BISHOP>(Them);
            kingDanger += KingDangerInHand[ROOK] * pos.count_in_hand<ROOK>(Them);
            kingDanger += KingDangerInHand[QUEEN] * pos.count_in_hand<QUEEN>(Them);
            h = pos.count_in_hand<QUEEN>(Them) ? undefended & ~pos.pieces() : 0;
        }
#endif

        // Analyse the safe enemy's checks which are possible on next move
        safe  = ~pos.pieces(Them);
        safe &= ~attackedBy[Us][ALL_PIECES] | (undefended & attackedBy2[Them]);
#ifdef ATOMIC
        if (pos.is_atomic())
            safe |= attackedBy[Us][KING];
#endif

        b1 = pos.attacks_from<  ROOK>(ksq);
        b2 = pos.attacks_from<BISHOP>(ksq);

        // Enemy queen safe checks
        if ((b1 | b2) & (h | attackedBy[Them][QUEEN]) & safe)
            kingDanger += QueenCheck;

        // Defended by our queen only
        Bitboard dqo =  attackedBy2[Them]
                      & ~(attackedBy2[Us] | pos.pieces(Them))
                      & attackedBy[Us][QUEEN];
        // For minors and rooks, also consider the square safe if attacked twice,
        // and only defended by our queen.
        Bitboard dropSafe = (safe | (attackedBy[Them][ALL_PIECES] & dqo)) & ~pos.pieces(Us);
        safe |=  dqo;

        // Some other potential checks are also analysed, even from squares
        // currently occupied by the opponent own pieces, as long as the square
        // is not attacked by our pawns, and is not occupied by a blocked pawn.
        other = ~(   attackedBy[Us][PAWN]
                  | (pos.pieces(Them, PAWN) & shift<Up>(pos.pieces(PAWN))));
#ifdef THREECHECK
        if (pos.is_three_check() && pos.checks_given(Them))
            other = safe = ~pos.pieces(Them);
#endif

        // Enemy rooks safe and other checks
#ifdef CRAZYHOUSE
        h = pos.is_house() && pos.count_in_hand<ROOK>(Them) ? ~pos.pieces() : 0;
#endif
        if (b1 & ((attackedBy[Them][ROOK] & safe) | (h & dropSafe)))
            kingDanger += RookCheck;

        else if (b1 & (h | attackedBy[Them][ROOK]) & other)
            score -= OtherCheck;

        // Enemy bishops safe and other checks
#ifdef CRAZYHOUSE
        h = pos.is_house() && pos.count_in_hand<BISHOP>(Them) ? ~pos.pieces() : 0;
#endif
        if (b2 & ((attackedBy[Them][BISHOP] & safe) | (h & dropSafe)))
            kingDanger += BishopCheck;

        else if (b2 & (h | attackedBy[Them][BISHOP]) & other)
            score -= OtherCheck;

        // Enemy knights safe and other checks
#ifdef CRAZYHOUSE
        h = pos.is_house() && pos.count_in_hand<KNIGHT>(Them) ? ~pos.pieces() : 0;
#endif
        Bitboard k = pos.attacks_from<KNIGHT>(ksq);
        b = k & attackedBy[Them][KNIGHT];
        if ((b & safe) | (k & h & dropSafe))
            kingDanger += KnightCheck;

        else if ((b | (k & h)) & other)
            score -= OtherCheck;

#ifdef ATOMIC
        if (pos.is_atomic())
        {
            kingDanger += IndirectKingAttack * popcount(pos.attacks_from<KING>(pos.square<KING>(Us)) & pos.pieces(Us) & attackedBy[Them][ALL_PIECES]);
            score -= make_score(100, 100) * popcount(attackedBy[Us][KING] & pos.pieces());
        }
#endif
        // Transform the kingDanger units into a Score, and substract it from the evaluation
        if (kingDanger > 0)
        {
#ifdef THREECHECK
            if (pos.is_three_check())
                kingDanger = ThreeCheckKSFactors[pos.checks_given(Them)] * kingDanger / 256;
#endif
            int v = kingDanger * kingDanger / 4096;
#ifdef CRAZYHOUSE
            if (pos.is_house() && v > QueenValueMg)
                v = QueenValueMg;
#endif
            score -= make_score(v, kingDanger / 16 + KDP[6] * v / 256);
        }
    }

    // King tropism: firstly, find squares that opponent attacks in our king flank
    File kf = file_of(ksq);
    b = attackedBy[Them][ALL_PIECES] & KingFlank[kf] & Camp;

    assert(((Us == WHITE ? b << 4 : b >> 4) & b) == 0);
    assert(popcount(Us == WHITE ? b << 4 : b >> 4) == popcount(b));

    // Secondly, add the squares which are attacked twice in that flank and
    // which are not defended by our pawns.
    b =  (Us == WHITE ? b << 4 : b >> 4)
       | (b & attackedBy2[Them] & ~attackedBy[Us][PAWN]);

    score -= CloseEnemies[pos.variant()] * popcount(b);

    // Penalty when our king is on a pawnless flank
    if (!(pos.pieces(PAWN) & KingFlank[kf]))
        score -= PawnlessFlank;

    if (T)
        Trace::add(KING, Us, score);

    return score;
  }


  // evaluate_threats() assigns bonuses according to the types of the attacking
  // and the attacked pieces.

  template<Tracing T>  template<Color Us>
  Score Evaluation<T>::evaluate_threats() {

    const Color Them        = (Us == WHITE ? BLACK      : WHITE);
    const Square Up         = (Us == WHITE ? NORTH      : SOUTH);
    const Square Left       = (Us == WHITE ? NORTH_WEST : SOUTH_EAST);
    const Square Right      = (Us == WHITE ? NORTH_EAST : SOUTH_WEST);
    const Bitboard TRank2BB = (Us == WHITE ? Rank2BB    : Rank7BB);
    const Bitboard TRank7BB = (Us == WHITE ? Rank7BB    : Rank2BB);

    Bitboard b, weak, defended, stronglyProtected, safeThreats;
    Score score = SCORE_ZERO;
#ifdef ANTI
    if (pos.is_anti())
    {
        bool weCapture = attackedBy[Us][ALL_PIECES] & pos.pieces(Them);
        bool theyCapture = attackedBy[Them][ALL_PIECES] & pos.pieces(Us);

        // Penalties for possible captures
        if (weCapture)
        {
            // Penalty if we only attack unprotected pieces
            bool theyDefended = attackedBy[Us][ALL_PIECES] & pos.pieces(Them) & attackedBy[Them][ALL_PIECES];
            for (PieceType pt = PAWN; pt <= KING; ++pt)
            {
                if (attackedBy[Us][pt] & pos.pieces(Them) & ~attackedBy2[Us])
                    score -= AttacksAnti[theyCapture][theyDefended][pt];
                else if (attackedBy[Us][pt] & pos.pieces(Them))
                    score -= AttacksAnti[theyCapture][theyDefended][NO_PIECE_TYPE];
            }
            // If both colors attack pieces, increase penalty with piece count
            if (theyCapture)
                score -= PieceCountAnti * pos.count<ALL_PIECES>(Us);
        }
        // Bonus if we threaten to force captures (ignoring possible discoveries)
        if (!weCapture || theyCapture)
        {
            b = pos.pieces(Us, PAWN);
            Bitboard pawnPushes = shift<Up>(b | (shift<Up>(b & TRank2BB) & ~pos.pieces())) & ~pos.pieces();
            Bitboard pieceMoves = (attackedBy[Us][KNIGHT] | attackedBy[Us][BISHOP] | attackedBy[Us][ROOK]
                                 | attackedBy[Us][QUEEN] | attackedBy[Us][KING]) & ~pos.pieces();
            Bitboard threats = pawnPushes | pieceMoves;
            Bitboard unprotectedPawnPushes = pawnPushes & ~attackedBy[Us][ALL_PIECES];
            Bitboard unprotectedPieceMoves = pieceMoves & ~attackedBy2[Us];
            safeThreats = unprotectedPawnPushes | unprotectedPieceMoves;

            score += ThreatsAnti[0] * popcount(attackedBy[Them][ALL_PIECES] & threats);
            score += ThreatsAnti[1] * popcount(attackedBy[Them][ALL_PIECES] & safeThreats);
        }
    }
    else
#endif
#ifdef ATOMIC
    if (pos.is_atomic())
    {
    }
    else
#endif
#ifdef LOSERS
    if (pos.is_losers())
    {
        bool weCapture = attackedBy[Us][ALL_PIECES] & pos.pieces(Them);
        bool theyCapture = attackedBy[Them][ALL_PIECES] & pos.pieces(Us);

        // Penalties for possible captures
        if (weCapture)
        {
            // Penalty if we only attack unprotected pieces
            bool theyDefended = attackedBy[Us][ALL_PIECES] & pos.pieces(Them) & attackedBy[Them][ALL_PIECES];
            for (PieceType pt = PAWN; pt <= KING; ++pt)
            {
                if (attackedBy[Us][pt] & pos.pieces(Them) & ~attackedBy2[Us])
                    score -= AttacksLosers[theyCapture][theyDefended][pt];
                else if (attackedBy[Us][pt] & pos.pieces(Them))
                    score -= AttacksLosers[theyCapture][theyDefended][NO_PIECE_TYPE];
            }
        }
        // Bonus if we threaten to force captures (ignoring possible discoveries)
        if (!weCapture || theyCapture)
        {
            b = pos.pieces(Us, PAWN);
            Bitboard pawnPushes = shift<Up>(b | (shift<Up>(b & TRank2BB) & ~pos.pieces())) & ~pos.pieces();
            Bitboard pieceMoves = (attackedBy[Us][KNIGHT] | attackedBy[Us][BISHOP] | attackedBy[Us][ROOK]
                                 | attackedBy[Us][QUEEN] | attackedBy[Us][KING]) & ~pos.pieces();
            Bitboard threats = pawnPushes | pieceMoves;
            Bitboard unprotectedPawnPushes = pawnPushes & ~attackedBy[Us][ALL_PIECES];
            Bitboard unprotectedPieceMoves = pieceMoves & ~attackedBy2[Us];
            safeThreats = unprotectedPawnPushes | unprotectedPieceMoves;

            score += ThreatsLosers[0] * popcount(attackedBy[Them][ALL_PIECES] & threats);
            score += ThreatsLosers[1] * popcount(attackedBy[Them][ALL_PIECES] & safeThreats);
        }
    }
    else
#endif
    {

    // Non-pawn enemies attacked by a pawn
    weak = (pos.pieces(Them) ^ pos.pieces(Them, PAWN)) & attackedBy[Us][PAWN];

    if (weak)
    {
        b = pos.pieces(Us, PAWN) & ( ~attackedBy[Them][ALL_PIECES]
                                    | attackedBy[Us][ALL_PIECES]);

        safeThreats = (shift<Right>(b) | shift<Left>(b)) & weak;

        score += ThreatBySafePawn * popcount(safeThreats);

        if (weak ^ safeThreats)
            score += ThreatByHangingPawn;
    }

    // Squares strongly protected by the opponent, either because they attack the
    // square with a pawn, or because they attack the square twice and we don't.
    stronglyProtected =  attackedBy[Them][PAWN]
                       | (attackedBy2[Them] & ~attackedBy2[Us]);

    // Non-pawn enemies, strongly protected
    defended =  (pos.pieces(Them) ^ pos.pieces(Them, PAWN))
              & stronglyProtected;

    // Enemies not strongly protected and under our attack
    weak =   pos.pieces(Them)
          & ~stronglyProtected
          &  attackedBy[Us][ALL_PIECES];

    // Add a bonus according to the kind of attacking pieces
    if (defended | weak)
    {
        b = (defended | weak) & (attackedBy[Us][KNIGHT] | attackedBy[Us][BISHOP]);
        while (b)
        {
            Square s = pop_lsb(&b);
            score += ThreatByMinor[type_of(pos.piece_on(s))];
            if (type_of(pos.piece_on(s)) != PAWN)
                score += ThreatByRank * (int)relative_rank(Them, s);
        }

        b = (pos.pieces(Them, QUEEN) | weak) & attackedBy[Us][ROOK];
        while (b)
        {
            Square s = pop_lsb(&b);
            score += ThreatByRook[type_of(pos.piece_on(s))];
            if (type_of(pos.piece_on(s)) != PAWN)
                score += ThreatByRank * (int)relative_rank(Them, s);
        }

        score += Hanging * popcount(weak & ~attackedBy[Them][ALL_PIECES]);

        b = weak & attackedBy[Us][KING];
        if (b)
            score += ThreatByKing[more_than_one(b)];
    }

    // Bonus if some pawns can safely push and attack an enemy piece
    b = pos.pieces(Us, PAWN) & ~TRank7BB;
    b = shift<Up>(b | (shift<Up>(b & TRank2BB) & ~pos.pieces()));

    b &=  ~pos.pieces()
        & ~attackedBy[Them][PAWN]
        & (attackedBy[Us][ALL_PIECES] | ~attackedBy[Them][ALL_PIECES]);

    b =  (shift<Left>(b) | shift<Right>(b))
       &  pos.pieces(Them)
       & ~attackedBy[Us][PAWN];

    score += ThreatByPawnPush * popcount(b);

#ifdef THREECHECK
    if (pos.is_three_check())
        score += ChecksGivenBonus[pos.checks_given(Us)];
#endif
#ifdef HORDE
    if (pos.is_horde() && pos.is_horde_color(Them))
    {
        // Add a bonus according to how close we are to breaking through the pawn wall
        if (pos.pieces(Us, ROOK) | pos.pieces(Us, QUEEN))
        {
            int min = 8;
            if ((attackedBy[Us][QUEEN] | attackedBy[Us][ROOK]) & rank_bb(RANK_1))
                min = 0;
            else
            {
                for (File f = FILE_A; f <= FILE_H; ++f)
                {
                    int pawns = popcount(pos.pieces(Them, PAWN) & file_bb(f));
                    int pawnsl = f > FILE_A ? std::min(popcount(pos.pieces(Them, PAWN) & FileBB[f - 1]), pawns) : 0;
                    int pawnsr = f < FILE_H ? std::min(popcount(pos.pieces(Them, PAWN) & FileBB[f + 1]), pawns) : 0;
                    min = std::min(min, pawnsl + pawnsr);
                }
            }
            score += ThreatByHangingPawn * pos.count<PAWN>(Them) / (1 + min) / (pos.pieces(Us, QUEEN) ? 2 : 4);
        }
    }
#endif
    }
    if (T)
        Trace::add(THREAT, Us, score);

    return score;
  }


  // evaluate_passer_pawns() evaluates the passed pawns and candidate passed
  // pawns of the given color.

  template<Tracing T>  template<Color Us>
  Score Evaluation<T>::evaluate_passer_pawns() {

    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard b, bb, squaresToQueen, defendedSquares, unsafeSquares;
    Score score = SCORE_ZERO;

#ifdef RACE
    if (pos.is_race())
    {
        Square ksq = pos.square<KING>(Us);
        int s = relative_rank(BLACK, ksq);
        for (Rank kr = rank_of(ksq), r = Rank(kr + 1); r <= RANK_8; ++r)
            if (!(rank_bb(r) & DistanceRingBB[ksq][r - 1 - kr] & ~attackedBy[Them][ALL_PIECES] & ~pos.pieces(Us)))
                s++;
        score = KingRaceBonus[std::min(s, 7)];
    }
    else
    {
#endif
    b = pe->passed_pawns(Us);

#ifdef KOTH
    if (pos.is_koth())
    {
        Square ksq = pos.square<KING>(Us);
        Square center[4] = {SQ_E4, SQ_D4, SQ_D5, SQ_E5};
        for (int i = 0; i<4; i++)
        {
            int dist = distance(ksq, center[i])
                      + popcount(pos.attackers_to(center[i]) & pos.pieces(Them))
                      + popcount(pos.pieces(Us) & center[i]) ;
            assert(dist > 0);
            score += KothDistanceBonus[std::min(dist - 1, 5)];
        }
    }
#endif
    while (b)
    {
        Square s = pop_lsb(&b);

        assert(!(pos.pieces(Them, PAWN) & forward_bb(Us, s + pawn_push(Us))));

        bb = forward_bb(Us, s) & (attackedBy[Them][ALL_PIECES] | pos.pieces(Them));
        score -= HinderPassedPawn * popcount(bb);

        int r = relative_rank(Us, s) - RANK_2;
        int rr = r * (r - 1);

        Value mbonus = Passed[pos.variant()][MG][r], ebonus = Passed[pos.variant()][EG][r];

        if (rr)
        {
            Square pawnPush = pawn_push(Us);
            Square blockSq = s + pawnPush;
#ifdef HORDE
            if (pos.is_horde())
            {
                // Assume a horde king distance of approximately 5
                if (pos.is_horde_color(Us))
                    ebonus += distance(pos.square<KING>(Them), blockSq) * 5 * rr - 10 * rr;
                else
                    ebonus += 25 * rr - distance(pos.square<KING>(Us), blockSq) * 2 * rr;
            }
            else
#endif
#ifdef ANTI
            if (pos.is_anti()) {} else
#endif
#ifdef ATOMIC
            if (pos.is_atomic())
                ebonus +=  distance(pos.square<KING>(Them), blockSq) * 5 * rr;
            else
#endif
            {
            // Adjust bonus based on the king's proximity
            ebonus +=  distance(pos.square<KING>(Them), blockSq) * 5 * rr
                     - distance(pos.square<KING>(  Us), blockSq) * 2 * rr;

            // If blockSq is not the queening square then consider also a second push
            if (relative_rank(Us, blockSq) != RANK_8)
                ebonus -= distance(pos.square<KING>(Us), blockSq + pawnPush) * rr;
            }

            // If the pawn is free to advance, then increase the bonus
            if (pos.empty(blockSq))
            {
                // If there is a rook or queen attacking/defending the pawn from behind,
                // consider all the squaresToQueen. Otherwise consider only the squares
                // in the pawn's path attacked or occupied by the enemy.
                defendedSquares = unsafeSquares = squaresToQueen = forward_bb(Us, s);

                bb = forward_bb(Them, s) & pos.pieces(ROOK, QUEEN) & pos.attacks_from<ROOK>(s);

                if (!(pos.pieces(Us) & bb))
                    defendedSquares &= attackedBy[Us][ALL_PIECES];

                if (!(pos.pieces(Them) & bb))
                    unsafeSquares &= attackedBy[Them][ALL_PIECES] | pos.pieces(Them);

                // If there aren't any enemy attacks, assign a big bonus. Otherwise
                // assign a smaller bonus if the block square isn't attacked.
                int k = !unsafeSquares ? 18 : !(unsafeSquares & blockSq) ? 8 : 0;

                // If the path to the queen is fully defended, assign a big bonus.
                // Otherwise assign a smaller bonus if the block square is defended.
                if (defendedSquares == squaresToQueen)
                    k += 6;
                else if (defendedSquares & blockSq)
                    k += 4;

                mbonus += k * rr, ebonus += k * rr;
            }
            else if (pos.pieces(Us) & blockSq)
                mbonus += rr + r * 2, ebonus += rr + r * 2;
        } // rr != 0

        // Scale down bonus for candidate passers which need more than one
        // pawn push to become passed or have a pawn in front of them.
        if (!pos.pawn_passed(Us, s + pawn_push(Us)) || (pos.pieces(PAWN) & forward_bb(Us, s)))
            mbonus /= 2, ebonus /= 2;

        score += make_score(mbonus, ebonus) + PassedFile[file_of(s)];
    }
#ifdef RACE
    }
#endif

    if (T)
        Trace::add(PASSED, Us, score);

    return score;
  }


  // evaluate_space() computes the space evaluation for a given side. The
  // space evaluation is a simple bonus based on the number of safe squares
  // available for minor pieces on the central four files on ranks 2--4. Safe
  // squares one, two or three squares behind a friendly pawn are counted
  // twice. Finally, the space bonus is multiplied by a weight. The aim is to
  // improve play on game opening.

  template<Tracing T>  template<Color Us>
  Score Evaluation<T>::evaluate_space() {

    const Color Them = (Us == WHITE ? BLACK : WHITE);
    const Bitboard SpaceMask =
      Us == WHITE ? CenterFiles & (Rank2BB | Rank3BB | Rank4BB)
                  : CenterFiles & (Rank7BB | Rank6BB | Rank5BB);

    // Find the safe squares for our pieces inside the area defined by
    // SpaceMask. A square is unsafe if it is attacked by an enemy
    // pawn, or if it is undefended and attacked by an enemy piece.
    Bitboard safe =   SpaceMask
                   & ~pos.pieces(Us, PAWN)
                   & ~attackedBy[Them][PAWN]
                   & (attackedBy[Us][ALL_PIECES] | ~attackedBy[Them][ALL_PIECES]);
#ifdef HORDE
    if (pos.is_horde())
        safe =   ~attackedBy[Them][PAWN]
               & (attackedBy[Us][ALL_PIECES] | ~attackedBy[Them][ALL_PIECES]);
#endif

    // Find all squares which are at most three squares behind some friendly pawn
    Bitboard behind = pos.pieces(Us, PAWN);
    behind |= (Us == WHITE ? behind >>  8 : behind <<  8);
    behind |= (Us == WHITE ? behind >> 16 : behind << 16);
#ifdef HORDE
    if (pos.is_horde())
        behind |= (Us == WHITE ? behind >> 24 : behind << 24);
#endif

    // Since SpaceMask[Us] is fully on our half of the board...
#ifdef HORDE
    assert(pos.is_horde() || unsigned(safe >> (Us == WHITE ? 32 : 0)) == 0);
#else
    assert(unsigned(safe >> (Us == WHITE ? 32 : 0)) == 0);
#endif

    // ...count safe + (behind & safe) with a single popcount.
    int bonus;
#ifdef HORDE
    if (pos.is_horde())
        bonus = popcount(safe) + popcount(behind & safe);
    else
#endif
    bonus = popcount((Us == WHITE ? safe << 32 : safe >> 32) | (behind & safe));
    int weight = pos.count<ALL_PIECES>(Us) - 2 * pe->open_files();
#ifdef HORDE
    if (pos.is_horde() && pos.is_horde_color(Us))
        return make_score(bonus * weight * weight / 200, 0);
#endif
#ifdef KOTH
    if (pos.is_koth())
        return make_score(bonus * weight * weight / 22, 0)
              + KothSafeCenter * popcount(safe & behind & (Rank4BB | Rank5BB) & (FileDBB | FileEBB));
#endif

    return make_score(bonus * weight * weight / 16, 0);
  }


  // evaluate_initiative() computes the initiative correction value for the
  // position, i.e., second order bonus/malus based on the known attacking/defending
  // status of the players.

  template<Tracing T>
  Score Evaluation<T>::evaluate_initiative(Value eg) {

    int kingDistance =  distance<File>(pos.square<KING>(WHITE), pos.square<KING>(BLACK))
                      - distance<Rank>(pos.square<KING>(WHITE), pos.square<KING>(BLACK));
    bool bothFlanks = (pos.pieces(PAWN) & QueenSide) && (pos.pieces(PAWN) & KingSide);

    // Compute the initiative bonus for the attacking side
    int initiative = 8 * (pe->pawn_asymmetry() + kingDistance - 17) + 12 * pos.count<PAWN>() + 16 * bothFlanks;

    // Now apply the bonus: note that we find the attacking side by extracting
    // the sign of the endgame value, and that we carefully cap the bonus so
    // that the endgame score will never change sign after the bonus.
    int v = ((eg > 0) - (eg < 0)) * std::max(initiative, -abs(eg));

    return make_score(0, v);
  }


  // evaluate_scale_factor() computes the scale factor for the winning side

  template<Tracing T>
  ScaleFactor Evaluation<T>::evaluate_scale_factor(Value eg) {

    Color strongSide = eg > VALUE_DRAW ? WHITE : BLACK;
    ScaleFactor sf = me->scale_factor(pos, strongSide);

    // If we don't already have an unusual scale factor, check for certain
    // types of endgames, and use a lower scale for those.
#ifdef ATOMIC
    if (pos.is_atomic()) {} else
#endif
    if (sf == SCALE_FACTOR_NORMAL || sf == SCALE_FACTOR_ONEPAWN)
    {
        if (pos.opposite_bishops())
        {
            // Endgame with opposite-colored bishops and no other pieces (ignoring pawns)
            // is almost a draw, in case of KBP vs KB, it is even more a draw.
            if (   pos.non_pawn_material(WHITE) == BishopValueMg
                && pos.non_pawn_material(BLACK) == BishopValueMg)
                return more_than_one(pos.pieces(PAWN)) ? ScaleFactor(31) : ScaleFactor(9);

            // Endgame with opposite-colored bishops, but also other pieces. Still
            // a bit drawish, but not as drawish as with only the two bishops.
            return ScaleFactor(46);
        }
        // Endings where weaker side can place his king in front of the opponent's
        // pawns are drawish.
        else if (    abs(eg) <= BishopValueEg
                 &&  pos.count<PAWN>(strongSide) <= 2
                 && !pos.pawn_passed(~strongSide, pos.square<KING>(~strongSide)))
            return ScaleFactor(37 + 7 * pos.count<PAWN>(strongSide));
    }
#ifdef HORDE
    if (   pos.is_horde()
        && pos.non_pawn_material(pos.is_horde_color(WHITE) ? WHITE : BLACK) >= QueenValueMg
        && !pos.is_horde_color(strongSide))
        sf = ScaleFactor(10);
#endif

    return sf;
  }


  // value() is the main function of the class. It computes the various parts of
  // the evaluation and returns the value of the position from the point of view
  // of the side to move.

  template<Tracing T>
  Value Evaluation<T>::value() {

    assert(!pos.checkers());

    if (pos.is_variant_end())
        return pos.variant_result();

    // Probe the material hash table
    me = Material::probe(pos);

    // If we have a specialized evaluation function for the current material
    // configuration, call it and return.
    if (me->specialized_eval_exists())
        return me->evaluate(pos);

    // Initialize score by reading the incrementally updated scores included in
    // the position object (material + piece square tables) and the material
    // imbalance. Score is computed internally from the white point of view.
    Score score = pos.psq_score() + me->imbalance();

    // Probe the pawn hash table
    pe = Pawns::probe(pos);
    score += pe->pawns_score();

    // Early exit if score is high
    Value v = (mg_value(score) + eg_value(score)) / 2;
    if (pos.variant() == CHESS_VARIANT)
    {
    if (abs(v) > LazyThreshold)
       return pos.side_to_move() == WHITE ? v : -v;
    }

    // Main evaluation begins here

    initialize<WHITE>();
    initialize<BLACK>();

    score += evaluate_pieces<WHITE, KNIGHT>() - evaluate_pieces<BLACK, KNIGHT>();
    score += evaluate_pieces<WHITE, BISHOP>() - evaluate_pieces<BLACK, BISHOP>();
    score += evaluate_pieces<WHITE, ROOK  >() - evaluate_pieces<BLACK, ROOK  >();
    score += evaluate_pieces<WHITE, QUEEN >() - evaluate_pieces<BLACK, QUEEN >();

    score += mobility[WHITE] - mobility[BLACK];

#ifdef ANTI
  if (pos.is_anti()) {} else
#endif
#ifdef RACE
  if (pos.is_race()) {} else
#endif
    score +=  evaluate_king<WHITE>()
            - evaluate_king<BLACK>();

    score +=  evaluate_threats<WHITE>()
            - evaluate_threats<BLACK>();

    score +=  evaluate_passer_pawns<WHITE>()
            - evaluate_passer_pawns<BLACK>();

    if (pos.non_pawn_material() >= SpaceThreshold[pos.variant()])
        score +=  evaluate_space<WHITE>()
                - evaluate_space<BLACK>();

#ifdef ANTI
  if (pos.is_anti()) {} else
#endif
#ifdef HORDE
  if (pos.is_horde()) {} else
#endif
    score += evaluate_initiative(eg_value(score));

    // Interpolate between a middlegame and a (scaled by 'sf') endgame score
    ScaleFactor sf = evaluate_scale_factor(eg_value(score));
    v =  mg_value(score) * int(me->game_phase())
       + eg_value(score) * int(PHASE_MIDGAME - me->game_phase()) * sf / SCALE_FACTOR_NORMAL;

    v /= int(PHASE_MIDGAME);

    // In case of tracing add all remaining individual evaluation terms
    if (T)
    {
        Trace::add(MATERIAL, pos.psq_score());
        Trace::add(IMBALANCE, me->imbalance());
        Trace::add(PAWN, pe->pawns_score());
        Trace::add(MOBILITY, mobility[WHITE], mobility[BLACK]);
        if (pos.non_pawn_material() >= SpaceThreshold[pos.variant()])
            Trace::add(SPACE, evaluate_space<WHITE>()
                            , evaluate_space<BLACK>());
        Trace::add(TOTAL, score);
    }

    return (pos.side_to_move() == WHITE ? v : -v) + Eval::Tempo[pos.variant()]; // Side to move point of view
  }

} // namespace


/// evaluate() is the evaluator for the outer world. It returns a static evaluation
/// of the position from the point of view of the side to move.

Value Eval::evaluate(const Position& pos)
{
   return Evaluation<>(pos).value();
}

/// trace() is like evaluate(), but instead of returning a value, it returns
/// a string (suitable for outputting to stdout) that contains the detailed
/// descriptions and values of each evaluation term. Useful for debugging.

std::string Eval::trace(const Position& pos) {

  std::memset(scores, 0, sizeof(scores));

  Value v = Evaluation<TRACE>(pos).value();
  v = pos.side_to_move() == WHITE ? v : -v; // White's point of view

  std::stringstream ss;
  ss << std::showpoint << std::noshowpos << std::fixed << std::setprecision(2)
     << "      Eval term |    White    |    Black    |    Total    \n"
     << "                |   MG    EG  |   MG    EG  |   MG    EG  \n"
     << "----------------+-------------+-------------+-------------\n"
     << "       Material | " << Term(MATERIAL)
     << "      Imbalance | " << Term(IMBALANCE)
     << "          Pawns | " << Term(PAWN)
     << "        Knights | " << Term(KNIGHT)
     << "        Bishops | " << Term(BISHOP)
     << "          Rooks | " << Term(ROOK)
     << "         Queens | " << Term(QUEEN)
     << "       Mobility | " << Term(MOBILITY)
     << "    King safety | " << Term(KING)
     << "        Threats | " << Term(THREAT)
     << "   Passed pawns | " << Term(PASSED)
     << "          Space | " << Term(SPACE)
     << "----------------+-------------+-------------+-------------\n"
     << "          Total | " << Term(TOTAL);

  ss << "\nTotal Evaluation: " << to_cp(v) << " (white side)\n";

  return ss.str();
}
