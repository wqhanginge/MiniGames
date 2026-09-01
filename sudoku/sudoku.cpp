#include <iostream>
#include <fstream>

#include <array>
#include <vector>
#include <string>

#include <random>
#include <cstring>
#include <string_view>


constexpr size_t SEG = 3, ROW = 9, MAP = 81, CNT = 10;
constexpr std::string_view ITEMS = "0123456789";
constexpr std::string_view DIGIT = ITEMS.substr(1);
constexpr char BLANK = ITEMS[0];

static_assert(ITEMS.size() == CNT, "insufficient items");


class SudokuMap : public std::array<char, MAP> {
    size_t _doff(size_t offset) const { return static_cast<size_t>((*this)[offset] - BLANK); }

public:
    SudokuMap() { fill(BLANK); }
    explicit SudokuMap(std::string_view map) : SudokuMap() {
        for (size_t i = 0; i < std::min(MAP, map.size()); i++) {
            (*this)[i] = ITEMS[std::min(static_cast<size_t>(map[i] - BLANK), CNT) % CNT];
        }
    }

    constexpr explicit operator std::string_view() const { return std::string_view{ data(), MAP }; }
    size_t blank(size_t offset) const {
        while (offset < MAP && (*this)[offset] != BLANK) offset++;
        return (offset >= MAP) ? -1 : offset;
    }
    bool unique(const size_t offset) const {
        if (offset >= MAP) throw std::out_of_range{ "invalid map offset" };
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


std::minstd_rand Urng{ std::random_device{}() };

bool _solveRecursive(SudokuMap& map, const size_t off, std::string_view option = DIGIT) {
    if (off == -1) return true;
    for (char digit : option) {
        map[off] = digit;
        if (map.unique(off) && _solveRecursive(map, map.blank(off + 1), option)) return true;
    }
    map[off] = BLANK;
    return false;
}

SudokuMap solve(const SudokuMap& map) {
    std::string option{ DIGIT };
    std::shuffle(option.begin(), option.end(), Urng);

    SudokuMap solution = map;
    _solveRecursive(solution, solution.blank(0), option);
    return solution;
}


int main() {
    SudokuMap smap{ "007008000095002600000100200010000079600009000008004000000000000006400800030050020" };
    std::cout << std::string_view(smap) << '\n' << std::string_view(solve(smap)) << std::endl;
    try { std::cout << smap.unique(-1) << std::endl; }
    catch (std::exception& e) { std::cout << e.what() << std::endl; }
    return 0;
}
