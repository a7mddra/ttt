#ifndef APP_H
#define APP_H

#include <iostream>
#include <string>

#include "engine.h"

namespace ttt {

inline char cellToCli(std::uint8_t cell) {
    if (cell == AI) {
        return 'r';
    }

    if (cell == USER) {
        return 'b';
    }

    return '0';
}

inline bool parseBlueMove(const std::string& token, int& pos) {
    if (token.size() != 2 || token[0] != 'b') {
        return false;
    }

    if (token[1] >= '1' && token[1] <= '9') {
        pos = token[1] - '1';
        return true;
    }

    return false;
}

inline bool onWinningLine(const GameResult& result, int pos) {
    return result.line[0] == pos || result.line[1] == pos || result.line[2] == pos;
}

inline void render(const Board& board) {
    GameResult result = inspect(board);

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int pos = row * 3 + col;
            char out = cellToCli(board.cell[pos]);

            if (result.winner != EMPTY && onWinningLine(result, pos)) {
                out = 'g';
            }

            if (col > 0) {
                std::cout << ' ';
            }

            std::cout << out;
        }

        std::cout << '\n';
    }
}

inline void printMoveStats(const MoveScore& score) {
    std::cout << "r" << (score.move + 1)
              << " | guarantee=";

    if (score.tree.guarantee == 1) {
        std::cout << "win";
    } else if (score.tree.guarantee == 0) {
        std::cout << "draw";
    } else {
        std::cout << "loss";
    }

    std::cout << " | leaves w/d/l="
              << score.tree.wins << '/'
              << score.tree.draws << '/'
              << score.tree.losses
              << " | threats=" << score.aiThreats
              << " | forks=" << score.aiForks
              << '\n';
}

inline void printHelp() {
    std::cout << "input: b1..b9, q to quit, reset to clear\n";
    std::cout << "cells: 1 2 3 / 4 5 6 / 7 8 9\n";
}

inline void app() {
    Board board;

    std::cout << std::unitbuf;

    std::cout << "Personalized unbeatable tic tac toe\n";
    std::cout << "AI is r, user is b, win LEDs are g.\n";
    printHelp();
    render(board);

    std::string token;
    while (std::cin >> token) {
        if (token == "q" || token == "quit" || token == "exit") {
            std::cout << "bye\n";
            return;
        }

        if (token == "reset" || token == "new") {
            board = Board();
            render(board);
            continue;
        }

        GameResult before = inspect(board);
        if (before.winner != EMPTY || before.full) {
            std::cout << "game over, type reset\n";
            render(board);
            continue;
        }

        int userPos = -1;
        if (!parseBlueMove(token, userPos)) {
            std::cout << "bad input, use b1..b9\n";
            continue;
        }

        if (board.cell[userPos] != EMPTY) {
            std::cout << "cell " << (userPos + 1) << " is busy\n";
            render(board);
            continue;
        }

        board.cell[userPos] = USER;
        render(board);

        GameResult afterUser = inspect(board);
        if (afterUser.winner == USER) {
            std::cout << "b wins\n";
            continue;
        }

        if (afterUser.full) {
            std::cout << "draw\n";
            continue;
        }

        MoveScore ai = chooseAiMove(board);
        if (ai.move == -1) {
            std::cout << "draw\n";
            continue;
        }

        board.cell[ai.move] = AI;
        printMoveStats(ai);
        render(board);

        GameResult afterAi = inspect(board);
        if (afterAi.winner == AI) {
            std::cout << "r wins\n";
        } else if (afterAi.full) {
            std::cout << "draw\n";
        }
    }
}

}  // namespace ttt

inline void app() {
    ttt::app();
}

#endif
