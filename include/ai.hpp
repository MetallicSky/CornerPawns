#pragma once

#include <functional>
#include <thread>

#include "board.hpp"

class AI {
  // clang-format off
  static constexpr std::array wBase{
    62,  67,  75,  78,  82,  90,  95, 100,
    60,  63,  68,  75,  79,  85,  90,  95,
    55,  60,  64,  69,  75,  80,  85,  90,
    50,  55,  60,  65,  70,  75,  79,  82,
    45,  50,  55,  60,  65,  69,  75,  78,
    40,  45,  50,  55,  60,  64,  68,  75,
    35,  40,  45,  50,  55,  60,  63,  67,
    30,  35,  40,  45,  50,  55,  60,  62
  };
  static constexpr std::array bBase{
    62,  60, 55, 50, 45, 40, 35, 30,
    67,  63, 60, 55, 50, 45, 40, 35,
    75,  68, 64, 60, 55, 50, 45, 40,
    78,  75, 69, 65, 60, 55, 50, 45,
    82,  79, 75, 70, 65, 60, 55, 50,
    90,  85, 80, 75, 69, 64, 60, 55,
    95,  90, 85, 79, 75, 68, 63, 60,
    100, 95, 90, 82, 78, 75, 67, 62
  };
  // clang-format on

 public:
  AI() : worker_{std::bind_front(&AI::run, this)} {
    LOG("AI", "Thread started");
  }

  void think(const Board& board, const std::vector<CornerTile>& blackBase,
             const std::vector<CornerTile>& whiteBase);

  Move get_best_move();
  [[nodiscard]] bool is_thinking() const { return thinking_; }
  [[nodiscard]] bool has_found_move() const { return found_move_; }

 private:
  void run(const std::stop_token& stop_token);

  void search();

  void order_moves(Moves& moves) const;

  Move best_move_;
  Board board_;
  std::vector<CornerTile> blackBase_;
  std::vector<CornerTile> whiteBase_;

  std::atomic<bool> thinking_;
  std::atomic<bool> found_move_;

  std::jthread worker_;
};
