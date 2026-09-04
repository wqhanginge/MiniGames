/*
 * Copyright (C) 2024, Bulls and Cows by Gee Wang
 * Licensed under the GPLv3.
 * C++17 standard is required.
 */


#include <iostream>

#include <array>
#include <string>
#include <string_view>

#include <algorithm>
#include <random>
#include <cstring>


constexpr std::string_view SYMBOLS = "0123456789";
constexpr size_t COUNT = 4;
constexpr size_t CHANCES = 10;

static_assert(COUNT <= SYMBOLS.size(), "insufficient types of symbols");


template<size_t Size>
class Bytes : public std::array<char, Size> {
public:
    constexpr operator std::string_view() const { return std::string_view{ this->data(), Size }; }
};

using Guess = Bytes<COUNT>;
using Secret = Bytes<COUNT>;
using Result = std::pair<size_t, size_t>;

class SecretManager {
public:
    SecretManager() : _secret(), _urng(std::random_device{}()) {}

    void refresh(const bool master) {   //Mastermind gives non-unique digits, while Standard gives unique digits
        if (master) {
            for (auto& digit : _secret) digit = SYMBOLS[_urng() % SYMBOLS.size()];
        } else {
            std::sample(SYMBOLS.begin(), SYMBOLS.end(), _secret.begin(), COUNT, _urng);
        }
        std::shuffle(_secret.begin(), _secret.end(), _urng);
    }

    std::string_view secret() const { return static_cast<std::string_view>(_secret); }
    Result evaluate(const Guess& guess) const {
        size_t bulls = 0, cows = 0;
        bool mask[2][COUNT] = {};   //remember matched digits for both Guess and Secret
        for (size_t shift = 0; shift < COUNT; shift++) {
            for (size_t offset = 0; offset < COUNT; offset++) {
                size_t sec_idx = (offset + shift) % COUNT;
                bool equal = (guess[offset] == _secret[sec_idx]);
                cows += (!mask[0][offset] && !mask[1][sec_idx] && equal);
                mask[0][offset] |= equal;
                mask[1][sec_idx] |= equal;
            }
            bulls += cows * !shift;
        }
        return Result{ bulls, cows - bulls };
    }

private:
    Secret _secret;
    std::minstd_rand _urng;
};


bool isDigit(unsigned char c) {
    return std::isdigit(c);
}

bool isNotSpace(unsigned char c) {
    return !std::isspace(c);
}

bool isNotDigit(unsigned char c) {
    return !std::isdigit(c);
}

bool equalAlpha(unsigned char c, unsigned char alpha) {
    return std::isalpha(c) && std::isalpha(alpha) && std::tolower(c) == std::tolower(alpha);
}

bool readGuessFromLine(Guess& guess, std::string_view line, const bool master) {
    std::string buff;
    std::copy_if(line.begin(), line.end(), std::back_inserter(buff), isNotSpace);
    std::sort(buff.begin(), buff.end());

    if (buff.size() != guess.size())
        return false;
    if (std::any_of(buff.begin(), buff.end(), isNotDigit))
        return false;
    if (!master && std::adjacent_find(buff.begin(), buff.end()) != buff.end())
        return false;

    std::copy_if(line.begin(), line.end(), guess.begin(), isDigit);
    return true;
}

bool testAlphaFromLine(std::string_view line, const char alpha) {
    auto iter = std::find_if(line.begin(), line.end(), isNotSpace);
    size_t size = std::count_if(iter, line.end(), isNotSpace);
    return size == 1 && equalAlpha(*iter, alpha);
}


std::string_view mode(const bool master) {
    return (master) ? "Mastermind" : "Standard";
}

std::string help() {
    std::string str;
    str += std::string(60, '=') + '\n';
    str += std::string(23, ' ') + "Bulls and Cows" + '\n';
    str += std::string(11, ' ') + "s: start    q: quit    t: toggle mode\n";
    str += std::string(60, '=');
    return str;
}

std::string title(const bool master) {
    std::string str;
    str += std::string(60, '=') + '\n';
    str += std::string(18 - master, ' ') + "Bulls and Cows: ";
    str += mode(master);
    str += '\n' + std::string(60, '=');
    return str;
}

std::string tips(const size_t turn, std::string_view guess, const size_t bulls, const size_t cows) {
    std::string str;
    str += std::string(40, ' ') + std::to_string(turn) + ".\t";
    str += guess;
    str += std::string(4, ' ');
    str += std::to_string(bulls) + 'A' + std::to_string(cows) + 'B';
    return str;
}

std::string finish(const size_t tries, std::string_view secret) {
    std::string str;
    if (tries <= CHANCES) { //win
        str += "Congratulations! You got the Answer: ";
        str += secret;
        str += "\nA total of " + std::to_string(tries) + " chances were consumed.";
    } else {    //loss
        str += "Sorry! You ran out of all chances.\n";
        str += "The Answer is: ";
        str += secret;
    }
    str += '\n' + std::string(60, '=');
    return str;
}


int main() {
    SecretManager secret_mgr;
    Guess guess;
    std::string line;
    bool master = false;

    std::cout << help() << std::endl;
    while (true) {
        std::cout << mode(master) << "> ";
        if (!std::getline(std::cin, line).good()) return 0;

        if (testAlphaFromLine(line, 'q')) return 0;
        master ^= testAlphaFromLine(line, 't');

        if (!testAlphaFromLine(line, 's')) continue;
        std::cout << title(master) << std::endl;
        secret_mgr.refresh(master);

        size_t chance;
        for (chance = 0; chance < CHANCES; chance++) {
            std::cout << "Guess> ";
            if (!std::getline(std::cin, line).good()) return 0;

            if (!readGuessFromLine(guess, line, master)) guess.fill(' ');
            auto [bulls, cows] = secret_mgr.evaluate(guess);

            std::cout << tips(chance + 1, guess, bulls, cows) << std::endl;
            if (bulls == guess.size()) break;
        }
        std::cout << '\n' << finish(chance + 1, secret_mgr.secret()) << '\n' << std::endl;
    }
    return 0;
}
