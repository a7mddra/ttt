#include <cstdint>
#include <iostream>
#include <string>

#include "engine.h"

namespace {

bool parseCell(char ch, std::uint8_t& out) {
    if (ch == '0') {
        out = ttt::EMPTY;
        return true;
    }

    if (ch == '1' || ch == 'r' || ch == 'R') {
        out = ttt::RED;
        return true;
    }

    if (ch == '2' || ch == 'b' || ch == 'B') {
        out = ttt::BLUE;
        return true;
    }

    return false;
}

bool parseBoardText(const std::string& text, ttt::Board& board) {
    if (text.size() != ttt::BOARD_CELLS) {
        return false;
    }

    for (int i = 0; i < ttt::BOARD_CELLS; i++) {
        if (!parseCell(text[i], board.cell[i])) {
            return false;
        }
    }

    return true;
}

bool parseKey(const std::string& text, std::uint16_t& key) {
    std::size_t used = 0;
    unsigned long value = 0;

    try {
        value = std::stoul(text, &used, 10);
    } catch (...) {
        return false;
    }

    if (used != text.size() || value >= ttt::BOARD_COUNT) {
        return false;
    }

    key = static_cast<std::uint16_t>(value);
    return true;
}

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

void printUsage(const char* argv0) {
    std::cerr << "usage:\n"
              << "  " << argv0 << " encode <9-cell-board>\n"
              << "  " << argv0 << " decode <0..19682>\n"
              << "\n"
              << "cells: 0=empty, 1/r=red AI, 2/b=blue user\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    std::string value = argv[2];

    if (command == "encode") {
        ttt::Board board;
        if (!parseBoardText(value, board)) {
            std::cerr << "bad board: expected exactly 9 cells using 0/r/b or 0/1/2\n";
            return 1;
        }

        std::cout << ttt::encodeBoard(board) << '\n';
        return 0;
    }

    if (command == "decode") {
        std::uint16_t key = 0;
        if (!parseKey(value, key)) {
            std::cerr << "bad key: expected a number from 0 to 19682\n";
            return 1;
        }

        ttt::Board board = ttt::decodeBoard(key);
        std::cout << "digits: " << boardString(board, cellToDigit) << '\n';
        std::cout << "cli: " << boardString(board, cellToCli) << '\n';
        return 0;
    }

    printUsage(argv[0]);
    return 1;
}
