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
    static constexpr std::string_view DIGITS = "123456789";
    static constexpr char BLANK = '0';

    SudokuMap();
    explicit SudokuMap(const std::string_view map);

    std::string string() const;
    size_t solve(std::vector<SudokuMap>& results);

private:
    std::array<char, MAP> _map;

    bool _unique(size_t idx);
    size_t _next_blank(size_t idx);
};


SudokuMap::SudokuMap() {
    memset(_map.data(), BLANK, _map.size());
}

SudokuMap::SudokuMap(const std::string_view map) :SudokuMap() {
    size_t cnt = map.copy(_map.data(), _map.size());
    for (size_t i = 0; i < cnt; i++) {
        _map[i] = std::min(std::max(_map[i] - BLANK, 0), 10) % 10 + BLANK;
    }
}

std::string SudokuMap::string() const {
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

size_t SudokuMap::solve(std::vector<SudokuMap>& results) {
    size_t cnt = 0;
    std::vector<std::pair<size_t, size_t>> stack = { { 0, 0 } };
    while (!stack.empty()) {
        auto [imap, idigit] = stack.back();
        stack.pop_back();

        size_t iblank = _next_blank(imap);
        if (iblank >= _map.size()) {
            results.push_back(*this);
            cnt++;
            continue;
        }

        if (idigit < DIGITS.size()) {
            _map[iblank] = DIGITS[idigit++];
            if (_unique(iblank)) stack.emplace_back(std::pair{ iblank + 1, 0 });
            continue;
        }

        _map[iblank] = BLANK;
    }
    return cnt;
}

bool SudokuMap::_unique(size_t idx) {
    size_t x = idx / ROW, y = idx % ROW, bx = x / SEG * SEG, by = y / SEG * SEG;
    size_t frow = 0, fcol = 0, fblk = 0, flag;
    for (size_t i = 0; i < ROW; i++) {  //size_t is at least 16 bits
        flag = size_t(2) << (sizeof(size_t) * 8 - 1 - size_t(_map[x * ROW + i] - BLANK));
        if (frow & flag) return false;
        frow |= flag;

        flag = size_t(2) << (sizeof(size_t) * 8 - 1 - size_t(_map[i * ROW + y] - BLANK));
        if (fcol & flag) return false;
        fcol |= flag;

        flag = size_t(2) << (sizeof(size_t) * 8 - 1 - size_t(_map[(bx + i / SEG) * ROW + (by + i % SEG)] - BLANK));
        if (fblk & flag) return false;
        fblk |= flag;
    }
    return true;
}

size_t SudokuMap::_next_blank(size_t idx) {
    while (idx < _map.size() && _map[idx] != BLANK) idx++;
    return idx;
}


int main() {
    SudokuMap smap("007008000095002600000100250010000079600009000008004000000000000006400800030050020");
    std::vector<SudokuMap> sols;
    std::cout << smap.string() << '\n' << smap.solve(sols) << std::endl;
    for (auto& sol : sols) std::cout << sol.string() << std::endl;
    return 0;
}
