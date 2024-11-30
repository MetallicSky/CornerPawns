#pragma once

#include <mutex>
#include <chrono>
#include <iostream>

#include "piece.hpp"
#include "vector"


constexpr bool is_valid_tile(int tile) { return 0 <= tile && tile <= 63; }

constexpr int get_tile_row(int tile) {
  assert(is_valid_tile(tile));
  return static_cast<uint8_t>(tile) >> 3U;
}

constexpr int get_tile_column(int tile) {
  assert(is_valid_tile(tile));
  return static_cast<uint8_t>(tile) & 7U;
}

struct Move {
  int tile{-1};
  int target{-1};
  //PieceType promotion{};
};

struct Moves {
  int size{};
  std::array<Move, 256> data{};
};

struct CornerTile {
  CornerTile(const int& tile, const bool occupied) {
    this->tile = tile;
    this->occupied = occupied;
  }
  int tile;
  bool occupied;
};

class Board {
  struct MoveRecord {
    Move move;
    Piece captured_piece{};
    bool is_in_checkmate_{};
  };

  using Records = std::vector<MoveRecord>;

  static constexpr std::string_view k_initial_fen{
      // "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"}; //
      // Original chess
      "ppp5/ppp5/ppp5/8/8/5PPP/5PPP/5PPP w KQkq - 0 1"};  // Corner pawns

 public:
  Board();

  void make_move(Move move);
  void undo();

  void generate_all_legal_moves(Moves& moves, bool only_captures = false);
  void generate_legal_moves(Moves& moves, int tile, bool only_captures = false);

  [[nodiscard]] bool is_in_check() const { return is_in_check_; }
  [[nodiscard]] bool is_in_checkmate() const { return is_in_checkmate_; }
  [[nodiscard]] bool is_in_draw() const { return is_in_draw_; }

  uint64_t perft(int depth);

  void load_fen(std::string_view fen = k_initial_fen);

  [[nodiscard]] PieceColor get_turn() const { return turn_; }
  /// <summary>
  /// Returns Piece located on the given tile
  /// </summary>
  /// <param name="tile">Tile index</param>
  [[nodiscard]] Piece get_tile(int tile) const { return tiles_[tile]; }
  // clang-format off
  
  /// <summary>
  /// Returns (PieceColor::) White or Black
  /// </summary>
  /// <param name="tile">Tile index</param>
  [[nodiscard]] PieceColor get_color(int tile) const { return get_piece_color(get_tile(tile)); }

  [[nodiscard]] PieceType get_type(int tile) const { return get_piece_type(get_tile(tile)); }

  [[nodiscard]] bool is_empty(int tile) const { return get_piece_type(get_tile(tile)) == PieceType::None; }
  [[nodiscard]] bool is_piece(int tile, PieceColor color, PieceType type) const { return get_color(tile) == color && get_type(tile) == type; }
  // clang-format on
  [[nodiscard]] const Records& get_records() const { return records_; }

  Board& operator=(const Board& other) {
    if (this != &other) {  // Avoid self-assignment
      this->blackBase = other.blackBase;
      this->whiteBase = other.whiteBase;
      this->turn_ = other.turn_;
      this->king_tiles_ = other.king_tiles_;
      this->tiles_ = other.tiles_;
      this->is_in_check_ = other.is_in_check_;
      this->is_in_checkmate_ = other.is_in_checkmate_;
      this->is_in_draw_ = other.is_in_draw_;
      this->records_ = other.records_;
      // Repeat for all members...
    }
    return *this;
  }
 private:
  void set_tile(int tile, Piece piece) { tiles_[tile] = piece; }

  void move(Move move);
  bool has_legal_moves();

  void generate_moves(Moves& moves, int tile) const;
  [[nodiscard]] bool is_threatened(int tile, PieceColor attacker_color) const;

  std::vector<CornerTile> blackBase;
  std::vector<CornerTile> whiteBase;
  mutable std::mutex score_mutex_;

  PieceColor turn_{};
  std::array<int, 2> king_tiles_{};
  int enpassant_tile_{-1};
  std::array<Piece, 64> tiles_{};
  bool is_in_check_{};
  bool is_in_checkmate_{};
  bool is_in_draw_{};
  Records records_;
};
