#include "board.hpp"

Board::Board() { load_fen(); }

void Board::make_move(Move move) {
  this->move(move);
  const bool has_legal_moves{this->has_legal_moves()};
  is_in_check_ = is_threatened(king_tiles_[get_color_index(turn_)],
                               get_opposite_color(turn_));
  is_in_checkmate_ = is_in_check_ && !has_legal_moves;
  is_in_draw_ = !is_in_check_ && !has_legal_moves;
  int whiteScore = 0;
  int blackScore = 0;
    for (int i = 0; i < 9; i++) {
      if (get_color(blackBase[i].tile) == PieceColor::White) {
        whiteScore++;
      }
      if (get_color(whiteBase[i].tile) == PieceColor::Black) {
        blackScore++;
      }
    LOGF("SCORES", "White: {} Black: {}", whiteScore, blackScore);
    if (whiteScore == 9 || blackScore == 9) {
      is_in_checkmate_ = true;
    }
  }
}

void Board::undo() {
  if (records_.empty()) {
    return;
  }

  const MoveRecord& record{records_.back()};
  const PieceType moved_type{get_type(record.move.target)};
  set_tile(record.move.tile, get_tile(record.move.target));

  int captured_tile{record.move.target};

  set_tile(captured_tile, record.captured_piece);

  turn_ = get_opposite_color(turn_);
  is_in_checkmate_ = record.is_in_checkmate_;

  records_.pop_back();
}

void Board::generate_all_legal_moves(Moves& moves, bool only_captures) {
  for (int tile = 0; tile < 64; tile++) {
    generate_legal_moves(moves, tile, only_captures);
  }
}

void Board::generate_legal_moves(Moves& moves, int tile, bool only_captures) {
  if (turn_ != get_color(tile)) {
    return;
  }
  auto check_king = [this] {
    return is_threatened(
        king_tiles_[get_color_index(get_opposite_color(turn_))], turn_);
  };
  int end{moves.size};
  generate_moves(moves, tile);
  for (int i = end; i < moves.size; i++) {
    const bool captured{!is_empty(moves.data[i].target)};
    move(moves.data[i]);
    if (!check_king() && (!only_captures || (only_captures && captured))) {
      moves.data[end++] = moves.data[i];
    }
    undo();
  }
  moves.size = end;
}

uint64_t Board::perft(int depth) {
  uint64_t nodes{};
  if (depth == 0) {
    return 1;
  }

  Moves moves;
  generate_all_legal_moves(moves);
  for (int i = 0; i < moves.size; i++) {
    move(moves.data[i]);
    nodes += perft(depth - 1);
    undo();
  }

  return nodes;
}

void Board::load_fen(std::string_view fen) {
  blackBase.clear();
  blackBase.push_back(CornerTile(56, false));
  blackBase.push_back(CornerTile(48, false));
  blackBase.push_back(CornerTile(49, false));
  blackBase.push_back(CornerTile(57, false));
  blackBase.push_back(CornerTile(40, false));
  blackBase.push_back(CornerTile(41, false));
  blackBase.push_back(CornerTile(42, false));
  blackBase.push_back(CornerTile(50, false));
  blackBase.push_back(CornerTile(58, false));

  whiteBase.clear();
  whiteBase.push_back(CornerTile(7, false));
  whiteBase.push_back(CornerTile(6, false));
  whiteBase.push_back(CornerTile(14, false));
  whiteBase.push_back(CornerTile(15, false));
  whiteBase.push_back(CornerTile(5, false));
  whiteBase.push_back(CornerTile(13, false));
  whiteBase.push_back(CornerTile(21, false));
  whiteBase.push_back(CornerTile(22, false));
  whiteBase.push_back(CornerTile(23, false));

  turn_ = {};
  king_tiles_ = {};
  enpassant_tile_ = -1;
  tiles_ = {};
  is_in_checkmate_ = false;
  records_ = {};

  std::array<std::string_view, 6> parts{};
  for (int i = 0, begin = 0, end = 0; i < 6; i++) {
    begin = end;
    end = static_cast<int>(fen.find_first_of(' ', begin + 1));
    parts[i] = fen.substr(begin, end - begin);
    end++;  // Skip whitespace
  }

  for (int i = 0, tile = 0; i < parts[0].length(); i++) {
    const char ch{parts[0][i]};

    if (ch == '/') {
      continue;
    }

    if (ch >= '0' && ch <= '8') {
      tile += ch - '0';
      continue;
    }

    auto color{PieceColor::Black};
    if (ch >= 'A' && ch < 'Z') {
      color = PieceColor::White;
    }

    const int tile_rot{8 * (7 - get_tile_row(tile)) + get_tile_column(tile)};

    PieceType type{};
    switch (ch) {
      case 'P':
      case 'p':
        type = PieceType::Pawn;
        break;
      default:
        break;
    }

    if (type != PieceType::None) {
      set_tile(tile_rot, make_piece(color, type));
    }
    tile++;
  }

  if (parts[1] == "w") {
    turn_ = PieceColor::White;
  } else if (parts[1] == "b") {
    turn_ = PieceColor::Black;
  }



  if (parts[3] != "-") {
    enpassant_tile_ = 8 * (parts[3][1] - '0' - 1) + (parts[3][0] - 'a');
  }
}

void Board::move(Move move) {
  assert(get_color(move.tile) != PieceColor::None &&
         get_type(move.tile) != PieceType::None);

  const MoveRecord& record{records_.emplace_back(
      move, get_tile(move.target)/*,
      enpassant_tile_, is_in_check_*/, is_in_checkmate_)};
  set_tile(move.target, get_tile(move.tile));
  set_tile(move.tile, {});

  turn_ = get_opposite_color(turn_);
  enpassant_tile_ = -1;


  const uint8_t color_index{get_color_index(get_color(move.target))};

  switch (get_type(move.target)) {
    case PieceType::Pawn:
      if (glm::abs(move.target - move.tile) == 16) {
        enpassant_tile_ = (move.tile + move.target) / 2;
      }
      break;
    default:
      break;
  }
}

bool Board::has_legal_moves() {
  Moves moves;
  for (int tile = 0; tile < 64; tile++) {
    generate_legal_moves(moves, tile);
    if (moves.size != 0) {
      return true;
    }
  }
  return false;
}

void Board::generate_moves(Moves& moves, int tile) const {
  const int row{get_tile_row(tile)};
  const int col{get_tile_column(tile)};

  const PieceType tile_type{get_type(tile)};

#define CHECK_MOVE_OFFSET(offset, condition)           \
  if (const int target = tile + (offset); condition) { \
    add_move(moves, tile, target);                     \
  }

#define CHECK_MOVE_DIRECTION(direction, condition)                          \
  for (int target = tile + (direction); condition; target += (direction)) { \
    add_move(moves, tile, target);                                          \
    if (!is_empty(target)) {                                                \
      break;                                                                \
    }                                                                       \
  }

  auto add_move = [this](Moves& moves, int tile, int target) {
    assert(get_color(tile) != PieceColor::None);
    if (get_color(tile) != get_color(target)) {
      moves.data[moves.size++] = {tile, target};
    }
  };

  switch (tile_type) {
    case PieceType::Pawn: {
#define CHECK_PAWN_MOVE_OFFSET(offset, condition)      \
  if (const int target = tile + (offset); condition) { \
    add_pawn_move(moves, tile, target);                \
  }

      auto add_pawn_move = [this](Moves& moves, int tile, int target) {
        assert(get_color(tile) != PieceColor::None);
        if (get_color(tile) != get_color(target)) {
          moves.data[moves.size++] = {tile, target};
          return;
        }
      };

      CHECK_PAWN_MOVE_OFFSET(-8, row != 0 && is_empty(target));
      CHECK_PAWN_MOVE_OFFSET(+8, row != 7 && is_empty(target));
      CHECK_PAWN_MOVE_OFFSET(-1, col != 0 && is_empty(target));
      CHECK_PAWN_MOVE_OFFSET(+1, col != 7 && is_empty(target));

      switch (get_color(tile)) {
        case PieceColor::Black:
          // Code if color of the pawn will matter
          break;
        case PieceColor::White:
          // Code if color of the pawn will matter
          break;
        default:
          break;
      }
    }

#undef CHECK_PAWN_MOVE_OFFSET
    break;
    // Code for potential other types of pieces
    default:
      break;
  }

#undef CHECK_MOVE_OFFSET
#undef CHECK_MOVE_DIRECTION
}

bool Board::is_threatened(int tile, PieceColor attacker_color) const {
  const int row{get_tile_row(tile)};
  const int col{get_tile_column(tile)};

  return false;
}
