/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <deque>
#include <memory> // For std::unique_ptr
#include <string>

#include "bitboard.h"
#include "types.h"

/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {

  // Copied when making a move
  Key    pawnKey;
  Key    materialKey;
  Value  nonPawnMaterial[COLOR_NB];
  int    castlingRights;
  int    rule50;
  int    pliesFromNull;
#ifdef THREECHECK
  CheckCount checksGiven[COLOR_NB];
#endif
  Score  psq;
  Square epSquare;

  // Not copied when making a move (will be recomputed anyhow)
  Key        key;
  Bitboard   checkersBB;
  Piece      capturedPiece;
#ifdef ATOMIC
  Piece      blast[SQUARE_NB];
#endif
#ifdef CRAZYHOUSE
  bool       capturedpromoted;
#endif
  StateInfo* previous;
  Bitboard   blockersForKing[COLOR_NB];
  Bitboard   pinnersForKing[COLOR_NB];
#ifdef RELAY
  Square     pieceListRelay[PIECE_NB][16];
  Bitboard   byTypeBBRelay[PIECE_TYPE_NB];
#endif
  Bitboard   checkSquares[PIECE_TYPE_NB];
};

// In a std::deque references to elements are unaffected upon resizing
typedef std::unique_ptr<std::deque<StateInfo>> StateListPtr;


/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.
class Thread;

class Position {
public:
  static void init();

  Position() = default;
  Position(const Position&) = delete;
  Position& operator=(const Position&) = delete;

  // FEN string input/output
  Position& set(const std::string& fenStr, bool isChess960, Variant v, StateInfo* si, Thread* th);
  Position& set(const std::string& code, Color c, Variant v, StateInfo* si);
  const std::string fen() const;

  // Position representation
  Bitboard pieces() const;
  Bitboard pieces(PieceType pt) const;
  Bitboard pieces(PieceType pt1, PieceType pt2) const;
  Bitboard pieces(Color c) const;
  Bitboard pieces(Color c, PieceType pt) const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
  Piece piece_on(Square s) const;
  Square ep_square() const;
  bool empty(Square s) const;
  template<PieceType Pt> int count(Color c) const;
  template<PieceType Pt> const Square* squares(Color c) const;
  template<PieceType Pt> Square square(Color c) const;

  // Castling
  int can_castle(Color c) const;
  int can_castle(CastlingRight cr) const;
  bool castling_impeded(CastlingRight cr) const;
#ifdef ANTI
  Square castling_king_square(CastlingRight cr) const;
#endif
  Square castling_rook_square(CastlingRight cr) const;

  // Checking
  Bitboard checkers() const;
  Bitboard discovered_check_candidates() const;
  Bitboard pinned_pieces(Color c) const;
  Bitboard check_squares(PieceType pt) const;

  // Attacks to/from a given square
  Bitboard attackers_to(Square s) const;
  Bitboard attackers_to(Square s, Bitboard occupied) const;
  Bitboard attacks_from(Piece pc, Square s) const;
  template<PieceType> Bitboard attacks_from(Square s) const;
  template<PieceType> Bitboard attacks_from(Square s, Color c) const;
  Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const;

  // Properties of moves
  bool legal(Move m) const;
  bool pseudo_legal(const Move m) const;
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  bool gives_check(Move m) const;
  bool advanced_pawn_push(Move m) const;
  Piece moved_piece(Move m) const;
  Piece captured_piece() const;

  // Piece specific
  bool pawn_passed(Color c, Square s) const;
  bool opposite_bishops() const;

  // Doing and undoing moves
  void do_move(Move m, StateInfo& newSt);
  void do_move(Move m, StateInfo& newSt, bool givesCheck);
  void undo_move(Move m);
  void do_null_move(StateInfo& newSt);
  void undo_null_move();

  // Static Exchange Evaluation
#ifdef ATOMIC
  template<Variant v>
  Value see(Move m) const;
#endif
  bool see_ge(Move m, Value value) const;

  // Accessing hash keys
  Key key() const;
  Key key_after(Move m) const;
  Key material_key() const;
  Key pawn_key() const;

  // Other properties of the position
  Color side_to_move() const;
  Phase game_phase() const;
  int game_ply() const;
  bool is_chess960() const;
  Variant variant() const;
  Variant subvariant() const;
#ifdef ATOMIC
  bool is_atomic() const;
  bool is_atomic_win() const;
  bool is_atomic_loss() const;
#endif
#ifdef HORDE
  bool is_horde() const;
  bool is_horde_color(Color c) const;
  bool is_horde_loss() const;
#endif
#ifdef CRAZYHOUSE
  bool is_house() const;
  int count_in_hand(Color c, PieceType pt) const;
  void add_to_hand(Color c, PieceType pt);
  void remove_from_hand(Color c, PieceType pt);
  bool is_promoted(Square s) const;
  void drop_piece(Piece pc, Square s);
  void undrop_piece(Piece pc, Square s);
#endif
#ifdef LOOP
  bool is_loop() const;
#endif
#ifdef KOTH
  bool is_koth() const;
  bool is_koth_win() const;
  bool is_koth_loss() const;
  int koth_distance(Color c) const;
#endif
#ifdef LOSERS
  bool is_losers() const;
  bool is_losers_win() const;
  bool is_losers_loss() const;
  bool can_capture_losers() const;
#endif
#ifdef RACE
  bool is_race() const;
  bool is_race_win() const;
  bool is_race_draw() const;
  bool is_race_loss() const;
#endif
#ifdef RELAY
  bool is_relay() const;
#endif
#ifdef THREECHECK
  bool is_three_check() const;
  bool is_three_check_win() const;
  bool is_three_check_loss() const;
  int checks_count() const;
  CheckCount checks_given(Color c) const;
#endif
#ifdef ANTI
  bool is_anti() const;
  bool is_anti_win() const;
  bool is_anti_loss() const;
  bool can_capture() const;
#endif
#ifdef SUICIDE
  bool is_suicide() const;
  Value suicide_stalemate(int ply, Value draw) const;
#endif
  Thread* this_thread() const;
  uint64_t nodes_searched() const;
  bool is_draw() const;
  int rule50_count() const;
  Score psq_score() const;
  Value non_pawn_material(Color c) const;

  // Position consistency check, for debugging
  bool pos_is_ok(int* failedStep = nullptr) const;
  void flip();

private:
  // Initialization helpers (used while setting up a position)
  void set_castling_right(Color c, Square kfrom, Square rfrom);
  void set_state(StateInfo* si) const;
  void set_check_info(StateInfo* si) const;

  // Other helpers
  void put_piece(Piece pc, Square s);
  void remove_piece(Piece pc, Square s);
  void move_piece(Piece pc, Square from, Square to);
  template<bool Do>
  void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

  // Data members
  Piece board[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  int pieceCount[PIECE_NB];
#ifdef HORDE
  Square pieceList[PIECE_NB][SQUARE_NB];
#else
  Square pieceList[PIECE_NB][16];
#endif
#ifdef CRAZYHOUSE
  int pieceCountInHand[COLOR_NB][PIECE_TYPE_NB];
  Bitboard promotedPieces;
#endif
  int index[SQUARE_NB];
  int castlingRightsMask[SQUARE_NB];
#ifdef ANTI
  Square castlingKingSquare[CASTLING_RIGHT_NB];
#endif
  Square castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
  uint64_t nodes;
  int gamePly;
  Color sideToMove;
  Thread* thisThread;
  StateInfo* st;
  bool chess960;
  Variant var;
  Variant subvar;

};

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Color Position::side_to_move() const {
  return sideToMove;
}

inline bool Position::empty(Square s) const {
  return board[s] == NO_PIECE;
}

inline Piece Position::piece_on(Square s) const {
  return board[s];
}

inline Piece Position::moved_piece(Move m) const {
#ifdef CRAZYHOUSE
  if (type_of(m) == DROP)
      return dropped_piece(m);
#endif
  return board[from_sq(m)];
}

inline Bitboard Position::pieces() const {
  return byTypeBB[ALL_PIECES];
}

inline Bitboard Position::pieces(PieceType pt) const {
  return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const {
  return byTypeBB[pt1] | byTypeBB[pt2];
}

inline Bitboard Position::pieces(Color c) const {
  return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const {
  return byColorBB[c] & byTypeBB[pt];
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const {
  return byColorBB[c] & (byTypeBB[pt1] | byTypeBB[pt2]);
}

template<PieceType Pt> inline int Position::count(Color c) const {
  return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt> inline const Square* Position::squares(Color c) const {
  return pieceList[make_piece(c, Pt)];
}

template<PieceType Pt> inline Square Position::square(Color c) const {
#ifdef HORDE
  if (is_horde() && pieceCount[make_piece(c, Pt)] == 0)
      return SQ_NONE;
#endif
#ifdef ATOMIC
  if (is_atomic() && pieceCount[make_piece(c, Pt)] == 0)
      return SQ_NONE;
#endif
#ifdef ANTI
  // There may be zero, one, or multiple kings
  if (is_anti() && Pt == KING)
      return SQ_NONE;
#endif
  assert(pieceCount[make_piece(c, Pt)] == 1);
  return pieceList[make_piece(c, Pt)][0];
}

#ifdef THREECHECK
inline bool Position::is_three_check() const {
  return var == THREECHECK_VARIANT;
}

inline bool Position::is_three_check_win() const {
  return st->checksGiven[sideToMove] == CHECKS_3;
}

inline bool Position::is_three_check_loss() const {
  return st->checksGiven[~sideToMove] == CHECKS_3;
}

inline int Position::checks_count() const {
  return st->checksGiven[WHITE] + st->checksGiven[BLACK];
}

inline CheckCount Position::checks_given(Color c) const {
  return st->checksGiven[c];
}
#endif

inline Square Position::ep_square() const {
  return st->epSquare;
}

inline int Position::can_castle(CastlingRight cr) const {
  return st->castlingRights & cr;
}

inline int Position::can_castle(Color c) const {
  return st->castlingRights & ((WHITE_OO | WHITE_OOO) << (2 * c));
}

inline bool Position::castling_impeded(CastlingRight cr) const {
  return byTypeBB[ALL_PIECES] & castlingPath[cr];
}

#ifdef ANTI
inline Square Position::castling_king_square(CastlingRight cr) const {
  return castlingKingSquare[cr];
}
#endif

inline Square Position::castling_rook_square(CastlingRight cr) const {
  return castlingRookSquare[cr];
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const {
  return  Pt == BISHOP || Pt == ROOK ? attacks_bb<Pt>(s, byTypeBB[ALL_PIECES])
        : Pt == QUEEN  ? attacks_from<ROOK>(s) | attacks_from<BISHOP>(s)
        : StepAttacksBB[Pt][s];
}

template<>
inline Bitboard Position::attacks_from<PAWN>(Square s, Color c) const {
  return StepAttacksBB[make_piece(c, PAWN)][s];
}

inline Bitboard Position::attacks_from(Piece pc, Square s) const {
  return attacks_bb(pc, s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::attackers_to(Square s) const {
  return attackers_to(s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::checkers() const {
  return st->checkersBB;
}

inline Bitboard Position::discovered_check_candidates() const {
  return st->blockersForKing[~sideToMove] & pieces(sideToMove);
}

inline Bitboard Position::pinned_pieces(Color c) const {
  return st->blockersForKing[c] & pieces(c);
}

inline Bitboard Position::check_squares(PieceType pt) const {
  return st->checkSquares[pt];
}

inline bool Position::pawn_passed(Color c, Square s) const {
#ifdef RACE
  if (is_race())
    return true;
#endif
#ifdef HORDE
  if (is_horde() && is_horde_color(c))
      return !(pieces(~c, PAWN) & forward_bb(c, s));
#endif
  return !(pieces(~c, PAWN) & passed_pawn_mask(c, s));
}

inline bool Position::advanced_pawn_push(Move m) const {
#ifdef RACE
  if (is_race())
    return   type_of(moved_piece(m)) == KING
          && rank_of(from_sq(m)) > RANK_4;
#endif
  return   type_of(moved_piece(m)) == PAWN
        && relative_rank(sideToMove, from_sq(m)) > RANK_4;
}

inline Key Position::key() const {
  return st->key;
}

inline Key Position::pawn_key() const {
  return st->pawnKey;
}

inline Key Position::material_key() const {
  return st->materialKey;
}

inline Score Position::psq_score() const {
  return st->psq;
}

inline Value Position::non_pawn_material(Color c) const {
  return st->nonPawnMaterial[c];
}

inline int Position::game_ply() const {
  return gamePly;
}

inline int Position::rule50_count() const {
  return st->rule50;
}

inline uint64_t Position::nodes_searched() const {
  return nodes;
}

inline bool Position::opposite_bishops() const {
  return   pieceCount[W_BISHOP] == 1
        && pieceCount[B_BISHOP] == 1
        && opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

#ifdef ATOMIC
inline bool Position::is_atomic() const {
  return var == ATOMIC_VARIANT;
}

// Loss if king is captured (Atomic)
inline bool Position::is_atomic_win() const {
  return count<KING>(~sideToMove) == 0;
}

// Loss if king is captured (Atomic)
inline bool Position::is_atomic_loss() const {
  return count<KING>(sideToMove) == 0;
}
#endif

#ifdef HORDE
inline bool Position::is_horde() const {
  return var == HORDE_VARIANT;
}

inline bool Position::is_horde_color(Color c) const {
  return pieceCount[make_piece(c, KING)] == 0;
}

// Loss if horde is captured (Horde)
inline bool Position::is_horde_loss() const {
  return count<ALL_PIECES>(is_horde_color(WHITE) ? WHITE : BLACK) == 0;
}
#endif

#ifdef ANTI
inline bool Position::is_anti() const {
  return var == ANTI_VARIANT;
}

inline bool Position::is_anti_loss() const {
  return count<ALL_PIECES>(~sideToMove) == 0;
}

inline bool Position::is_anti_win() const {
  return count<ALL_PIECES>(sideToMove) == 0;
}

inline bool Position::can_capture() const {
  if (ep_square() != SQ_NONE)
      if (attackers_to(ep_square()) & pieces(sideToMove, PAWN))
          return true;
  Bitboard b = pieces(sideToMove);
  while (b)
  {
      Square s = pop_lsb(&b);
      if (attacks_from(piece_on(s), s) & pieces(~sideToMove))
          return true;
  }
  return false;
}
#endif

#ifdef LOSERS
inline bool Position::is_losers() const {
  return var == LOSERS_VARIANT;
}

inline bool Position::is_losers_loss() const {
  return count<ALL_PIECES>(~sideToMove) == 1;
}

inline bool Position::is_losers_win() const {
  return count<ALL_PIECES>(sideToMove) == 1;
}

// Position::can_capture_losers checks whether we have a legal capture
// in a losers chess position.

inline bool Position::can_capture_losers() const {
  // En passent captures
  if (ep_square() != SQ_NONE && !checkers())
      if (attackers_to(ep_square()) & pieces(sideToMove, PAWN) & ~pinned_pieces(sideToMove))
          return true;
  Bitboard b = pieces(sideToMove);
  // Double check forces the king to move
  if (more_than_one(checkers()))
      b &= pieces(sideToMove, KING);
  // Loop over our pieces to find possible captures
  while (b)
  {
      Square s = pop_lsb(&b);
      Bitboard attacked = attacks_from(piece_on(s), s) & pieces(~sideToMove);
      // A pinned piece may only take the pinner
      if (pinned_pieces(sideToMove) & s)
          attacked &= LineBB[s][square<KING>(sideToMove)];
      // The king can only capture undefended pieces
      if (type_of(piece_on(s)) == KING)
      {
          while (attacked)
              if (!(attackers_to(pop_lsb(&attacked)) & pieces(~sideToMove)))
                  return true;
      }
      // If we are in check, any legal capture has to remove the checking piece
      else if (checkers() ? attacked & checkers() : attacked)
          return true;
  }
  return false;
}
#endif

#ifdef SUICIDE
inline bool Position::is_suicide() const {
    return subvar == SUICIDE_VARIANT;
}

inline Value Position::suicide_stalemate(int ply, Value draw) const {
    int balance = popcount(pieces(sideToMove)) - popcount(pieces(~sideToMove));
    if (balance > 0)
        return mated_in(ply);
    if (balance < 0)
        return mate_in(ply + 1);
    return draw;
}
#endif

#ifdef CRAZYHOUSE
inline bool Position::is_house() const {
  return var == CRAZYHOUSE_VARIANT;
}

inline int Position::count_in_hand(Color c, PieceType pt) const {
  return pieceCountInHand[c][pt];
}

inline void Position::add_to_hand(Color c, PieceType pt) {
  pieceCountInHand[c][pt]++;
}

inline void Position::remove_from_hand(Color c, PieceType pt) {
  pieceCountInHand[c][pt]--;
}

inline bool Position::is_promoted(Square s) const {
  return promotedPieces & s;
}
#endif

#ifdef LOOP
inline bool Position::is_loop() const {
  return subvar == LOOP_VARIANT;
}
#endif

#ifdef KOTH
inline bool Position::is_koth() const {
  return var == KOTH_VARIANT;
}

// Win if king is in the center (KOTH)
inline bool Position::is_koth_win() const {
  Square ksq = square<KING>(sideToMove);
  return (rank_of(ksq) == RANK_4 || rank_of(ksq) == RANK_5) &&
         (file_of(ksq) == FILE_D || file_of(ksq) == FILE_E);
}

// Loss if king is in the center (KOTH)
inline bool Position::is_koth_loss() const {
  Square ksq = square<KING>(~sideToMove);
  return (rank_of(ksq) == RANK_4 || rank_of(ksq) == RANK_5) &&
         (file_of(ksq) == FILE_D || file_of(ksq) == FILE_E);
}

inline int Position::koth_distance(Color c) const {
  Square ksq = square<KING>(c);
  return (distance(ksq, SQ_D4) + distance(ksq, SQ_E4) +
          distance(ksq, SQ_D5) + distance(ksq, SQ_E5)) / 4;
}
#endif

#ifdef RACE
inline bool Position::is_race() const {
  return var == RACE_VARIANT;
}

// Win if king is on the eighth rank (Racing Kings)
inline bool Position::is_race_win() const {
  return rank_of(square<KING>(sideToMove)) == RANK_8
        && rank_of(square<KING>(~sideToMove)) < RANK_8;
}

// Draw if kings are on the eighth rank (Racing Kings)
inline bool Position::is_race_draw() const {
  return rank_of(square<KING>(sideToMove)) == RANK_8
        && rank_of(square<KING>(~sideToMove)) == RANK_8;
}

// Loss if king is on the eighth rank (Racing Kings)
inline bool Position::is_race_loss() const {
  if (rank_of(square<KING>(~sideToMove)) != RANK_8)
      return false;
  if (rank_of(square<KING>(sideToMove)) < (sideToMove == WHITE ? RANK_8 : RANK_7))
      return true;
  // Check whether the black king can move to the eighth rank
  Bitboard b = attacks_from<KING>(square<KING>(sideToMove)) & rank_bb(RANK_8) & ~pieces(sideToMove);
  while (b)
      if (!(attackers_to(pop_lsb(&b)) & pieces(~sideToMove)))
          return false;
  return true;
}
#endif

#ifdef RELAY
inline bool Position::is_relay() const {
  return var == RELAY_VARIANT;
}
#endif

inline bool Position::is_chess960() const {
  return chess960;
}

inline Variant Position::variant() const {
  return var;
}

inline Variant Position::subvariant() const {
    return subvar;
}

inline bool Position::capture_or_promotion(Move m) const {
  assert(is_ok(m));
#ifdef RACE
  if (is_race())
  {
    Square from = from_sq(m), to = to_sq(m);
    return (type_of(board[from]) == KING && rank_of(to) >= rank_of(from)) || !empty(to);
  }
#endif
#ifdef CRAZYHOUSE
  return type_of(m) != NORMAL ? type_of(m) != DROP && type_of(m) != CASTLING : !empty(to_sq(m));
#else
  return type_of(m) != NORMAL ? type_of(m) != CASTLING : !empty(to_sq(m));
#endif
}

inline bool Position::capture(Move m) const {
  assert(is_ok(m));
  // Castling is encoded as "king captures rook"
  return (!empty(to_sq(m)) && type_of(m) != CASTLING) || type_of(m) == ENPASSANT;
}

inline Piece Position::captured_piece() const {
  return st->capturedPiece;
}

inline Thread* Position::this_thread() const {
  return thisThread;
}

inline void Position::put_piece(Piece pc, Square s) {

  board[s] = pc;
  byTypeBB[ALL_PIECES] |= s;
  byTypeBB[type_of(pc)] |= s;
  byColorBB[color_of(pc)] |= s;
  index[s] = pieceCount[pc]++;
  pieceList[pc][index[s]] = s;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
}

inline void Position::remove_piece(Piece pc, Square s) {

  // WARNING: This is not a reversible operation. If we remove a piece in
  // do_move() and then replace it in undo_move() we will put it at the end of
  // the list and not in its original place, it means index[] and pieceList[]
  // are not invariant to a do_move() + undo_move() sequence.
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[type_of(pc)] ^= s;
  byColorBB[color_of(pc)] ^= s;
#ifdef ATOMIC
  if (is_atomic())
      board[s] = NO_PIECE;
#endif
  /* board[s] = NO_PIECE;  Not needed, overwritten by the capturing one */
  Square lastSquare = pieceList[pc][--pieceCount[pc]];
  index[lastSquare] = index[s];
  pieceList[pc][index[lastSquare]] = lastSquare;
  pieceList[pc][pieceCount[pc]] = SQ_NONE;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
}

inline void Position::move_piece(Piece pc, Square from, Square to) {

  // index[from] is not updated and becomes stale. This works as long as index[]
  // is accessed just by known occupied squares.
  Bitboard from_to_bb = SquareBB[from] ^ SquareBB[to];
  byTypeBB[ALL_PIECES] ^= from_to_bb;
  byTypeBB[type_of(pc)] ^= from_to_bb;
  byColorBB[color_of(pc)] ^= from_to_bb;
  board[from] = NO_PIECE;
  board[to] = pc;
  index[to] = index[from];
  pieceList[pc][index[to]] = to;
}

#ifdef CRAZYHOUSE
inline void Position::drop_piece(Piece pc, Square s) {
  assert(pieceCountInHand[color_of(pc)][type_of(pc)]);
  put_piece(pc, s);
  pieceCountInHand[color_of(pc)][type_of(pc)]--;
}

inline void Position::undrop_piece(Piece pc, Square s) {
  remove_piece(pc, s);
  board[s] = NO_PIECE;
  pieceCountInHand[color_of(pc)][type_of(pc)]++;
  assert(pieceCountInHand[color_of(pc)][type_of(pc)]);
}
#endif

inline void Position::do_move(Move m, StateInfo& newSt) {
  do_move(m, newSt, gives_check(m));
}

#endif // #ifndef POSITION_H_INCLUDED
