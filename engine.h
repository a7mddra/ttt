#ifndef ENGINE_H
#define ENGINE_H

#include <array>
#include <cstdint>
#include <vector>

namespace ttt {

inline constexpr std::uint8_t EMPTY = 0;
inline constexpr std::uint8_t RED = 1;
inline constexpr std::uint8_t BLUE = 2;
inline constexpr std::uint8_t AI = RED;
inline constexpr std::uint8_t USER = BLUE;

inline constexpr int BOARD_CELLS = 9;
inline constexpr int BOARD_COUNT = 19683;
inline constexpr std::uint8_t NO_MOVE = 15;
inline constexpr std::uint8_t PACKED_INVALID = 0xff;

inline constexpr int WIN_LINES[8][3] = {
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},
    {0, 4, 8},
    {2, 4, 6},
};

struct Board {
    std::array<std::uint8_t, BOARD_CELLS> cell{};

    Board() {
        cell.fill(EMPTY);
    }
};

struct GameResult {
    std::uint8_t winner = EMPTY;
    bool full = false;
    std::array<int, 3> line = {-1, -1, -1};
};

struct TreeScore {
    int guarantee = 0;      // AI win = +1, draw = 0, AI loss = -1.
    int terminalDepth = 0;  // Smaller for wins, larger for losses.
    long long wins = 0;
    long long draws = 0;
    long long losses = 0;
};

struct MoveScore {
    int move = -1;
    TreeScore tree;
    int aiThreats = 0;
    int aiForks = 0;
    int userThreats = 0;
    int userForks = 0;
    int position = 0;
};

inline GameResult inspect(const Board& board) {
    GameResult result;

    for (const auto& line : WIN_LINES) {
        std::uint8_t a = board.cell[line[0]];
        if (a != EMPTY && a == board.cell[line[1]] && a == board.cell[line[2]]) {
            result.winner = a;
            result.line = {line[0], line[1], line[2]};
            return result;
        }
    }

    result.full = true;
    for (std::uint8_t c : board.cell) {
        if (c == EMPTY) {
            result.full = false;
            break;
        }
    }

    return result;
}

inline int lineThreats(const Board& board, std::uint8_t who) {
    int count = 0;

    for (const auto& line : WIN_LINES) {
        int mine = 0;
        int blank = 0;

        for (int idx : line) {
            if (board.cell[idx] == who) {
                mine++;
            } else if (board.cell[idx] == EMPTY) {
                blank++;
            }
        }

        if (mine == 2 && blank == 1) {
            count++;
        }
    }

    return count;
}

inline int forks(const Board& board, std::uint8_t who) {
    int count = 0;

    for (int pos = 0; pos < BOARD_CELLS; pos++) {
        if (board.cell[pos] != EMPTY) {
            continue;
        }

        Board next = board;
        next.cell[pos] = who;

        if (inspect(next).winner == who) {
            continue;
        }

        if (lineThreats(next, who) >= 2) {
            count++;
        }
    }

    return count;
}

inline int positionValue(int pos) {
    if (pos == 4) {
        return 4;
    }

    if (pos == 0 || pos == 2 || pos == 6 || pos == 8) {
        return 3;
    }

    return 2;
}

inline std::vector<int> moveOrder(const Board& board) {
    static constexpr int preferred[BOARD_CELLS] = {4, 0, 2, 6, 8, 1, 3, 5, 7};
    std::vector<int> moves;

    for (int pos : preferred) {
        if (board.cell[pos] == EMPTY) {
            moves.push_back(pos);
        }
    }

    return moves;
}

inline TreeScore solve(Board& board, std::uint8_t turn, int depth) {
    GameResult state = inspect(board);

    if (state.winner == AI) {
        return {1, depth, 1, 0, 0};
    }

    if (state.winner == USER) {
        return {-1, depth, 0, 0, 1};
    }

    if (state.full) {
        return {0, depth, 0, 1, 0};
    }

    std::vector<int> moves = moveOrder(board);
    std::vector<TreeScore> children;
    children.reserve(moves.size());

    long long wins = 0;
    long long draws = 0;
    long long losses = 0;

    for (int move : moves) {
        board.cell[move] = turn;
        TreeScore child = solve(board, turn == AI ? USER : AI, depth + 1);
        board.cell[move] = EMPTY;

        wins += child.wins;
        draws += child.draws;
        losses += child.losses;
        children.push_back(child);
    }

    TreeScore best = children.front();

    for (const TreeScore& child : children) {
        bool better = false;

        if (turn == AI) {
            if (child.guarantee != best.guarantee) {
                better = child.guarantee > best.guarantee;
            } else if (child.guarantee == 1) {
                better = child.terminalDepth < best.terminalDepth;
            } else if (child.guarantee == -1) {
                better = child.terminalDepth > best.terminalDepth;
            } else {
                better = child.wins > best.wins ||
                         (child.wins == best.wins && child.losses < best.losses);
            }
        } else {
            if (child.guarantee != best.guarantee) {
                better = child.guarantee < best.guarantee;
            } else if (child.guarantee == -1) {
                better = child.terminalDepth < best.terminalDepth;
            } else if (child.guarantee == 1) {
                better = child.terminalDepth > best.terminalDepth;
            } else {
                better = child.losses > best.losses ||
                         (child.losses == best.losses && child.wins < best.wins);
            }
        }

        if (better) {
            best = child;
        }
    }

    best.wins = wins;
    best.draws = draws;
    best.losses = losses;
    return best;
}

inline bool betterAiMove(const MoveScore& a, const MoveScore& b) {
    if (b.move == -1) {
        return true;
    }

    if (a.tree.guarantee != b.tree.guarantee) {
        return a.tree.guarantee > b.tree.guarantee;
    }

    if (a.tree.guarantee == 1 && a.tree.terminalDepth != b.tree.terminalDepth) {
        return a.tree.terminalDepth < b.tree.terminalDepth;
    }

    if (a.tree.guarantee == -1 && a.tree.terminalDepth != b.tree.terminalDepth) {
        return a.tree.terminalDepth > b.tree.terminalDepth;
    }

    if (a.tree.wins != b.tree.wins) {
        return a.tree.wins > b.tree.wins;
    }

    if (a.aiForks != b.aiForks) {
        return a.aiForks > b.aiForks;
    }

    if (a.aiThreats != b.aiThreats) {
        return a.aiThreats > b.aiThreats;
    }

    if (a.userThreats != b.userThreats) {
        return a.userThreats < b.userThreats;
    }

    if (a.userForks != b.userForks) {
        return a.userForks < b.userForks;
    }

    if (a.tree.losses != b.tree.losses) {
        return a.tree.losses < b.tree.losses;
    }

    if (a.tree.draws != b.tree.draws) {
        return a.tree.draws < b.tree.draws;
    }

    return a.position > b.position;
}

inline MoveScore scoreAiMove(Board& board, int move) {
    board.cell[move] = AI;
    TreeScore tree = solve(board, USER, 1);

    MoveScore candidate;
    candidate.move = move;
    candidate.tree = tree;
    candidate.aiThreats = lineThreats(board, AI);
    candidate.aiForks = forks(board, AI);
    candidate.userThreats = lineThreats(board, USER);
    candidate.userForks = forks(board, USER);
    candidate.position = positionValue(move);

    board.cell[move] = EMPTY;
    return candidate;
}

inline MoveScore chooseAiMove(Board& board) {
    MoveScore best;

    for (int move : moveOrder(board)) {
        MoveScore candidate = scoreAiMove(board, move);
        if (betterAiMove(candidate, best)) {
            best = candidate;
        }
    }

    return best;
}

inline bool samePerfectScore(const MoveScore& a, const MoveScore& b) {
    return a.tree.guarantee == b.tree.guarantee &&
           a.tree.terminalDepth == b.tree.terminalDepth;
}

inline std::uint8_t moveNibble(int move) {
    if (move >= 0 && move <= 8) {
        return static_cast<std::uint8_t>(move);
    }

    return NO_MOVE;
}

inline std::uint8_t packMoves(int move1, int move2) {
    return static_cast<std::uint8_t>((moveNibble(move1) << 4) | moveNibble(move2));
}

inline std::uint8_t primaryMove(std::uint8_t packed) {
    return static_cast<std::uint8_t>(packed >> 4);
}

inline std::uint8_t alternateMove(std::uint8_t packed) {
    return static_cast<std::uint8_t>(packed & 0x0f);
}

inline std::uint16_t encodeBoard(const Board& board) {
    std::uint16_t key = 0;
    std::uint16_t pow3 = 1;

    for (std::uint8_t cell : board.cell) {
        key = static_cast<std::uint16_t>(key + cell * pow3);
        pow3 = static_cast<std::uint16_t>(pow3 * 3);
    }

    return key;
}

inline Board decodeBoard(std::uint16_t key) {
    Board board;

    for (int i = 0; i < BOARD_CELLS; i++) {
        board.cell[i] = static_cast<std::uint8_t>(key % 3);
        key = static_cast<std::uint16_t>(key / 3);
    }

    return board;
}

inline int countCells(const Board& board, std::uint8_t who) {
    int count = 0;

    for (std::uint8_t cell : board.cell) {
        if (cell == who) {
            count++;
        }
    }

    return count;
}

inline bool isAiTurnBoard(const Board& board) {
    return countCells(board, USER) == countCells(board, AI) + 1;
}

inline void markReachableBoards(Board& board,
                                std::uint8_t turn,
                                std::array<bool, BOARD_COUNT>& reachable) {
    reachable[encodeBoard(board)] = true;

    GameResult state = inspect(board);
    if (state.winner != EMPTY || state.full) {
        return;
    }

    for (int pos = 0; pos < BOARD_CELLS; pos++) {
        if (board.cell[pos] != EMPTY) {
            continue;
        }

        board.cell[pos] = turn;
        markReachableBoards(board, turn == USER ? AI : USER, reachable);
        board.cell[pos] = EMPTY;
    }
}

inline const std::array<bool, BOARD_COUNT>& reachableBoards() {
    static const std::array<bool, BOARD_COUNT> reachable = [] {
        std::array<bool, BOARD_COUNT> built{};
        Board board;
        markReachableBoards(board, USER, built);
        return built;
    }();

    return reachable;
}

inline bool isLegalReachable(const Board& board) {
    return reachableBoards()[encodeBoard(board)];
}

inline std::uint8_t choosePackedAiMoves(Board& board) {
    GameResult state = inspect(board);
    if (!isLegalReachable(board) || !isAiTurnBoard(board) ||
        state.winner != EMPTY || state.full) {
        return PACKED_INVALID;
    }

    MoveScore best = chooseAiMove(board);
    if (best.move == -1) {
        return PACKED_INVALID;
    }

    MoveScore alternate;
    for (int move : moveOrder(board)) {
        if (move == best.move) {
            continue;
        }

        MoveScore candidate = scoreAiMove(board, move);
        if (samePerfectScore(candidate, best) && betterAiMove(candidate, alternate)) {
            alternate = candidate;
        }
    }

    return packMoves(best.move, alternate.move == -1 ? NO_MOVE : alternate.move);
}

}  // namespace ttt

#endif
