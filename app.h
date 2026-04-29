#ifndef APP_H
#define APP_H

namespace ttt {

const char EMPTY = '0';
const char AI = 'r';
const char USER = 'b';

const int WIN_LINES[8][3] = {
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
    array<char, 9> cell{};

    Board() {
        cell.fill(EMPTY);
    }
};

struct GameResult {
    char winner = EMPTY;
    bool full = false;
    array<int, 3> line = {-1, -1, -1};
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
    int salt = 0;
};

GameResult inspect(const Board& board) {
    GameResult result;

    for (const auto& line : WIN_LINES) {
        char a = board.cell[line[0]];
        if (a != EMPTY && a == board.cell[line[1]] && a == board.cell[line[2]]) {
            result.winner = a;
            result.line = {line[0], line[1], line[2]};
            return result;
        }
    }

    result.full = true;
    for (char c : board.cell) {
        if (c == EMPTY) {
            result.full = false;
            break;
        }
    }

    return result;
}

int lineThreats(const Board& board, char who) {
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

int forks(const Board& board, char who) {
    int count = 0;

    for (int pos = 0; pos < 9; pos++) {
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

int positionValue(int pos) {
    if (pos == 4) {
        return 4;
    }

    if (pos == 0 || pos == 2 || pos == 6 || pos == 8) {
        return 3;
    }

    return 2;
}

vector<int> moveOrder(const Board& board) {
    static const int preferred[9] = {4, 0, 2, 6, 8, 1, 3, 5, 7};
    vector<int> moves;

    for (int pos : preferred) {
        if (board.cell[pos] == EMPTY) {
            moves.push_back(pos);
        }
    }

    return moves;
}

TreeScore solve(Board& board, char turn, int depth) {
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

    vector<int> moves = moveOrder(board);
    vector<TreeScore> children;
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

bool betterAiMove(const MoveScore& a, const MoveScore& b) {
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

    if (a.position != b.position) {
        return a.position > b.position;
    }

    return a.salt > b.salt;
}

MoveScore chooseAiMove(Board& board) {
    static mt19937 rng(static_cast<unsigned int>(
        chrono::steady_clock::now().time_since_epoch().count()));

    MoveScore best;

    for (int move : moveOrder(board)) {
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
        candidate.salt = static_cast<int>(rng() & 0xffff);

        board.cell[move] = EMPTY;

        if (betterAiMove(candidate, best)) {
            best = candidate;
        }
    }

    return best;
}

bool parseBlueMove(const string& token, int& pos) {
    if (token.size() < 2 || token[0] != USER) {
        return false;
    }

    if (token.size() == 2 && token[1] >= '1' && token[1] <= '9') {
        pos = token[1] - '1';
        return true;
    }

    return false;
}

bool onWinningLine(const GameResult& result, int pos) {
    return result.line[0] == pos || result.line[1] == pos || result.line[2] == pos;
}

void render(const Board& board) {
    GameResult result = inspect(board);

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int pos = row * 3 + col;
            char out = board.cell[pos];

            if (result.winner != EMPTY && onWinningLine(result, pos)) {
                out = 'g';
            }

            if (col > 0) {
                cout << ' ';
            }

            cout << out;
        }

        cout << el;
    }
}

void printMoveStats(const MoveScore& score) {
    cout << "r" << (score.move + 1)
         << " | guarantee=";

    if (score.tree.guarantee == 1) {
        cout << "win";
    } else if (score.tree.guarantee == 0) {
        cout << "draw";
    } else {
        cout << "loss";
    }

    cout << " | leaves w/d/l="
         << score.tree.wins << '/'
         << score.tree.draws << '/'
         << score.tree.losses
         << " | threats=" << score.aiThreats
         << " | forks=" << score.aiForks
         << el;
}

void printHelp() {
    cout << "input: b1..b9, q to quit, reset to clear" << el;
    cout << "cells: 1 2 3 / 4 5 6 / 7 8 9" << el;
}

void app() {
    Board board;

    cout << unitbuf;

    cout << "Personalized unbeatable tic tac toe" << el;
    cout << "AI is r, user is b, win LEDs are g." << el;
    printHelp();
    render(board);

    string token;
    while (cin >> token) {
        if (token == "q" || token == "quit" || token == "exit") {
            cout << "bye" << el;
            return;
        }

        if (token == "reset" || token == "new") {
            board = Board();
            render(board);
            continue;
        }

        GameResult before = inspect(board);
        if (before.winner != EMPTY || before.full) {
            cout << "game over, type reset" << el;
            render(board);
            continue;
        }

        int userPos = -1;
        if (!parseBlueMove(token, userPos)) {
            cout << "bad input, use b1..b9" << el;
            continue;
        }

        if (board.cell[userPos] != EMPTY) {
            cout << "cell " << (userPos + 1) << " is busy" << el;
            render(board);
            continue;
        }

        board.cell[userPos] = USER;
        render(board);

        GameResult afterUser = inspect(board);
        if (afterUser.winner == USER) {
            cout << "b wins" << el;
            continue;
        }

        if (afterUser.full) {
            cout << "draw" << el;
            continue;
        }

        MoveScore ai = chooseAiMove(board);
        if (ai.move == -1) {
            cout << "draw" << el;
            continue;
        }

        board.cell[ai.move] = AI;
        printMoveStats(ai);
        render(board);

        GameResult afterAi = inspect(board);
        if (afterAi.winner == AI) {
            cout << "r wins" << el;
        } else if (afterAi.full) {
            cout << "draw" << el;
        }
    }
}

}  // namespace ttt

void app() {
    ttt::app();
}

#endif

// APP_H
