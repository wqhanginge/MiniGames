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
struct Bytes :public std::array<char, Size> {
    static constexpr size_t SIZE = Size;
    constexpr operator std::string() const { return std::string(this->data(), Size); }
    constexpr operator std::string_view() const { return std::string_view(this->data(), Size); }
};

using Guess = Bytes<COUNT>;
using Secret = Bytes<COUNT>;
using Result = std::pair<size_t, size_t>;

class SecretManager {
public:
    SecretManager() :_secret(), _urng(std::random_device()()) {}

    std::string string() const { return std::string(string_view()); }
    std::string_view string_view() const { return static_cast<std::string_view>(_secret); }

    void refresh(const bool master) {   //Mastermind gives non-unique digits, while Standard gives unique digits
        if (master) { for (auto& digit : _secret) digit = SYMBOLS[_urng() % SYMBOLS.size()]; }
        else { std::sample(SYMBOLS.begin(), SYMBOLS.end(), _secret.begin(), COUNT, _urng); }
        std::shuffle(_secret.begin(), _secret.begin() + COUNT, _urng);
    }
    Result evaluate(const Guess& guess) const {
        size_t bulls = 0, cows = 0;
        std::array<bool, COUNT * 2> mask = {};  //remember matched digits for both Guess and Secret
        for (size_t shift = 0; shift < COUNT; shift++) {
            for (size_t offset = 0; offset < COUNT; offset++) {
                size_t sec_idx = (offset + shift) % COUNT;
                bool equal = (guess[offset] == _secret[sec_idx]);
                cows += (!mask[offset] && !mask[COUNT + sec_idx] && equal);
                mask[offset] |= equal;
                mask[COUNT + sec_idx] |= equal;
            }
            bulls += cows * !shift;
        }
        return Result(bulls, cows - bulls);
    }

private:
    Secret _secret;
    std::minstd_rand _urng;
};


inline bool isSpace(unsigned char c) {
    return std::isspace(c);
}

inline bool isDigit(unsigned char c) {
    return std::isdigit(c);
}

inline bool isNotSpaceAndDigit(unsigned char c) {
    return !std::isspace(c) && !std::isdigit(c);
}

template<unsigned char alpha>
inline bool equalAlpha(unsigned char c) {
    return std::isalpha(c) && std::tolower(c) == std::tolower(alpha);
}

bool readGuessFromLine(Guess& guess, std::string_view line, const bool master) {
    if (std::any_of(line.begin(), line.end(), isNotSpaceAndDigit))
        return false;
    if (std::count_if(line.begin(), line.end(), isDigit) != guess.SIZE)
        return false;
    std::copy_if(line.begin(), line.end(), guess.begin(), isDigit);
    if (!master && std::unique(guess.begin(), guess.end()) != guess.end())
        return false;
    return true;
}

bool readYnFromLine(std::string_view line) {
    auto iter = std::find_if_not(line.begin(), line.end(), isSpace);
    return iter != line.end() && equalAlpha<'y'>(*iter);
}


std::string_view mode(const bool master) {
    return (master) ? "<Mastermind>" : "<Standard>";
}

std::string title(const bool master) {
    std::string str;
    str += std::string(60, '=') + '\n';
    str += std::string(17, ' ') + "Bulls and Cows: ";
    str += mode(master);
    str += '\n' + std::string(60, '=');
    return str;
}

std::string tips(const bool valid, const size_t turn, const Guess& guess, const size_t bulls, const size_t cows) {
    std::string str;
    str += std::string(40, ' ') + std::to_string(turn) + ".\t";
    str += (valid) ? guess : std::string(guess.SIZE, ' ');
    str += std::string(4, ' ');
    str += std::to_string(bulls) + 'A' + std::to_string(cows) + 'B';
    return str;
}

std::string finish(const size_t used_chances, const SecretManager& secret_mgr) {
    std::string str;
    if (used_chances <= CHANCES) {  //succeed
        str += "Congratulations! You got the Answer: ";
        str += secret_mgr.string_view();
        str += "\nA total of " + std::to_string(used_chances) + " chances were consumed.";
    }
    else {  //failed
        str += "Sorry! You ran out of all chances.\n";
        str += "The Answer is: ";
        str += secret_mgr.string_view();
    }
    str += '\n' + std::string(60, '=');
    return str;
}


int main() {
    SecretManager secret_mgr;
    Guess guess;
    std::string line;
    bool master = false;

    while (true) {
        std::cout << mode(master) << ": Switch Mode?<y/N> ";
        if (!std::getline(std::cin, line).good()) return 0;
        master ^= readYnFromLine(line);

        std::cout << title(master) << std::endl;
        secret_mgr.refresh(master);

        size_t chance = 0;
        while (chance < CHANCES) {
            std::cout << "Make a Guess> ";
            if (!std::getline(std::cin, line).good()) return 0;

            bool valid = readGuessFromLine(guess, line, master);
            auto [bulls, cows] = (valid) ? secret_mgr.evaluate(guess) : Result(0, 0);

            std::cout << tips(valid, chance + 1, guess, bulls, cows) << std::endl;
            if (bulls == guess.SIZE) break;
            else chance++;
        }
        std::cout << '\n' << finish(chance + 1, secret_mgr) << '\n' << std::endl;
    }
    return 0;
}
