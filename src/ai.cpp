#include "ai.hpp"

#include <chrono>

#include "board.hpp"

void AI::think(const Board& board, const std::vector<CornerTile>& blackBase,
               const std::vector<CornerTile>& whiteBase) {
  assert(!thinking_);
  board_ = board;
  blackBase_ = blackBase;
  whiteBase_ = whiteBase;
  found_move_ = false;
  thinking_ = true;
}

Move AI::get_best_move() {
  assert(found_move_);
  found_move_ = false;
  return best_move_;
}

void AI::run(const std::stop_token& stop_token) {
  while (!stop_token.stop_requested()) {
    std::this_thread::sleep_for(1s);
    if (thinking_ && !found_move_) {
      auto start{std::chrono::high_resolution_clock::now()};
      int score{-1000000};
      for (int i = 1;; i++) {
        score = std::max(score, search(i, -1000000, 1000000));
        auto now{std::chrono::high_resolution_clock::now()};
        auto ms{
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start)};
        LOGF("AI", "Depth: {} Time: {}ms", i, ms.count());
        if (score >= 100000 || ms.count() > 500) {
          break;
        }
      }
      thinking_ = false;
      found_move_ = true;
    }
  }
  LOG("AI", "Thread stopped");
}

int AI::search(int depth, int alpha, int beta) {
  if (depth == 0 || board_.is_in_checkmate()) {
    return quiesce(alpha, beta);
  }
  int max{-1000000};
  Move best_move;
  Moves all_legal_moves;
  board_.generate_all_legal_moves(all_legal_moves);
  assert(all_legal_moves.size != 0);
  order_moves(all_legal_moves);
  best_move_ = all_legal_moves.data[0];
  return max;
}

int AI::quiesce(int alpha, int beta) {
  int score{evaluate()};

  if (score >= beta) {
    return beta;
  }
  if (alpha < score) {
    alpha = score;
  }

  Moves all_legal_moves;
  board_.generate_all_legal_moves(all_legal_moves, true);
  order_moves(all_legal_moves);
  for (int i = 0; i < all_legal_moves.size; i++) {
    const Move& move = all_legal_moves.data[i];
    board_.make_move(move);
    score = -quiesce(-beta, -alpha);
    board_.undo();

    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  return alpha;
}

int AI::evaluate() const {
  if (board_.is_in_checkmate()) {
    return -500000;
  }

  const auto& base_table =
      (board_.get_turn() == PieceColor::White) ? bBase : wBase;

  int score{};
  for (int tile = 0; tile < 64; tile++) {
    if (board_.is_empty(tile)) {
      continue;
    }
    const PieceColor color = board_.get_color(tile);
    if (board_.get_type(tile) == PieceType::Pawn) {
      // Evaluate based on position in the base table
      score += base_table[tile];
    }
  }
  return score;
}

void AI::order_moves(Moves& moves) const {
  const auto& base_table =
      (board_.get_turn() == PieceColor::White) ? bBase : wBase;
  const auto& begin{moves.data.begin()};

  std::sort(begin, begin + moves.size,
            [this, &base_table](const Move& left, const Move& right) {
              int left_value_before = base_table[left.tile];
              int left_value_after = base_table[left.target];
              int right_value_before = base_table[right.tile];
              int right_value_after = base_table[right.target];

              // Check if moves decrease value
              bool left_decreases = left_value_after < left_value_before;
              bool right_decreases = right_value_after < right_value_before;

              // Prioritize non-decreasing moves over decreasing moves
              if (left_decreases != right_decreases) {
                return !left_decreases;  // true for non-decreasing moves, false
                                         // otherwise
              }

              // For non-decreasing moves, prioritize:
              // 1. Lower initial tile value
              // 2. Higher value improvement
              if (!left_decreases && !right_decreases) {
                if (left_value_before != right_value_before) {
                  return left_value_before < right_value_before;
                }
                return (left_value_after - left_value_before) >
                       (right_value_after - right_value_before);
              }

              // For decreasing moves, prioritize minimizing the decrease
              return (left_value_after - left_value_before) >
                     (right_value_after - right_value_before);
            });
}
