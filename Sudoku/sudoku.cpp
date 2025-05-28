#include <iostream>
#include <fstream>

#include <array>
#include <vector>
#include <string>

#include <cstring>
#include <string_view>


constexpr size_t SEG = 3, ROW = 9, MAP = 81;
constexpr std::string_view DIGITS = "0123456789";
constexpr size_t DSIZE = DIGITS.size();
constexpr char BLANK = DIGITS[0];


class SudokuMap :public std::array<char, MAP> {
    using _Base = std::array<char, MAP>;
    size_t _doff(size_t offset) const { return static_cast<size_t>(_Base::at(offset) - BLANK); }
    size_t _next(size_t offset) const { while (offset < MAP && _doff(offset)) offset++; return offset; }

    bool _solve(size_t offset) {
        if (offset >= MAP) return true;
        for (auto digit : DIGITS.substr(1)) {
            _Base::at(offset) = digit;
            if (unique(offset) && _solve(_next(offset))) return true;
        }
        _Base::at(offset) = BLANK;
        return false;
    }

public:
    SudokuMap() { _Base::fill(BLANK); }
    explicit SudokuMap(std::string_view map) :SudokuMap() {
        for (size_t i = 0; i < std::min(MAP, map.size()); i++) {
            _Base::at(i) = DIGITS[std::min(static_cast<size_t>(map[i] - BLANK), DSIZE) % DSIZE];
        }
    }

    constexpr explicit operator std::string_view() const { return std::string_view(_Base::data(), MAP); }
    bool unique(const size_t offset) const {
        size_t x = offset / ROW, y = offset % ROW, b = ((x / SEG) * ROW + (y / SEG)) * SEG;
        bool duplicate = false, flags[3][DSIZE] = {};
        for (size_t i = 0; i < ROW; i++) {
            size_t ix = _doff(x * ROW + i), iy = _doff(i * ROW + y), ib = _doff(b + (i / SEG) * ROW + (i % SEG));
            duplicate |= flags[0][ix] || flags[1][iy] || flags[2][ib];
            flags[0][ix] |= static_cast<bool>(ix);
            flags[1][iy] |= static_cast<bool>(iy);
            flags[2][ib] |= static_cast<bool>(ib);
        }
        return !duplicate;
    }

    SudokuMap& solve() { _solve(_next(0)); return *this; }
};


int main() {
    SudokuMap smap("007008000095002600000100250010000079600009000008004000000000000006400800030050020");
    std::cout << std::string_view(smap) << '\n' << std::string_view(smap.solve());
    return 0;
}
