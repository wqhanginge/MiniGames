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
};

using OffsetList = std::vector<size_t>;
using OffsetIter = OffsetList::iterator;


bool _solveRecursive(SudokuMap& map, const OffsetIter it, const OffsetIter end) {
    if (it == end) return true;
    for (auto digit : DIGITS.substr(1)) {
        map[*it] = digit;
        if (map.unique(*it) && _solveRecursive(map, it + 1, end)) return true;
    }
    map[*it] = BLANK;
    return false;
}

SudokuMap solve(const SudokuMap& map) {
    SudokuMap solution = map;
    OffsetList offlist;
    for (size_t off = 0; off < solution.size(); off++) {
        if (solution[off] == BLANK) offlist.push_back(off);
    }
    _solveRecursive(solution, offlist.begin(), offlist.end());
    return solution;
}


int main() {
    SudokuMap smap("007008000095002600000100250010000079600009000008004000000000000006400800030050020");
    std::cout << std::string_view(smap) << '\n' << std::string_view(solve(smap)) << std::endl;
    try { std::cout << smap.unique(-1) << std::endl; }
    catch (std::exception& e) { std::cout << e.what() << std::endl; }
    return 0;
}
