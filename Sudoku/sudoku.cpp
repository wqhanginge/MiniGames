#include <iostream>
#include <fstream>

#include <array>
#include <vector>
#include <string>

#include <cstring>
#include <string_view>


class SudokuMap {
public:
    static constexpr size_t SEG = 3, ROW = 9, SEC = 27, MAP = 81;
    static constexpr std::string_view DIGITS = "0123456789";

    SudokuMap();
    explicit SudokuMap(const std::string_view map);

    std::string to_string() const;
    size_t solve(std::vector<SudokuMap>& sols);

private:
    std::array<char, MAP> _map;

    bool _unique(size_t idx);
    size_t _solve_recursive(std::vector<SudokuMap>& sols, const size_t lidx);
};


SudokuMap::SudokuMap() {
    memset(_map.data(), DIGITS[0], _map.size());
}

SudokuMap::SudokuMap(const std::string_view map) {
    memset(_map.data(), DIGITS[0], _map.size());
    memcpy(_map.data(), map.data(), std::min(_map.size(), map.size()));
    for (size_t i = 0; i < std::min(_map.size(), map.size()); i++) {
        _map[i] = (DIGITS.find(_map[i]) == DIGITS.npos) ? DIGITS[0] : _map[i];
    }
}

std::string SudokuMap::to_string() const {
    std::string str;
    for (size_t i = 0; i < _map.size(); i++) {
        str += _map[i];
        if (!((i + 1) % MAP)) continue;
        if (!((i + 1) % SEC)) str += '\n';
        if (!((i + 1) % ROW)) { str += '\n'; continue; }
        if (!((i + 1) % SEG)) str += ' ';
        str += ' ';
    }
    return str;
}

size_t SudokuMap::solve(std::vector<SudokuMap>& sols) {
    size_t idx = 0;
    while (idx < _map.size() && _map[idx] != DIGITS[0]) idx++;
    return (idx < _map.size()) ? _solve_recursive(sols, idx) : 0;
}

bool SudokuMap::_unique(size_t idx) {
    size_t x = idx / ROW, y = idx % ROW;
    size_t frow = 0, fcol = 0, fblk = 0, flag;
    for (size_t i = 0; i < ROW; i++) {
        flag = static_cast<size_t>(1) << (_map[x * ROW + i] - DIGITS[0]);
        if ((frow & flag) > 1) return false;
        frow |= flag;

        flag = static_cast<size_t>(1) << (_map[i * ROW + y] - DIGITS[0]);
        if ((fcol & flag) > 1) return false;
        fcol |= flag;

        flag = static_cast<size_t>(1) << (_map[(x / SEG * SEG + i / SEG) * ROW + (y / SEG * SEG + i % SEG)] - DIGITS[0]);
        if ((fblk & flag) > 1) return false;
        fblk |= flag;
    }
    return true;
}

size_t SudokuMap::_solve_recursive(std::vector<SudokuMap>& sols, size_t idx) {
    if (idx >= _map.size()) {
        sols.push_back(*this);
        return 1;
    }

    size_t cnt = 0, next = idx + 1;
    while (next < _map.size() && _map[next] != DIGITS[0]) next++;
    for (auto digit : DIGITS.substr(1)) {
        _map[idx] = digit;
        if (_unique(idx)) cnt += _solve_recursive(sols, next);
    }
    _map[idx] = DIGITS[0];
    return cnt;
}


int main() {
    SudokuMap smap("007008000095002600000100250010000079600009000008004000000000000006400800030050020");
    std::vector<SudokuMap> sols;
    std::cout << smap.to_string() << '\n' << smap.solve(sols) << std::endl;
    for (auto& sol : sols) std::cout << sol.to_string() << std::endl;
    return 0;
}
