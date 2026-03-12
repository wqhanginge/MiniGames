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


class SudokuMap : public std::array<char, MAP> {
    size_t _doff(size_t offset) const { return static_cast<size_t>((*this)[offset] - BLANK); }

public:
    SudokuMap() { fill(BLANK); }
    explicit SudokuMap(std::string_view map) :SudokuMap() {
        for (size_t i = 0; i < std::min(MAP, map.size()); i++) {
            (*this)[i] = DIGITS[std::min(static_cast<size_t>(map[i] - BLANK), DSIZE) % DSIZE];
        }
    }

    constexpr explicit operator std::string_view() const { return std::string_view(data(), MAP); }
    bool unique(const size_t offset) const {
        if (offset >= MAP) throw std::out_of_range("invalid map offset");
        size_t r = offset / ROW, c = offset % ROW, b = ((r / SEG) * ROW + (c / SEG)) * SEG;
        for (size_t i = 0, flags = 0; i < ROW; i++) {
            size_t ir = _doff(r * ROW + i), ic = _doff(i * ROW + c), ib = _doff(b + (i / SEG) * ROW + (i % SEG));
            size_t mask = (0x00000001ULL << ir) | (0x00000400ULL << ic) | (0x00100000ULL << ib);
            if (flags & mask & 0x3FEFFBFEULL) return false;
            flags |= mask;
        }
        return true;
    }
};

using BlankList = std::vector<size_t>;
using BlankIter = BlankList::iterator;


bool _solveRecursive(SudokuMap& map, const BlankIter it, const BlankIter end) {
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
    BlankList blanks;
    for (size_t off = 0; off < solution.size(); off++) {
        if (solution[off] == BLANK) blanks.push_back(off);
    }
    _solveRecursive(solution, blanks.begin(), blanks.end());
    return solution;
}


int main() {
    SudokuMap smap("007008000095002600000100250010000079600009000008004000000000000006400800030050020");
    std::cout << std::string_view(smap) << '\n' << std::string_view(solve(smap)) << std::endl;
    try { std::cout << smap.unique(-1) << std::endl; }
    catch (std::exception& e) { std::cout << e.what() << std::endl; }
    return 0;
}
