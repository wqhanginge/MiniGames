/*
 * Copyright (C) 2020, 24 Point Solver by Gee Wang
 * Licensed under the GNU GPL v3.
 * C++11 standard is required.
 */


#include <iostream>
#include <fstream>

#include <string>
#include <array>
#include <vector>

#include <future>
#include <cmath>
#include <cstring>


typedef std::string::value_type Elem;
typedef std::string Expr, Postfix, Infix;
typedef std::vector<Expr> ExprList, PostfixList, InfixList;
typedef std::pair<Infix, int> PriInfix;


constexpr double EPS = 1e-5;
constexpr int MIN_NUMBER = 0x00;
constexpr int MAX_NUMBER = 0x7F;

#pragma warning(push)
#pragma warning(disable: 4309)  //truncation of constant value (MSVC)
constexpr char F_PRUNENUM = 0x01;   //if pruning at number level
constexpr char F_PRUNEOPS = 0x02;   //if pruning at operator level
constexpr char F_RANGENUM = 0x04;   //if ranged number list
constexpr char F_SVERBOSE = 0x08;   //if display numbers without any solution

constexpr Elem OP_ADD = 0x80;
constexpr Elem OP_SUB = 0x81;
constexpr Elem OP_MUL = 0x82;
constexpr Elem OP_DIV = 0x83;
constexpr Elem OP_LBK = 0x84;
constexpr Elem OP_RBK = 0x85;
constexpr Elem OP_SIG = 0xF0;
constexpr Elem OP_NOP = 0xFF;

constexpr Elem OP_MIN = 0x80;
constexpr Elem OP_MAX = 0x83;
constexpr Elem OP_MSK = 0x80;
constexpr Elem OP_NCMBIT = 0x01;    //non-commutable bit
constexpr Elem OP_PRIBIT = 0x02;    //high priority bit
#pragma warning(pop)

constexpr auto USAGE = "\
24point [-v] [-j <n>] [-o <file>] [-p <level>] [-r <min>:<max>] <target> <num>[:...] [--op=<op>[...]]\n\n\
Positional arguments:\n\
  target                expected result value of expressions\n\
  num                   non-negative integers as input numbers\n\n\
Optional arguments:\n\
  -v, --verbose         display all results including those have no solutions\n\
  -j, --jobs <n>        specify the max available working threads\n\
  -o, --out <file>      output solutions into a file\n\
  -p, --prune <level>   set the prune level for the solving process as <std|max|off>,\n\
                        <std> prune expressions with the same operators (default),\n\
                        <max> prune expressions with the same numbers and operators,\n\
                        <off> do not prune expressions\n\
  -r, --range <min>:<max>\n\
                        enable exhaustion mode to search solutions from ranged input\n\
                        numbers, this interpret the first input number as the size of\n\
                        each number list and ignore excess input numbers\n\
  --op=<op>[...]        extra operators to be used in expressions, excess operators\n\
                        are ignored\n\
";


class ParseError : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

struct Args {
    char flags;
    int nthreads;
    int target;
    int size;
    int rmin;
    int rmax;
    Expr numbers;
    Expr operators;
    std::ofstream outfile;
} args_{};


/* init state of Postfix:          example of Header:
 * +---+---+---+---+---+---+---+   +---+---+---+---+---+
 * |NUM|NUM|NUM|...|OP |OP |...|   |SIG|NUM|NUM|...|NUM|
 * +---+---+---+---+---+---+---+   +---+---+---+---+---+
 *   1           s  s+1     2s-1     1   2          s+1
 */


inline bool isOp(const Elem e) {
    return e & OP_MSK;
}

inline bool isNonCommutative(const Elem e) {
    return isOp(e) && (e & OP_NCMBIT);
}

inline bool isExpr(const Expr& expr) {
    return !expr.empty() && (expr.front() != OP_SIG);
}

inline bool isHeader(const Expr& expr) {
    return !expr.empty() && (expr.front() == OP_SIG);
}

inline int priority(const Elem e) {
    return !isOp(e) + bool(!isOp(e) + bool(e & OP_PRIBIT));
}

inline Elem encode(const char ch) {
    switch (ch) {
    case '+': return OP_ADD;
    case '-': return OP_SUB;
    case '*': return OP_MUL;
    case '/': return OP_DIV;
    case '(': return OP_LBK;
    case ')': return OP_RBK;
    default: return OP_NOP;
    }
}

inline char decode(const Elem e) {
    switch (e) {
    case OP_ADD: return '+';
    case OP_SUB: return '-';
    case OP_MUL: return '*';
    case OP_DIV: return '/';
    case OP_LBK: return '(';
    case OP_RBK: return ')';
    default: return '\0';
    }
}

inline ExprList& operator+=(ExprList& left, const ExprList& right) {
    left.insert(left.end(), right.begin(), right.end());
    return left;
}

inline ExprList& operator+=(ExprList& left, ExprList&& right) {
    left.insert(left.end(), std::make_move_iterator(right.begin()), std::make_move_iterator(right.end()));
    return left;
}


bool isSol(const Postfix& postfix) {
    static thread_local std::vector<double> stk;
    stk.reserve(postfix.size());

    stk.clear();
    for (auto elem : postfix) {
        if (isOp(elem)) {
            double rc = stk.back(); stk.pop_back();
            double lc = stk.back(); stk.pop_back();
            switch (elem) {
            case OP_ADD: lc += rc; break;
            case OP_SUB: lc -= rc; break;
            case OP_MUL: lc *= rc; break;
            case OP_DIV: lc /= rc; break;   //generally, floating-point do not cause "divide by zero" error
            }
            stk.emplace_back(lc);
        } else {
            stk.emplace_back(elem);
        }
    }
    return !stk.empty() && (std::fabs(stk.back() - args_.target) < EPS);
}

size_t permute(PostfixList& sols, Postfix& postfix, const size_t idx, const size_t op_cnt) {
    if (idx >= postfix.size() - 1) {    //early exam
        bool issol = isSol(postfix);
        if (issol) sols.emplace_back(postfix);
        return issol;
    }

    size_t num_sols = 0;
    for (size_t i = idx; i < postfix.size(); i++) {
        bool isop = isOp(postfix[i]);
        if (2 * (op_cnt + isop) >= idx + 1) //cut invalid expr
            continue;
        if (i != idx && postfix[i] == postfix[idx]) //cut duplicate expr
            continue;
        std::swap(postfix[i], postfix[idx]);
        num_sols += permute(sols, postfix, idx + 1, op_cnt + isop);
        std::swap(postfix[i], postfix[idx]);
        if ((args_.flags & F_PRUNEOPS) && num_sols) //find one per op
            break;
    }
    return num_sols;
}

size_t traverse(PostfixList& sols, PostfixList& cands) {
    size_t num_sols = 0;
    for (auto& postfix : cands) {
        if ((args_.flags & F_PRUNENUM) && num_sols) //find one per num
            continue;
        num_sols += permute(sols, postfix, 0, 0);
    }
    return num_sols;
}

PostfixList search(const PostfixList& cands) {
    PostfixList sols, group;
    for (auto& postfix : cands) {
        if (isHeader(postfix)) {
            traverse(sols, group);
            sols.emplace_back(postfix);
            group.clear();
        } else {
            group.emplace_back(postfix);
        }
    }
    traverse(sols, group);
    return sols;
}


size_t multicombination(size_t diff, size_t size) {
    double c = 1;
    for (; size > 0; size--) c *= static_cast<double>(diff + size) / size;
    return static_cast<size_t>(std::round(c));
}

size_t chooseRecursive(ExprList& results, Expr& expr, const size_t idx, const Elem last, const Elem max) {
    if (idx >= expr.size()) {
        results.emplace_back(expr);
        return 1;
    }

    size_t cnt = 0;
    for (Elem elem = last; elem <= max; elem++) {
        expr[idx] = elem;
        cnt += chooseRecursive(results, expr, idx + 1, elem, max);
    }
    return cnt;
}

ExprList chooseWithReplace(const Expr& fixed, const size_t expr_size, const Elem min, const Elem max) {
    size_t fix_size = std::min(fixed.size(), expr_size);

    Expr expr(fixed);
    expr.resize(expr_size, OP_NOP);

    ExprList results;
    if (max >= min) {
        results.reserve(multicombination(max - min, expr_size - fix_size));
        chooseRecursive(results, expr, fix_size, min, max);
    }
    return results;
}

ExprList product(const ExprList& left, const ExprList& right, const size_t off = 0, const size_t cnt = -1) {
    size_t max_size = left.size() * right.size();
    size_t begin = std::min(max_size, off);
    size_t end = std::min(max_size, begin + std::min(max_size, cnt));

    ExprList results;
    results.reserve(end - begin);
    for (size_t i = begin; i < end; i++) {
        size_t x = i / right.size(), y = i % right.size();
        if (!y) results.emplace_back(OP_SIG + left[x]); //insert a Header
        results.emplace_back(left[x] + right[y]);
    }
    return results;
}

PostfixList solve() {
    PostfixList nums = chooseWithReplace(args_.numbers, args_.size, args_.rmin, args_.rmax);
    PostfixList ops = chooseWithReplace(args_.operators, args_.size - 1, OP_MIN, OP_MAX);

    size_t cand_size = nums.size() * ops.size();
    size_t thread_cnt = std::min<size_t>(args_.nthreads, cand_size);
    if (thread_cnt == 1) {
        return search(product(nums, ops));
    }

    size_t step = cand_size / thread_cnt;
    size_t split = cand_size % thread_cnt;

    std::vector<std::future<PostfixList>> results;
    for (size_t start = 0, i = 0; i < thread_cnt; i++) {
        size_t size = step + (i < split);
        results.emplace_back(std::async(std::launch::async, search, product(nums, ops, start, size)));
        start += size;
    }

    //combine results
    PostfixList sols;
    for (size_t i = 0; i < thread_cnt; i++) {
        PostfixList ret = results[i].get();
        if (args_.flags & F_PRUNENUM) { //retain one per num
            if (!ret.empty() && !sols.empty() && isExpr(ret.front()) && isExpr(sols.back()))
                sols.pop_back();
        }
        sols += std::move(ret);
    }
    return sols;
}


Infix convert(const Postfix& postfix) {
    static std::vector<PriInfix> stk;
    stk.reserve(postfix.size());

    stk.clear();
    for (auto elem : postfix) {
        if (isOp(elem)) {
            int pri = priority(elem);
            PriInfix rchild = std::move(stk.back()); stk.pop_back();
            PriInfix lchild = std::move(stk.back()); stk.pop_back();
            if (lchild.second < pri)    //add brackets for low priority subexpr
                lchild.first = OP_LBK + lchild.first + OP_RBK;
            if (rchild.second < pri || (rchild.second == pri && isNonCommutative(elem)))
                rchild.first = OP_LBK + rchild.first + OP_RBK;
            if (!isNonCommutative(elem) && lchild.first.compare(rchild.first) > 0)  //always small first
                lchild.swap(rchild);
            stk.emplace_back(lchild.first + elem + rchild.first, pri);
        } else {
            stk.emplace_back(Infix(1, elem), priority(elem));   //number owns the highest priority
        }
    }
    return (stk.empty()) ? Infix() : stk.back().first;
}

InfixList convertSolutions(const PostfixList& sols) {
    InfixList insols;
    insols.reserve(sols.size());
    for (auto& expr : sols) {
        if (isHeader(expr)) {
            if (!(args_.flags & F_SVERBOSE) && !insols.empty() && isHeader(insols.back()))  //filter empty
                insols.pop_back();
            insols.emplace_back(expr);
        } else {
            bool exist = false;
            Infix infix = convert(expr);
            for (auto iter = insols.rbegin(); iter != insols.rend() && isExpr(*iter); iter++) { //filter redundant
                if (infix == *iter) {
                    exist = true;
                    break;
                }
            }
            if (!exist) insols.emplace_back(std::move(infix));
        }
    }
    if (!(args_.flags & F_SVERBOSE) && !insols.empty() && isHeader(insols.back()))  //filter empty
        insols.pop_back();
    return insols;
}


std::string decode(const Expr& expr) {
    std::string str;
    for (auto elem : expr) {
        if (isOp(elem)) str += decode(elem);
        else str += std::to_string(elem);
    }
    return str;
}

std::string formatSolutions(const ExprList& sols) {
    std::string str;
    for (auto& expr : sols) {
        if (isHeader(expr)) {
            if (!str.empty()) str += '\n';
            str += "  ";
            for (auto elem : expr.substr(1))    //skip OP_SIG
                str += std::to_string(elem) + ' ';
            str += ":  ";
        } else {
            if (expr.empty()) continue;
            str += decode(expr) + "  ";
        }
    }
    return str;
}

std::string formatArgs() {
    std::string str;
    str += "  target = " + std::to_string(args_.target) + "  ";
    if (args_.flags & F_RANGENUM) { //ranged numbers
        str += "min = " + std::to_string(args_.rmin) + "  ";
        str += "max = " + std::to_string(args_.rmax) + "  ";
        str += "size = " + std::to_string(args_.size) + "  ";
    }
    if (!args_.numbers.empty()) {   //specified numbers: [NUM...]
        str += "numbers: ";
        for (auto elem : args_.numbers) {
            str += std::to_string(elem);
            str += ' ';
        }
        str += ' ';
    }
    if (!args_.operators.empty()) { //specified operators: [OP...]
        str += "operators: ";
        for (auto elem : args_.operators) {
            str += decode(elem);
            str += ' ';
        }
        str += ' ';
    }
    return str;
}


inline void assert(const bool condition, const char* message) {
    if (!condition) throw ParseError(message);
}

inline void assert(const bool condition, const std::string& message) {
    if (!condition) throw ParseError(message);
}

inline int argtoi(const std::string& str, size_t* idx = nullptr, int base = 10) {
    try { return std::stoi(str, idx, base); }
    catch (std::exception& e) { throw ParseError(e.what()); }
}

/* optional args:
 * [-v] [-j <n>] [-o <file>] [-p <level>] [-r <min>:<max>] [--op=<op>[...]]
 */
int matchOptionalArgs(int argc, char* argv[], int idx, std::array<bool, 6>& parsed) {
    if (!std::strcmp(argv[idx], "-v") || !std::strcmp(argv[idx], "--verbose")) {    //display numbers without solutions
        assert(!parsed[0], "duplicate option: " + std::string(argv[idx]));

        args_.flags |= F_SVERBOSE;

        parsed[0] = true;
        return 1;
    } else if (!std::strcmp(argv[idx], "-j") || !std::strcmp(argv[idx], "--jobs")) {    //specify max working threads
        assert(!parsed[1], "duplicate option: " + std::string(argv[idx]));
        assert(idx + 1 < argc, "unspecified value for jobs");

        size_t cnt = 0;
        args_.nthreads = argtoi(argv[idx + 1], &cnt);
        assert(cnt == std::strlen(argv[idx + 1]), "invalid value for jobs");
        assert(args_.nthreads > 0, "invalid value for jobs");

        parsed[1] = true;
        return 2;
    } else if (!std::strcmp(argv[idx], "-o") || !std::strcmp(argv[idx], "--out")) { //specify output file
        assert(!parsed[2], "duplicate option: " + std::string(argv[idx]));
        assert(idx + 1 < argc, "unspecified file name");

        args_.outfile.open(argv[idx + 1], std::ios_base::out | std::ios_base::trunc);
        assert(args_.outfile.is_open(), "unable to open file: " + std::string(argv[idx + 1]));

        parsed[2] = true;
        return 2;
    } else if (!std::strcmp(argv[idx], "-p") || !std::strcmp(argv[idx], "--prune")) {   //specify prune level
        assert(!parsed[3], "duplicate option: " + std::string(argv[idx]));
        assert(idx + 1 < argc, "unspecified prune level");

        if (!std::strcmp(argv[idx + 1], "std")) args_.flags = args_.flags & ~F_PRUNENUM | F_PRUNEOPS;
        else if (!std::strcmp(argv[idx + 1], "max")) args_.flags |= (F_PRUNENUM | F_PRUNEOPS);
        else if (!std::strcmp(argv[idx + 1], "off")) args_.flags &= ~(F_PRUNENUM | F_PRUNEOPS);
        else throw ParseError("unknow prune level");

        parsed[3] = true;
        return 2;
    } else if (!std::strcmp(argv[idx], "-r") || !std::strcmp(argv[idx], "--range")) {   //enable exhaustion mode
        assert(!parsed[4], "duplicate option: " + std::string(argv[idx]));
        assert(idx + 1 < argc, "insufficient arguments for range");

        size_t cnt, off = 0;
        args_.rmin = argtoi(argv[idx + 1] + off, &cnt);
        assert(argv[idx + 1][off + cnt] == ':', "invalid arguments for range");
        off += cnt + 1;
        args_.rmax = argtoi(argv[idx + 1] + off, &cnt);
        assert(argv[idx + 1][off + cnt] == '\0', "invalid arguments for range");
        assert(args_.rmin >= MIN_NUMBER && args_.rmax <= MAX_NUMBER, "range out of bounds");
        assert(args_.rmin <= args_.rmax, "invalid range");
        args_.flags |= F_RANGENUM;

        parsed[4] = true;
        return 2;
    } else if (!std::strncmp(argv[idx], "--op=", 5)) {  //specify operators
        assert(!parsed[5], "duplicate option: " + std::string(argv[idx]));

        for (int i = 0; 5 + i < std::strlen(argv[idx]); i++) {
            Elem e = encode(argv[idx][5 + i]);
            assert(e >= OP_MIN && e <= OP_MAX, "invalid operator");
            args_.operators += e;
        }

        parsed[5] = true;
        return 1;
    }
    return 0;
}

/* positional args:
 * <target> <num>[:...]
 */
int matchPositionalArgs(int argc, char* argv[], int idx, int& pos) {
    switch (pos) {
    case 0: {   //target: integer
        size_t cnt = 0;
        args_.target = argtoi(argv[idx], &cnt);
        assert(cnt == std::strlen(argv[idx]), "invalid target");

        pos++;
        return 1;
    }
    case 1: {   //number list: non-negative integer with limitation
        for (size_t cnt, off = 0; off < std::strlen(argv[idx]); off += cnt + 1) {
            int num = argtoi(argv[idx] + off, &cnt);
            assert(argv[idx][off + cnt] == ':' || argv[idx][off + cnt] == '\0', "invalid number");
            assert(num >= MIN_NUMBER && num <= MAX_NUMBER, "number out of range");
            args_.numbers += num;
        }
        assert(!args_.numbers.empty(), "empty input numbers");

        pos++;
        return 1;
    }
    }
    return 0;
}

bool parseHelp(int argc, char* argv[]) {
    for (int idx = 1; idx < argc; idx++)
        if (!std::strcmp(argv[idx], "-h") || !std::strcmp(argv[idx], "--help"))
            return true;
    return argc == 1;
}

bool parseArgs(int argc, char* argv[]) {
    if (parseHelp(argc, argv)) {
        std::cout << USAGE << std::endl;
        return false;
    }

    //default value of args
    unsigned hwthreads = std::thread::hardware_concurrency();
    args_.flags = F_PRUNEOPS;
    args_.nthreads = std::max<int>(hwthreads, 1);

    try {
        int curr_pos = 0;   //init state for positional args
        std::array<bool, 6> parsed_options = {};    //init state for options

        for (int ret, idx = 1; idx < argc; idx += ret) {    //parse options first
            if (ret = matchOptionalArgs(argc, argv, idx, parsed_options)) continue;
            if (ret = matchPositionalArgs(argc, argv, idx, curr_pos)) continue;
            throw ParseError("unknow argument: " + std::string(argv[idx]));
        }

        assert(curr_pos >= 2, "missing arguments"); //lack of positional args
    } catch (ParseError& pe) {
        std::cerr << pe.what() << std::endl;
        return false;
    }

    //post processing for args
    args_.size = static_cast<int>(args_.numbers.size());
    if (args_.flags & F_RANGENUM) { //exhaustion mode, at least 1 number is ranged
        args_.size = args_.numbers[0];
        args_.numbers = args_.numbers.substr(1, args_.size - 1);
    }
    args_.operators = args_.operators.substr(0, args_.size - 1);
    args_.nthreads = (hwthreads) ? std::min<int>(args_.nthreads, hwthreads) : args_.nthreads;
    return true;
}


int main(int argc, char* argv[]) {
    if (parseArgs(argc, argv)) {
        InfixList sols = convertSolutions(solve());
        std::ostream& out = (args_.outfile.is_open() ? args_.outfile : std::cout);
        out << formatArgs() << '\n';
        out << std::string(80, '-') << '\n';
        out << formatSolutions(sols) << std::endl;
    }
    return 0;
}
