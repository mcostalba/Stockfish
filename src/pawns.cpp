/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2019 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#include "bitboard.h"
#include "pawns.h"
#include "position.h"
#include "thread.h"

namespace {

  #define V Value
  #define S(mg, eg) make_score(mg, eg)

  // Pawn penalties
  constexpr Score Backward[VARIANT_NB] = {
    S( 9, 24),
#ifdef ANTI
    S(26, 50),
#endif
#ifdef ATOMIC
    S(35, 15),
#endif
#ifdef CRAZYHOUSE
    S(41, 19),
#endif
#ifdef EXTINCTION
    S(17, 11),
#endif
#ifdef GRID
    S(17, 11),
#endif
#ifdef HORDE
    S(78, 14),
#endif
#ifdef KOTH
    S(41, 19),
#endif
#ifdef LOSERS
    S(26, 49),
#endif
#ifdef RACE
    S(0, 0),
#endif
#ifdef THREECHECK
    S(41, 19),
#endif
#ifdef TWOKINGS
    S(17, 11),
#endif
  };
  // Doubled pawn penalty
  constexpr Score Doubled[VARIANT_NB] = {
    S(11, 56),
#ifdef ANTI
    S( 4, 51),
#endif
#ifdef ATOMIC
    S( 0,  0),
#endif
#ifdef CRAZYHOUSE
    S(13, 40),
#endif
#ifdef EXTINCTION
    S(13, 40),
#endif
#ifdef GRID
    S(13, 40),
#endif
#ifdef HORDE
    S(11, 83),
#endif
#ifdef KOTH
    S(13, 40),
#endif
#ifdef LOSERS
    S( 4, 54),
#endif
#ifdef RACE
    S( 0,  0),
#endif
#ifdef THREECHECK
    S(13, 40),
#endif
#ifdef TWOKINGS
    S(13, 40),
#endif
  };
  constexpr Score Isolated[VARIANT_NB] = {
    S( 5, 15),
#ifdef ANTI
    S(54, 69),
#endif
#ifdef ATOMIC
    S(24, 14),
#endif
#ifdef CRAZYHOUSE
    S(30, 27),
#endif
#ifdef EXTINCTION
    S(13, 16),
#endif
#ifdef GRID
    S(13, 16),
#endif
#ifdef HORDE
    S(16, 38),
#endif
#ifdef KOTH
    S(30, 27),
#endif
#ifdef LOSERS
    S(53, 69),
#endif
#ifdef RACE
    S(0, 0),
#endif
#ifdef THREECHECK
    S(30, 27),
#endif
#ifdef TWOKINGS
    S(13, 16),
#endif
  };

  // Connected pawn bonus
  constexpr int Connected[RANK_NB] = { 0, 13, 17, 24, 59, 96, 171 };

  // Strength of pawn shelter for our king by [distance from edge][rank].
  // RANK_1 = 0 is used for files where we have no pawn, or pawn is behind our king.
  constexpr Value ShelterStrength[VARIANT_NB][int(FILE_NB) / 2][RANK_NB] = {
  {
    { V( -6), V( 81), V( 93), V( 58), V( 39), V( 18), V(  25) },
    { V(-43), V( 61), V( 35), V(-49), V(-29), V(-11), V( -63) },
    { V(-10), V( 75), V( 23), V( -2), V( 32), V(  3), V( -45) },
    { V(-39), V(-13), V(-29), V(-52), V(-48), V(-67), V(-166) }
  },
#ifdef ANTI
  {},
#endif
#ifdef ATOMIC
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
#ifdef CRAZYHOUSE
  {
    { V(-48), V(138), V(80), V( 48), V( 5), V( -7), V(  9) },
    { V(-78), V(116), V(20), V( -2), V(14), V(  6), V(-36) },
    { V(-69), V( 99), V(12), V(-19), V(38), V( 22), V(-50) },
    { V( -6), V( 95), V( 9), V(  4), V(-2), V(  2), V(-37) }
  },
#endif
#ifdef EXTINCTION
  {},
#endif
#ifdef GRID
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
#ifdef HORDE
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
#ifdef KOTH
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
#ifdef LOSERS
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
#ifdef RACE
  {},
#endif
#ifdef THREECHECK
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
#ifdef TWOKINGS
  {
    { V( 7), V(76), V(84), V( 38), V( 7), V( 30), V(-19) },
    { V(-3), V(93), V(52), V(-17), V(12), V(-22), V(-35) },
    { V(-6), V(83), V(25), V(-24), V(15), V( 22), V(-39) },
    { V(11), V(83), V(19), V(  8), V(18), V(-21), V(-30) }
  },
#endif
  };

  // Danger of enemy pawns moving toward our king by [distance from edge][rank].
  // RANK_1 = 0 is used for files where the enemy has no pawn, or their pawn
  // is behind our king.
  constexpr Value UnblockedStorm[int(FILE_NB) / 2][RANK_NB] = {
    { V( 89), V(107), V(123), V(93), V(57), V( 45), V( 51) },
    { V( 44), V(-18), V(123), V(46), V(39), V( -7), V( 23) },
    { V(  4), V( 52), V(162), V(37), V( 7), V(-14), V( -2) },
    { V(-10), V(-14), V( 90), V(15), V( 2), V( -7), V(-16) }
  };

#ifdef HORDE
  constexpr Score ImbalancedHorde = S(49, 39);
#endif
  #undef S
  #undef V

  template<Color Us>
  Score evaluate(const Position& pos, Pawns::Entry* e) {

    constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
    constexpr Direction Up   = (Us == WHITE ? NORTH : SOUTH);

    Bitboard b, neighbours, stoppers, doubled, support, phalanx;
    Bitboard lever, leverPush;
    Square s;
    bool opposed, backward;
    Score score = SCORE_ZERO;
    const Square* pl = pos.squares<PAWN>(Us);

    Bitboard ourPawns   = pos.pieces(  Us, PAWN);
    Bitboard theirPawns = pos.pieces(Them, PAWN);

    e->passedPawns[Us] = e->pawnAttacksSpan[Us] = e->weakUnopposed[Us] = 0;
    e->kingSquares[Us]   = SQ_NONE;
    e->pawnAttacks[Us]   = pawn_attacks_bb<Us>(ourPawns);

#ifdef HORDE
    if (pos.is_horde() && pos.is_horde_color(Us))
    {
        int l = 0, m = 0, r = popcount(ourPawns & file_bb(FILE_A));
        for (File f1 = FILE_A; f1 <= FILE_H; ++f1)
        {
            l = m; m = r; r = popcount(ourPawns & shift<EAST>(file_bb(f1)));
            score -= ImbalancedHorde * m / (1 + l * r);
        }
    }
#endif

    // Loop through all pawns of the current color and score each pawn
    while ((s = *pl++) != SQ_NONE)
    {
        assert(pos.piece_on(s) == make_piece(Us, PAWN));

        File f = file_of(s);
        Rank r = relative_rank(Us, s);

        e->pawnAttacksSpan[Us] |= pawn_attack_span(Us, s);

        // Flag the pawn
        opposed    = theirPawns & forward_file_bb(Us, s);
        stoppers   = theirPawns & passed_pawn_span(Us, s);
        lever      = theirPawns & PawnAttacks[Us][s];
        leverPush  = theirPawns & PawnAttacks[Us][s + Up];
#ifdef HORDE
        if (pos.is_horde() && relative_rank(Us, s) == RANK_1)
            doubled = 0;
        else
#endif
        doubled    = ourPawns   & (s - Up);
        neighbours = ourPawns   & adjacent_files_bb(f);
        phalanx    = neighbours & rank_bb(s);
#ifdef HORDE
        if (pos.is_horde() && relative_rank(Us, s) == RANK_1)
            support = 0;
        else
#endif
        support    = neighbours & rank_bb(s - Up);

        // A pawn is backward when it is behind all pawns of the same color
        // on the adjacent files and cannot be safely advanced.
        backward =  !(ourPawns & pawn_attack_span(Them, s + Up))
                  && (stoppers & (leverPush | (s + Up)));

        // Passed pawns will be properly scored in evaluation because we need
        // full attack info to evaluate them. Include also not passed pawns
        // which could become passed after one or two pawn pushes when are
        // not attacked more times than defended.
        if (   !(stoppers ^ lever ^ leverPush)
            && (support || !more_than_one(lever))
            && popcount(phalanx) >= popcount(leverPush))
            e->passedPawns[Us] |= s;

        else if (stoppers == square_bb(s + Up) && r >= RANK_5)
        {
            b = shift<Up>(support) & ~theirPawns;
            while (b)
                if (!more_than_one(theirPawns & PawnAttacks[Us][pop_lsb(&b)]))
                    e->passedPawns[Us] |= s;
        }

        // Score this pawn
#ifdef HORDE
        if (pos.is_horde() && relative_rank(Us, s) == RANK_1) {} else
#endif
        if (support | phalanx)
        {
            int v = (phalanx ? 3 : 2) * Connected[r];
            v = 17 * popcount(support) + (v >> (opposed + 1));
            score += make_score(v, v * (r - 2) / 4);
        }
        else if (!neighbours)
            score -= Isolated[pos.variant()], e->weakUnopposed[Us] += !opposed;

        else if (backward)
            score -= Backward[pos.variant()], e->weakUnopposed[Us] += !opposed;

#ifdef HORDE
        if (doubled && (!support || pos.is_horde()))
#else
        if (doubled && !support)
#endif
            score -= Doubled[pos.variant()];
    }

    return score;
  }

} // namespace

namespace Pawns {

/// Pawns::probe() looks up the current position's pawns configuration in
/// the pawns hash table. It returns a pointer to the Entry if the position
/// is found. Otherwise a new Entry is computed and stored there, so we don't
/// have to recompute all when the same pawns configuration occurs again.

Entry* probe(const Position& pos) {

  Key key = pos.pawn_key();
  Entry* e = pos.this_thread()->pawnsTable[key];

  if (e->key == key)
      return e;

  e->key = key;
  e->scores[WHITE] = evaluate<WHITE>(pos, e);
  e->scores[BLACK] = evaluate<BLACK>(pos, e);

  return e;
}


/// Entry::evaluate_shelter() calculates the shelter bonus and the storm
/// penalty for a king, looking at the king file and the two closest files.

template<Color Us>
Value Entry::evaluate_shelter(const Position& pos, Square ksq) {

  constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
  constexpr Direction Down = (Us == WHITE ? SOUTH : NORTH);
  constexpr Bitboard  BlockRanks = (Us == WHITE ? Rank1BB | Rank2BB : Rank8BB | Rank7BB);

  Bitboard b = pos.pieces(PAWN) & ~forward_ranks_bb(Them, ksq);
  Bitboard ourPawns = b & pos.pieces(Us);
  Bitboard theirPawns = b & pos.pieces(Them);

  Value safety = (shift<Down>(theirPawns) & (FileABB | FileHBB) & BlockRanks & ksq) ?
                 Value(374) : Value(5);

  File center = clamp(file_of(ksq), FILE_B, FILE_G);
  for (File f = File(center - 1); f <= File(center + 1); ++f)
  {
      b = ourPawns & file_bb(f);
      Rank ourRank = b ? relative_rank(Us, backmost_sq(Us, b)) : RANK_1;

      b = theirPawns & file_bb(f);
      Rank theirRank = b ? relative_rank(Us, frontmost_sq(Them, b)) : RANK_1;

      int d = std::min(f, ~f);
      safety += ShelterStrength[pos.variant()][d][ourRank];
      safety -= (ourRank && (ourRank == theirRank - 1)) ? 66 * (theirRank == RANK_3)
                                                        : UnblockedStorm[d][theirRank];
  }

  return safety;
}


/// Entry::do_king_safety() calculates a bonus for king safety. It is called only
/// when king square changes, which is about 20% of total king_safety() calls.

template<Color Us>
Score Entry::do_king_safety(const Position& pos) {

  Square ksq = pos.square<KING>(Us);
  kingSquares[Us] = ksq;
  castlingRights[Us] = pos.castling_rights(Us);
  int minKingPawnDistance = 0;

  Bitboard pawns = pos.pieces(Us, PAWN);
  if (pawns)
      while (!(DistanceRingBB[ksq][++minKingPawnDistance] & pawns)) {}

  Value bonus = evaluate_shelter<Us>(pos, ksq);

  // If we can castle use the bonus after the castling if it is bigger
  if (pos.can_castle(Us | KING_SIDE))
      bonus = std::max(bonus, evaluate_shelter<Us>(pos, relative_square(Us, SQ_G1)));

  if (pos.can_castle(Us | QUEEN_SIDE))
      bonus = std::max(bonus, evaluate_shelter<Us>(pos, relative_square(Us, SQ_C1)));

#ifdef ATOMIC
  if (pos.is_atomic())
      return make_score(bonus + 16 * minKingPawnDistance, 16 * minKingPawnDistance);
#endif
#ifdef CRAZYHOUSE
  if (pos.is_house())
      return make_score(bonus, bonus);
#endif
  return make_score(bonus, -16 * minKingPawnDistance);
}

// Explicit template instantiation
template Score Entry::do_king_safety<WHITE>(const Position& pos);
template Score Entry::do_king_safety<BLACK>(const Position& pos);

} // namespace Pawns
