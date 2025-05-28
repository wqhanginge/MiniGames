#include <iostream>
#include <fstream>

#include <array>
#include <vector>
#include <string>

#include <cstring>
#include <string_view>


constexpr size_t SEG = 3, ROW = 9, MAP = 81;
constexpr std::string_view DIGITS = "0123456789";
constexpr char BLANK = DIGITS[0];


class SudokuMap :public std::array<char, MAP> {
    using _Base = std::array<char, MAP>;
    constexpr size_t _doff(size_t offset) const { return static_cast<size_t>(_Base::at(offset) - BLANK); }

public:
    SudokuMap() { _Base::fill(BLANK); }
    SudokuMap(const std::string_view map) : SudokuMap() {
        for (size_t i = 0; i < std::min(MAP, map.size()); i++) {
            _Base::at(i) = DIGITS[std::min(static_cast<size_t>(map[i] - BLANK), DIGITS.size()) % DIGITS.size()];
        }
    }

    constexpr explicit operator std::string_view() const { return std::string_view(_Base::data(), MAP); }
    constexpr bool unique(size_t offset) const {
        const size_t x = offset / ROW, y = offset % ROW, bx = x / SEG * SEG, by = y / SEG * SEG;
        const size_t stride = DIGITS.size();
        std::array<bool, stride * 3> flags = {};
        for (size_t i = 0; i < ROW; i++) {
            size_t irow = 0*stride+_doff(x * ROW + i), icol = _doff(i * ROW + y), iblk = _doff((bx + i / SEG) * ROW + (by + i % SEG));
            if (flags[irow] || flags[icol] || flags[iblk]) return false;
            flags[irow] |= static_cast<bool>(irow);
            flags[icol] |= static_cast<bool>(icol);
            flags[iblk] |= static_cast<bool>(iblk);
        }
        return true;
    }
};


int main() {
    SudokuMap smap("007008000095002600000100250010000079600009000008004000000000000006400800030050020");
    std::vector<SudokuMap> sols;
    return 0;
}
