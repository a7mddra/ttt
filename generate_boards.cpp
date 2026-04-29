#include <fstream>
#include <iostream>
#include <string>

#include "engine.h"

namespace {

char cellToDigit(std::uint8_t cell) {
    return static_cast<char>('0' + cell);
}

char cellToCli(std::uint8_t cell) {
    if (cell == ttt::RED) {
        return 'r';
    }

    if (cell == ttt::BLUE) {
        return 'b';
    }

    return '0';
}

std::string boardString(const ttt::Board& board, char (*convert)(std::uint8_t)) {
    std::string out;
    out.reserve(ttt::BOARD_CELLS);

    for (std::uint8_t cell : board.cell) {
        out.push_back(convert(cell));
    }

    return out;
}

}  // namespace

int main() {
    std::ofstream out("boards.txt");
    if (!out) {
        std::cerr << "failed to open boards.txt for writing\n";
        return 1;
    }

    int count = 0;
    for (int key = 0; key < ttt::BOARD_COUNT; key++) {
        ttt::Board board = ttt::decodeBoard(static_cast<std::uint16_t>(key));
        if (!ttt::isLegalReachable(board)) {
            continue;
        }

        out << key << '\t'
            << boardString(board, cellToDigit) << '\t'
            << boardString(board, cellToCli) << '\n';
        count++;
    }

    std::cout << "generated " << count
              << " legal reachable boards to boards.txt\n";
    return 0;
}
