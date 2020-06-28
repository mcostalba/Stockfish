/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2020 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

/// When compiling with provided Makefile (e.g. for Linux and OSX), configuration
/// is done automatically. To get started type 'make help'.
///
/// When Makefile is not used (e.g. with Microsoft Visual Studio) some switches
/// need to be set manually:
///
/// -DNDEBUG      | Disable debugging mode. Always use this for release.
///
/// -DNO_PREFETCH | Disable use of prefetch asm-instruction. You may need this to
///               | run on some very old machines.
///
/// -DUSE_POPCNT  | Add runtime support for use of popcnt asm-instruction. Works
///               | only in 64-bit mode and requires hardware with popcnt support.
///
/// -DUSE_PEXT    | Add runtime support for use of pext asm-instruction. Works
///               | only in 64-bit mode and requires hardware with pext support.

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>

#if defined(_MSC_VER)
// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#endif

/// Predefined macros hell:
///
/// __GNUC__           Compiler is gcc, Clang or Intel on Linux
/// __INTEL_COMPILER   Compiler is Intel
/// _MSC_VER           Compiler is MSVC or Intel on Windows
/// _WIN32             Building on Windows (any)
/// _WIN64             Building on Windows 64 bit

#if defined(_WIN64) && defined(_MSC_VER) // No Makefile used
#  include <intrin.h> // Microsoft header for _BitScanForward64()
#  define IS_64BIT
#endif

#if defined(USE_POPCNT) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <nmmintrin.h> // Intel and Microsoft header for _mm_popcnt_u64()
#endif

#if !defined(NO_PREFETCH) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <xmmintrin.h> // Intel and Microsoft header for _mm_prefetch()
#endif

#if defined(USE_PEXT)
#  include <immintrin.h> // Header for _pext_u64() intrinsic
#  define pext(b, m) _pext_u64(b, m)
#else
#  define pext(b, m) 0
#endif

#ifdef USE_POPCNT
constexpr bool HasPopCnt = true;
#else
constexpr bool HasPopCnt = false;
#endif

#ifdef USE_PEXT
constexpr bool HasPext = true;
#else
constexpr bool HasPext = false;
#endif

#ifdef IS_64BIT
constexpr bool Is64Bit = true;
#else
constexpr bool Is64Bit = false;
#endif

typedef uint64_t Key;
typedef uint64_t Bitboard;

#if defined(CRAZYHOUSE) || defined(HORDE)
constexpr int MAX_MOVES = 512;
#else
constexpr int MAX_MOVES = 256;
#endif
constexpr int MAX_PLY   = 246;

enum Variant {
  //main variants
  CHESS_VARIANT,
#ifdef ANTI
  ANTI_VARIANT,
#endif
#ifdef ATOMIC
  ATOMIC_VARIANT,
#endif
#ifdef CRAZYHOUSE
  CRAZYHOUSE_VARIANT,
#endif
#ifdef EXTINCTION
  EXTINCTION_VARIANT,
#endif
#ifdef GRID
  GRID_VARIANT,
#endif
#ifdef HORDE
  HORDE_VARIANT,
#endif
#ifdef KOTH
  KOTH_VARIANT,
#endif
#ifdef LOSERS
  LOSERS_VARIANT,
#endif
#ifdef RACE
  RACE_VARIANT,
#endif
#ifdef THREECHECK
  THREECHECK_VARIANT,
#endif
#ifdef TWOKINGS
  TWOKINGS_VARIANT,
#endif
  VARIANT_NB,
  LAST_VARIANT = VARIANT_NB - 1,
  //subvariants
#ifdef ANTIHELPMATE
  ANTIHELPMATE_VARIANT,
#endif
#ifdef HELPMATE
  HELPMATE_VARIANT,
#endif
#ifdef GIVEAWAY
  GIVEAWAY_VARIANT,
#endif
#ifdef SUICIDE
  SUICIDE_VARIANT,
#endif
#ifdef BUGHOUSE
  BUGHOUSE_VARIANT,
#endif
#ifdef DISPLACEDGRID
  DISPLACEDGRID_VARIANT,
#endif
#ifdef LOOP
  LOOP_VARIANT,
#endif
#ifdef PLACEMENT
  PLACEMENT_VARIANT,
#endif
#ifdef KNIGHTRELAY
  KNIGHTRELAY_VARIANT,
#endif
#ifdef RELAY
  RELAY_VARIANT,
#endif
#ifdef SLIPPEDGRID
  SLIPPEDGRID_VARIANT,
#endif
#ifdef TWOKINGSSYMMETRIC
  TWOKINGSSYMMETRIC_VARIANT,
#endif
  SUBVARIANT_NB,
};

//static const constexpr char* variants[] doesn't play nicely with uci.h
static std::vector<std::string> variants = {
//main variants
"chess",
#ifdef ANTI
"antichess",
#endif
#ifdef ATOMIC
"atomic",
#endif
#ifdef CRAZYHOUSE
"crazyhouse",
#endif
#ifdef EXTINCTION
"extinction",
#endif
#ifdef GRID
"grid",
#endif
#ifdef HORDE
"horde",
#endif
#ifdef KOTH
"kingofthehill",
#endif
#ifdef LOSERS
"losers",
#endif
#ifdef RACE
"racingkings",
#endif
#ifdef THREECHECK
"3check",
#endif
#ifdef TWOKINGS
"twokings",
#endif
//subvariants
#ifdef ANTIHELPMATE
"antihelpmate",
#endif
#ifdef HELPMATE
"helpmate",
#endif
#ifdef GIVEAWAY
"giveaway",
#endif
#ifdef SUICIDE
"suicide",
#endif
#ifdef BUGHOUSE
"bughouse",
#endif
#ifdef DISPLACEDGRID
"displacedgrid",
#endif
#ifdef LOOP
"loop",
#endif
#ifdef PLACEMENT
"placement",
#endif
#ifdef KNIGHTRELAY
"knightrelay",
#endif
#ifdef RELAY
"relay",
#endif
#ifdef SLIPPEDGRID
"slippedgrid",
#endif
#ifdef TWOKINGSSYMMETRIC
"twokingssymmetric",
#endif
};

/// A move needs 16 bits to be stored
///
/// bit  0- 5: destination square (from 0 to 63)
/// bit  6-11: origin square (from 0 to 63)
/// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
/// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
/// NOTE: EN-PASSANT bit is set only when a pawn can be captured
///
/// Special cases are MOVE_NONE and MOVE_NULL. We can sneak these in because in
/// any normal move destination square is always different from origin square
/// while MOVE_NONE and MOVE_NULL have the same origin and destination square.

enum Move : int {
  MOVE_NONE,
  MOVE_NULL = 65
};

enum MoveType {
  NORMAL,
  PROMOTION = 1 << 14,
  ENPASSANT = 2 << 14,
  CASTLING  = 3 << 14,
  // special moves use promotion piece type bits as flags
#if defined(ANTI) || defined(CRAZYHOUSE) || defined(EXTINCTION)
  SPECIAL = ENPASSANT,
#endif
#ifdef CRAZYHOUSE
  DROP = 1 << 12,
#endif
#if defined(ANTI) || defined(EXTINCTION)
  KING_PROMOTION = 2 << 12, // not used as an actual move type
#endif
};

enum Color {
  WHITE, BLACK, COLOR_NB = 2
};

enum CastlingRights {
  NO_CASTLING,
  WHITE_OO,
  WHITE_OOO = WHITE_OO << 1,
  BLACK_OO  = WHITE_OO << 2,
  BLACK_OOO = WHITE_OO << 3,

  KING_SIDE      = WHITE_OO  | BLACK_OO,
  QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
  WHITE_CASTLING = WHITE_OO  | WHITE_OOO,
  BLACK_CASTLING = BLACK_OO  | BLACK_OOO,
  ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING,

  CASTLING_RIGHT_NB = 16
};

#ifdef GRID
enum GridLayout {
  NORMAL_GRID,
#ifdef DISPLACEDGRID
  DISPLACED_GRID,
#endif
#ifdef SLIPPEDGRID
  SLIPPED_GRID,
#endif
  GRIDLAYOUT_NB
};
#endif

#ifdef THREECHECK
enum CheckCount : int {
  CHECKS_0 = 0, CHECKS_1 = 1, CHECKS_2 = 2, CHECKS_3 = 3, CHECKS_NB = 4
};
#endif

enum Phase {
  PHASE_ENDGAME,
  PHASE_MIDGAME = 128,
  MG = 0, EG = 1, PHASE_NB = 2
};

enum ScaleFactor {
  SCALE_FACTOR_DRAW    = 0,
  SCALE_FACTOR_NORMAL  = 64,
  SCALE_FACTOR_MAX     = 128,
  SCALE_FACTOR_NONE    = 255
};

enum Bound {
  BOUND_NONE,
  BOUND_UPPER,
  BOUND_LOWER,
  BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Value : int {
  VALUE_ZERO      = 0,
  VALUE_DRAW      = 0,
  VALUE_KNOWN_WIN = 10000,
  VALUE_MATE      = 32000,
  VALUE_INFINITE  = 32001,
  VALUE_NONE      = 32002,

  VALUE_TB_WIN_IN_MAX_PLY  =  VALUE_MATE - 2 * MAX_PLY,
  VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY,
  VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - MAX_PLY,
  VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY,

  PawnValueMg   = 124,   PawnValueEg   = 206,
  KnightValueMg = 781,   KnightValueEg = 854,
  BishopValueMg = 825,   BishopValueEg = 915,
  RookValueMg   = 1276,  RookValueEg   = 1380,
  QueenValueMg  = 2538,  QueenValueEg  = 2682,
#ifdef ANTI
  PawnValueMgAnti   = -108,  PawnValueEgAnti   = -165,
  KnightValueMgAnti = -155,  KnightValueEgAnti = 194,
  BishopValueMgAnti = -270,  BishopValueEgAnti = 133,
  RookValueMgAnti   = -472,  RookValueEgAnti   = 56,
  QueenValueMgAnti  = -114,  QueenValueEgAnti  = -218,
  KingValueMgAnti   = -23,   KingValueEgAnti   = 173,
#endif
#ifdef ATOMIC
  PawnValueMgAtomic   = 244,   PawnValueEgAtomic   = 367,
  KnightValueMgAtomic = 437,   KnightValueEgAtomic = 652,
  BishopValueMgAtomic = 552,   BishopValueEgAtomic = 716,
  RookValueMgAtomic   = 787,   RookValueEgAtomic   = 1074,
  QueenValueMgAtomic  = 1447,  QueenValueEgAtomic  = 1892,
#endif
#ifdef CRAZYHOUSE
  PawnValueMgHouse   = 149,   PawnValueEgHouse   = 206,
  KnightValueMgHouse = 447,   KnightValueEgHouse = 527,
  BishopValueMgHouse = 450,   BishopValueEgHouse = 521,
  RookValueMgHouse   = 619,   RookValueEgHouse   = 669,
  QueenValueMgHouse  = 878,   QueenValueEgHouse  = 965,
#endif
#ifdef EXTINCTION
  PawnValueMgExtinction   = 209,   PawnValueEgExtinction   = 208,
  KnightValueMgExtinction = 823,   KnightValueEgExtinction = 1091,
  BishopValueMgExtinction = 1097,  BishopValueEgExtinction = 1055,
  RookValueMgExtinction   = 726,   RookValueEgExtinction   = 950,
  QueenValueMgExtinction  = 2111,  QueenValueEgExtinction  = 2014,
  KingValueMgExtinction   = 919,   KingValueEgExtinction   = 1093,
#endif
#ifdef GRID
  PawnValueMgGrid   = 38,    PawnValueEgGrid   = 55,
  KnightValueMgGrid = 993,   KnightValueEgGrid = 903,
  BishopValueMgGrid = 685,   BishopValueEgGrid = 750,
  RookValueMgGrid   = 1018,  RookValueEgGrid   = 1055,
  QueenValueMgGrid  = 2556,  QueenValueEgGrid  = 2364,
#endif
#ifdef HORDE
  PawnValueMgHorde   = 321,   PawnValueEgHorde   = 326,
  KnightValueMgHorde = 888,   KnightValueEgHorde = 991,
  BishopValueMgHorde = 743,   BishopValueEgHorde = 1114,
  RookValueMgHorde   = 948,   RookValueEgHorde   = 1230,
  QueenValueMgHorde  = 2736,  QueenValueEgHorde  = 2554,
  KingValueMgHorde   = 2073,  KingValueEgHorde   = 921,
#endif
#ifdef KOTH
  PawnValueMgHill   = 136,   PawnValueEgHill   = 225,
  KnightValueMgHill = 657,   KnightValueEgHill = 781,
  BishopValueMgHill = 763,   BishopValueEgHill = 849,
  RookValueMgHill   = 1010,  RookValueEgHill   = 1175,
  QueenValueMgHill  = 2104,  QueenValueEgHill  = 2402,
#endif
#ifdef LOSERS
  PawnValueMgLosers   = -40,   PawnValueEgLosers   = -25,
  KnightValueMgLosers = -23,   KnightValueEgLosers = 369,
  BishopValueMgLosers = -206,  BishopValueEgLosers = 245,
  RookValueMgLosers   = -415,  RookValueEgLosers   = 80,
  QueenValueMgLosers  = -111,  QueenValueEgLosers  = -209,
#endif
#ifdef RACE
  KnightValueMgRace = 777,   KnightValueEgRace = 881,
  BishopValueMgRace = 1025,  BishopValueEgRace = 1070,
  RookValueMgRace   = 1272,  RookValueEgRace   = 1847,
  QueenValueMgRace  = 1674,  QueenValueEgRace  = 2280,
#endif
#ifdef THREECHECK
  PawnValueMgThreeCheck   = 119,   PawnValueEgThreeCheck   = 205,
  KnightValueMgThreeCheck = 645,   KnightValueEgThreeCheck = 770,
  BishopValueMgThreeCheck = 693,   BishopValueEgThreeCheck = 754,
  RookValueMgThreeCheck   = 1027,  RookValueEgThreeCheck   = 1418,
  QueenValueMgThreeCheck  = 1947,  QueenValueEgThreeCheck  = 2323,
#endif
#ifdef TWOKINGS
  PawnValueMgTwoKings   = 206,   PawnValueEgTwoKings   = 265,
  KnightValueMgTwoKings = 887,   KnightValueEgTwoKings = 871,
  BishopValueMgTwoKings = 940,   BishopValueEgTwoKings = 898,
  RookValueMgTwoKings   = 1360,  RookValueEgTwoKings   = 1415,
  QueenValueMgTwoKings  = 2455,  QueenValueEgTwoKings  = 2846,
  KingValueMgTwoKings   = 554,   KingValueEgTwoKings   = 806,
#endif
  Tempo = 28,

  MidgameLimit  = 15258, EndgameLimit  = 3915
};

enum PieceType {
  NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
  ALL_PIECES = 0,
  PIECE_TYPE_NB = 8
};

enum Piece {
  NO_PIECE,
  W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
  B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
  PIECE_NB = 16
};

constexpr Value PieceValue[VARIANT_NB][PHASE_NB][PIECE_NB] = {
{
  { VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg, VALUE_ZERO, VALUE_ZERO }
},
#ifdef ANTI
{
  { VALUE_ZERO, PawnValueMgAnti, KnightValueMgAnti, BishopValueMgAnti, RookValueMgAnti, QueenValueMgAnti, KingValueMgAnti, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgAnti, KnightValueEgAnti, BishopValueEgAnti, RookValueEgAnti, QueenValueEgAnti, KingValueEgAnti, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgAnti, KnightValueMgAnti, BishopValueMgAnti, RookValueMgAnti, QueenValueMgAnti, KingValueMgAnti, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgAnti, KnightValueEgAnti, BishopValueEgAnti, RookValueEgAnti, QueenValueEgAnti, KingValueEgAnti, VALUE_ZERO }
},
#endif
#ifdef ATOMIC
{
  { VALUE_ZERO, PawnValueMgAtomic, KnightValueMgAtomic, BishopValueMgAtomic, RookValueMgAtomic, QueenValueMgAtomic, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgAtomic, KnightValueEgAtomic, BishopValueEgAtomic, RookValueEgAtomic, QueenValueEgAtomic, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgAtomic, KnightValueMgAtomic, BishopValueMgAtomic, RookValueMgAtomic, QueenValueMgAtomic, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgAtomic, KnightValueEgAtomic, BishopValueEgAtomic, RookValueEgAtomic, QueenValueEgAtomic, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef CRAZYHOUSE
{
  { VALUE_ZERO, PawnValueMgHouse, KnightValueMgHouse, BishopValueMgHouse, RookValueMgHouse, QueenValueMgHouse, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgHouse, KnightValueEgHouse, BishopValueEgHouse, RookValueEgHouse, QueenValueEgHouse, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgHouse, KnightValueMgHouse, BishopValueMgHouse, RookValueMgHouse, QueenValueMgHouse, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgHouse, KnightValueEgHouse, BishopValueEgHouse, RookValueEgHouse, QueenValueEgHouse, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef EXTINCTION
{
  { VALUE_ZERO, PawnValueMgExtinction, KnightValueMgExtinction, BishopValueMgExtinction, RookValueMgExtinction, QueenValueMgExtinction, KingValueMgExtinction, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgExtinction, KnightValueEgExtinction, BishopValueEgExtinction, RookValueEgExtinction, QueenValueEgExtinction, KingValueEgExtinction, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgExtinction, KnightValueMgExtinction, BishopValueMgExtinction, RookValueMgExtinction, QueenValueMgExtinction, KingValueMgExtinction, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgExtinction, KnightValueEgExtinction, BishopValueEgExtinction, RookValueEgExtinction, QueenValueEgExtinction, KingValueEgExtinction, VALUE_ZERO }
},
#endif
#ifdef GRID
{
  { VALUE_ZERO, PawnValueMgGrid, KnightValueMgGrid, BishopValueMgGrid, RookValueMgGrid, QueenValueMgGrid, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgGrid, KnightValueEgGrid, BishopValueEgGrid, RookValueEgGrid, QueenValueEgGrid, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgGrid, KnightValueMgGrid, BishopValueMgGrid, RookValueMgGrid, QueenValueMgGrid, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgGrid, KnightValueEgGrid, BishopValueEgGrid, RookValueEgGrid, QueenValueEgGrid, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef HORDE
{
  { VALUE_ZERO, PawnValueMgHorde, KnightValueMgHorde, BishopValueMgHorde, RookValueMgHorde, QueenValueMgHorde, KingValueMgHorde, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgHorde, KnightValueEgHorde, BishopValueEgHorde, RookValueEgHorde, QueenValueEgHorde, KingValueEgHorde, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgHorde, KnightValueMgHorde, BishopValueMgHorde, RookValueMgHorde, QueenValueMgHorde, KingValueMgHorde, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgHorde, KnightValueEgHorde, BishopValueEgHorde, RookValueEgHorde, QueenValueEgHorde, KingValueEgHorde, VALUE_ZERO }
},
#endif
#ifdef KOTH
{
  { VALUE_ZERO, PawnValueMgHill, KnightValueMgHill, BishopValueMgHill, RookValueMgHill, QueenValueMgHill, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgHill, KnightValueEgHill, BishopValueEgHill, RookValueEgHill, QueenValueEgHill, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgHill, KnightValueMgHill, BishopValueMgHill, RookValueMgHill, QueenValueMgHill, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgHill, KnightValueEgHill, BishopValueEgHill, RookValueEgHill, QueenValueEgHill, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef LOSERS
{
  { VALUE_ZERO, PawnValueMgLosers, KnightValueMgLosers, BishopValueMgLosers, RookValueMgLosers, QueenValueMgLosers, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgLosers, KnightValueEgLosers, BishopValueEgLosers, RookValueEgLosers, QueenValueEgLosers, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgLosers, KnightValueMgLosers, BishopValueMgLosers, RookValueMgLosers, QueenValueMgLosers, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgLosers, KnightValueEgLosers, BishopValueEgLosers, RookValueEgLosers, QueenValueEgLosers, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef RACE
{
  { VALUE_ZERO, VALUE_ZERO, KnightValueMgRace, BishopValueMgRace, RookValueMgRace, QueenValueMgRace, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, KnightValueEgRace, BishopValueEgRace, RookValueEgRace, QueenValueEgRace, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, VALUE_ZERO, KnightValueMgRace, BishopValueMgRace, RookValueMgRace, QueenValueMgRace, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, KnightValueEgRace, BishopValueEgRace, RookValueEgRace, QueenValueEgRace, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef THREECHECK
{
  { VALUE_ZERO, PawnValueMgThreeCheck, KnightValueMgThreeCheck, BishopValueMgThreeCheck, RookValueMgThreeCheck, QueenValueMgThreeCheck, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgThreeCheck, KnightValueEgThreeCheck, BishopValueEgThreeCheck, RookValueEgThreeCheck, QueenValueEgThreeCheck, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgThreeCheck, KnightValueMgThreeCheck, BishopValueMgThreeCheck, RookValueMgThreeCheck, QueenValueMgThreeCheck, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgThreeCheck, KnightValueEgThreeCheck, BishopValueEgThreeCheck, RookValueEgThreeCheck, QueenValueEgThreeCheck, VALUE_ZERO, VALUE_ZERO }
},
#endif
#ifdef TWOKINGS
{
  { VALUE_ZERO, PawnValueMgTwoKings, KnightValueMgTwoKings, BishopValueMgTwoKings, RookValueMgTwoKings, QueenValueMgTwoKings, KingValueMgTwoKings, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgTwoKings, KnightValueEgTwoKings, BishopValueEgTwoKings, RookValueEgTwoKings, QueenValueEgTwoKings, KingValueEgTwoKings, VALUE_ZERO },
  { VALUE_ZERO, PawnValueMgTwoKings, KnightValueMgTwoKings, BishopValueMgTwoKings, RookValueMgTwoKings, QueenValueMgTwoKings, KingValueMgTwoKings, VALUE_ZERO,
    VALUE_ZERO, PawnValueEgTwoKings, KnightValueEgTwoKings, BishopValueEgTwoKings, RookValueEgTwoKings, QueenValueEgTwoKings, KingValueEgTwoKings, VALUE_ZERO }
},
#endif
};

typedef int Depth;

enum : int {
  DEPTH_QS_CHECKS     =  0,
  DEPTH_QS_NO_CHECKS  = -1,
  DEPTH_QS_RECAPTURES = -5,

  DEPTH_NONE   = -6,
  DEPTH_OFFSET = DEPTH_NONE
};

enum Square : int {
  SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
  SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
  SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
  SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
  SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
  SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
  SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
  SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
  SQ_NONE,

  SQUARE_NB = 64
};

enum Direction : int {
  NORTH =  8,
  EAST  =  1,
  SOUTH = -NORTH,
  WEST  = -EAST,

  NORTH_EAST = NORTH + EAST,
  SOUTH_EAST = SOUTH + EAST,
  SOUTH_WEST = SOUTH + WEST,
  NORTH_WEST = NORTH + WEST
};

enum File : int {
  FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB
};

enum Rank : int {
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
};


/// Score enum stores a middlegame and an endgame value in a single integer (enum).
/// The least significant 16 bits are used to store the middlegame value and the
/// upper 16 bits are used to store the endgame value. We have to take care to
/// avoid left-shifting a signed int to avoid undefined behavior.
enum Score : int { SCORE_ZERO };

constexpr Score make_score(int mg, int eg) {
  return Score((int)((unsigned int)eg << 16) + mg);
}

/// Extracting the signed lower and upper 16 bits is not so trivial because
/// according to the standard a simple cast to short is implementation defined
/// and so is a right shift of a signed integer.
inline Value eg_value(Score s) {
  union { uint16_t u; int16_t s; } eg = { uint16_t(unsigned(s + 0x8000) >> 16) };
  return Value(eg.s);
}

inline Value mg_value(Score s) {
  union { uint16_t u; int16_t s; } mg = { uint16_t(unsigned(s)) };
  return Value(mg.s);
}

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, int d2) { return T(int(d1) + d2); } \
constexpr T operator-(T d1, int d2) { return T(int(d1) - d2); } \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }         \
inline T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)
ENABLE_FULL_OPERATORS_ON(Direction)

ENABLE_INCR_OPERATORS_ON(Variant)
#ifdef THREECHECK
ENABLE_INCR_OPERATORS_ON(CheckCount)
#endif
ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)

ENABLE_BASE_OPERATORS_ON(Score)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

/// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square& operator+=(Square& s, Direction d) { return s = s + d; }
inline Square& operator-=(Square& s, Direction d) { return s = s - d; }

/// Only declared but not defined. We don't want to multiply two scores due to
/// a very high risk of overflow. So user should explicitly convert to integer.
Score operator*(Score, Score) = delete;

/// Division of a Score must be handled separately for each term
inline Score operator/(Score s, int i) {
  return make_score(mg_value(s) / i, eg_value(s) / i);
}

/// Multiplication of a Score by an integer. We check for overflow in debug mode.
inline Score operator*(Score s, int i) {

  Score result = Score(int(s) * i);

  assert(eg_value(result) == (i * eg_value(s)));
  assert(mg_value(result) == (i * mg_value(s)));
  assert((i == 0) || (result / i) == s);

  return result;
}

/// Multiplication of a Score by a boolean
inline Score operator*(Score s, bool b) {
  return b ? s : SCORE_ZERO;
}

constexpr Color operator~(Color c) {
  return Color(c ^ BLACK); // Toggle color
}

constexpr Square flip_rank(Square s) { // Swap A1 <-> A8
  return Square(s ^ SQ_A8);
}

constexpr Square flip_file(Square s) { // Swap A1 <-> H1
  return Square(s ^ SQ_H1);
}

constexpr Piece operator~(Piece pc) {
  return Piece(pc ^ 8); // Swap color of piece B_KNIGHT <-> W_KNIGHT
}

constexpr CastlingRights operator&(Color c, CastlingRights cr) {
  return CastlingRights((c == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr);
}

constexpr Value mate_in(int ply) {
  return VALUE_MATE - ply;
}

constexpr Value mated_in(int ply) {
  return -VALUE_MATE + ply;
}

constexpr Square make_square(File f, Rank r) {
  return Square((r << 3) + f);
}

constexpr Piece make_piece(Color c, PieceType pt) {
  return Piece((c << 3) + pt);
}

constexpr PieceType type_of(Piece pc) {
  return PieceType(pc & 7);
}

inline Color color_of(Piece pc) {
  assert(pc != NO_PIECE);
  return Color(pc >> 3);
}

constexpr bool is_ok(Square s) {
  return s >= SQ_A1 && s <= SQ_H8;
}

constexpr File file_of(Square s) {
  return File(s & 7);
}

constexpr Rank rank_of(Square s) {
  return Rank(s >> 3);
}

constexpr Square relative_square(Color c, Square s) {
  return Square(s ^ (c * 56));
}

constexpr Rank relative_rank(Color c, Rank r) {
  return Rank(r ^ (c * 7));
}

constexpr Rank relative_rank(Color c, Square s) {
  return relative_rank(c, rank_of(s));
}

constexpr Direction pawn_push(Color c) {
  return c == WHITE ? NORTH : SOUTH;
}

#ifdef RACE
constexpr Square horizontal_flip(Square s) {
  return Square(s ^ (FILE_NB - 1)); // Horizontal flip SQ_A1 -> SQ_H1
}
#endif

inline MoveType type_of(Move m);

inline Square from_sq(Move m) {
#ifdef CRAZYHOUSE
  if (type_of(m) == DROP)
      return SQ_NONE;
#endif
  return Square((m >> 6) & 0x3F);
}

constexpr Square to_sq(Move m) {
  return Square(m & 0x3F);
}

inline int from_to(Move m) {
#ifdef CRAZYHOUSE
  if (type_of(m) == DROP)
      return (to_sq(m) << 6) + to_sq(m);
#endif
 return m & 0xFFF;
}

inline MoveType type_of(Move m) {
#if defined(ANTI) || defined(CRAZYHOUSE) || defined(EXTINCTION)
  if ((m & (3 << 14)) == SPECIAL && (m & (3 << 12)))
  {
#ifdef CRAZYHOUSE
      if ((m & (3 << 12)) == DROP)
          return DROP;
#endif
#if defined(ANTI) || defined(EXTINCTION)
      if ((m & (3 << 12)) == KING_PROMOTION)
          return PROMOTION;
#endif
  }
#endif
  return MoveType(m & (3 << 14));
}

inline PieceType promotion_type(Move m) {
#if defined(ANTI) || defined(EXTINCTION)
  if ((m & (3 << 14)) == SPECIAL && (m & (3 << 12)) == KING_PROMOTION)
      return KING;
#endif
  return PieceType(((m >> 12) & 3) + KNIGHT);
}

constexpr Move make_move(Square from, Square to) {
  return Move((from << 6) + to);
}

inline Move reverse_move(Move m) {
  return make_move(to_sq(m), from_sq(m));
}

template<MoveType T>
inline Move make(Square from, Square to, PieceType pt = KNIGHT) {
#if defined(ANTI) || defined(EXTINCTION)
  if (pt == KING)
      return Move(SPECIAL + KING_PROMOTION + (from << 6) + to);
#endif
  return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
}

#ifdef CRAZYHOUSE
constexpr Move make_drop(Square to, Piece pc) {
  return Move(SPECIAL + DROP + (pc << 6) + to);
}

constexpr Piece dropped_piece(Move m) {
  return Piece((m >> 6) & 0x0F);
}
#endif

inline bool is_ok(Move m) {
  return from_sq(m) != to_sq(m); // Catch MOVE_NULL and MOVE_NONE
}

/// Based on a congruential pseudo random number generator
constexpr Key make_key(uint64_t seed) {
  return seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

inline Variant main_variant(Variant v) {
  if (v < VARIANT_NB)
      return v;
  switch(v)
  {
#ifdef ANTIHELPMATE
  case ANTIHELPMATE_VARIANT:
      return CHESS_VARIANT;
#endif
#ifdef HELPMATE
  case HELPMATE_VARIANT:
      return CHESS_VARIANT;
#endif
#ifdef GIVEAWAY
  case GIVEAWAY_VARIANT:
      return ANTI_VARIANT;
#endif
#ifdef SUICIDE
  case SUICIDE_VARIANT:
      return ANTI_VARIANT;
#endif
#ifdef BUGHOUSE
  case BUGHOUSE_VARIANT:
      return CRAZYHOUSE_VARIANT;
#endif
#ifdef DISPLACEDGRID
  case DISPLACEDGRID_VARIANT:
      return GRID_VARIANT;
#endif
#ifdef LOOP
  case LOOP_VARIANT:
      return CRAZYHOUSE_VARIANT;
#endif
#ifdef PLACEMENT
  case PLACEMENT_VARIANT:
      return CRAZYHOUSE_VARIANT;
#endif
#ifdef KNIGHTRELAY
  case KNIGHTRELAY_VARIANT:
      return CHESS_VARIANT;
#endif
#ifdef RELAY
  case RELAY_VARIANT:
      return CHESS_VARIANT;
#endif
#ifdef SLIPPEDGRID
  case SLIPPEDGRID_VARIANT:
      return GRID_VARIANT;
#endif
#ifdef TWOKINGSSYMMETRIC
  case TWOKINGSSYMMETRIC_VARIANT:
      return TWOKINGS_VARIANT;
#endif
  default:
      assert(false);
      return CHESS_VARIANT; // Silence a warning
  }
}

#endif // #ifndef TYPES_H_INCLUDED

#include "tune.h" // Global visibility to tuning setup
