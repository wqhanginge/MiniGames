#include <iostream>
#include <fstream>

#include <thread>
#include <cmath>
#include <cstring>

#include <string>
#include <stack>
#include <vector>


typedef std::string::value_type Atom;
typedef std::string Expr, Postfix, Infix;
typedef std::vector<Expr> ExprList, PostfixList, InfixList;
typedef std::pair<Infix, int> PriInfix;
typedef std::vector<PostfixList> PGroup;
typedef std::vector<std::thread> TGroup;


constexpr double EPS = 1e-5;
constexpr int MIN_NUMBER = 0x00;
constexpr int MAX_NUMBER = 0x7F;

#pragma warning(push)
#pragma warning(disable: 4309)  //truncation of constant value
constexpr char F_PRUNENUM = 0x01;   //if pruning at number level
constexpr char F_PRUNEOPS = 0x02;   //if pruning at operator level
constexpr char F_RANGENUM = 0x04;   //if ranged number list
constexpr char F_DISPFULL = 0x08;   //if display numbers without any solution

constexpr Atom OP_ADD = 0x80;
constexpr Atom OP_SUB = 0x81;
constexpr Atom OP_MUL = 0x82;
constexpr Atom OP_DIV = 0x83;
constexpr Atom OP_LBK = 0x84;
constexpr Atom OP_RBK = 0x85;
constexpr Atom OP_SIG = 0xF0;
constexpr Atom OP_SEP = 0xF1;
constexpr Atom OP_NOP = 0xFF;

constexpr Atom OP_MIN = 0x80;
constexpr Atom OP_MAX = 0x83;
constexpr Atom OP_MSK = 0x80;
constexpr Atom OP_NCO_MSK = 0x01;   //non-commutable bit
constexpr Atom OP_PRI_MSK = 0x02;   //high priority bit
#pragma warning(pop)

constexpr auto USAGE = "\
24point [-d] [-j <n>] [-o <file>] [-p | -P] [-r <min> <max>] <target> <num>[:...] [--op=<op>[...]]\n\n\
Positional arguments:\n\
  target                expected result value of expressions\n\
  num                   non-negative integers as input numbers\n\n\
Optional arguments:\n\
  -d, --display-blanks  display number lists that have no solutions\n\
  -j, --jobs <n>        number of working threads, default to max available threads\n\
  -o, --out <file>      output solutions into a file\n\
  -p, --prune           search only one solution for each different list of numbers\n\
  -P, --no-prune        search all possible solutions\n\
  -r, --range <min> <max>\n\
                        enable exhaustion mode to search solutions from ranged input\n\
                        numbers, this interpret the first input number as the size of\n\
                        each number list and ignore excess input numbers\n\
  --op=<op>[...]        extra operators to be used in expressions, excess operators\n\
                        are ignored\n\
";


class ParseError : public std::logic_error {
public:
    explicit ParseError(const std::string& msg) :std::logic_error(msg.c_str()) {}
    explicit ParseError(const char* msg) :std::logic_error(msg) {}
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
 * |NUM|NUM|NUM|...|OP |OP |...|   |NUM|NUM|...|NUM|SIG|
 * +---+---+---+---+---+---+---+   +---+---+---+---+---+
 *   1           s          2s-1     1           s  s+1
 */


inline bool isOp(const Atom a) {
    return a & OP_MSK;
}

inline bool isNonCommutative(const Atom a) {
    return a & OP_NCO_MSK;
}

inline bool isExpr(const Expr& item) {
    return item.find(OP_SIG) == Expr::npos;
}

inline bool isHeader(const Expr& item) {
    return item.find(OP_SIG) != Expr::npos;
}

inline int priority(const Atom a) {
    return !isOp(a) + bool(!isOp(a) + bool(a & OP_PRI_MSK));
}

inline Atom encode(const char ch) {
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

inline char decode(const Atom a) {
    switch (a) {
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
    left.insert(left.cend(), right.cbegin(), right.cend());
    return left;
}


bool isSol(const Postfix& postfix) {
    std::stack<double> stk;
    double lc, rc;
    for (auto atom : postfix) {
        if (isOp(atom)) {
            rc = stk.top();
            stk.pop();
            lc = stk.top();
            stk.pop();
            switch (atom) {
            case OP_ADD: lc += rc; break;
            case OP_SUB: lc -= rc; break;
            case OP_MUL: lc *= rc; break;
            case OP_DIV: lc /= rc; break;   //generally, floating-point do not cause "divide by zero" error
            }
            stk.emplace(lc);
        }
        else {
            stk.emplace(atom);
        }
    }
    return std::fabs(stk.top() - args_.target) < EPS;
}

size_t permutePostfix(PostfixList& sols, Postfix& postfix, const size_t idx, const size_t op_cnt) {
    if (idx == postfix.size() - 1) {
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
        num_sols += permutePostfix(sols, postfix, idx + 1, op_cnt + isop);
        std::swap(postfix[i], postfix[idx]);
        if ((args_.flags & F_PRUNEOPS) && num_sols) //find one per op
            break;
    }
    return num_sols;
}

void traversePostfix(PostfixList& sols, const PostfixList& reqs) {
    size_t nsols = 0;
    for (auto& item : reqs) {
        if (isHeader(item)) {
            sols.emplace_back(item);
            nsols = 0;
            continue;
        }
        if ((args_.flags & F_PRUNENUM) && nsols)    //find one per num
            continue;
        Postfix postfix(item);
        nsols += permutePostfix(sols, postfix, 0, 0);
    }
}

void solveMultiThread(PostfixList& sols, const PostfixList& reqs) {
    if (args_.nthreads == 1 || reqs.size() <= 2) {
        traversePostfix(sols, reqs);
        return;
    }

    size_t thread_cnt = std::min<size_t>(args_.nthreads, reqs.size());
    size_t step = reqs.size() / thread_cnt;
    size_t split = reqs.size() % thread_cnt;

    PGroup solutions(thread_cnt);
    TGroup threads(thread_cnt);
    size_t start = 0;
    for (size_t i = 0; i < thread_cnt; i++) {
        size_t stop = start + step + (i < split);
        threads[i] = std::thread(traversePostfix, std::ref(solutions[i]), PostfixList(reqs.cbegin() + start, reqs.cbegin() + stop));
        start = stop;
    }

    //combine results
    for (size_t i = 0; i < thread_cnt; i++) {
        threads[i].join();
        if (args_.flags & F_PRUNENUM) { //retain one per num
            if (!solutions[i].empty() && !sols.empty()
                && isExpr(solutions[i].front()) && isExpr(sols.back()))
                sols.pop_back();
        }
        sols += solutions[i];
    }
}


size_t exhaustCombination(PostfixList& results, Postfix& postfix, const size_t idx, const Atom last, const Atom max) {
    if (idx == postfix.size()) {
        results.emplace_back(postfix);
        return 1;
    }
    size_t cnt = 0;
    auto& item = postfix[idx];
    for (item = last; item <= max; item++)
        cnt += exhaustCombination(results, postfix, idx + 1, item, max);
    return cnt;
}

size_t exhaustCCount(const int min, const int max, const size_t size) {
    if (min > max) return 0;
    double s = 1, d = max - min, n = 0;
    while (n < size) s *= ++d / ++n;
    return (size_t)s;
}

void productPostfix(PostfixList& results, const PostfixList& postfixA, const PostfixList& postfixB) {
    results.reserve(results.size() + postfixA.size() + postfixA.size() * postfixB.size());
    for (auto& pfa : postfixA) {
        results.emplace_back(pfa + OP_SIG); //insert a Header
        for (auto& pfb : postfixB) {
            results.emplace_back(pfa + pfb);
        }
    }
}

void filterDuplicateSolutions(PostfixList& filtered, const PostfixList& origin) {    
    for (auto& postfix : origin) {
        bool exist = false;
        for (auto iter = filtered.crbegin(); iter < filtered.crend() && isExpr(*iter); iter++) {
            if (postfix == *iter) {
                exist = true;
                break;
            }
        }
        if (!exist) filtered.emplace_back(postfix);
    }
}

void filterEmptySolutions(PostfixList& filtered, const PostfixList& origin) {
    for (auto& postfix : origin) {
        if (isHeader(postfix) && filtered.size() && isHeader(filtered.back()))
            filtered.pop_back();
        filtered.emplace_back(postfix);
    }
    while (filtered.size() && isHeader(filtered.back()))
        filtered.pop_back();
}

PostfixList solve() {
    PostfixList num_list;
    Postfix num(args_.numbers);
    num.resize(args_.size, 0);
    num_list.reserve(exhaustCCount(args_.rmin, args_.rmax, args_.size - args_.numbers.size()));
    exhaustCombination(num_list, num, args_.numbers.size(), args_.rmin, args_.rmax);

    PostfixList op_list;
    Postfix op(args_.operators);
    op.resize(args_.size - 1, OP_NOP);
    op_list.reserve(exhaustCCount(OP_MIN, OP_MAX, args_.size - 1 - args_.operators.size()));
    exhaustCombination(op_list, op, args_.operators.size(), OP_MIN, OP_MAX);

    PostfixList sol_list, expr_list;
    productPostfix(expr_list, num_list, op_list);
    solveMultiThread(sol_list, expr_list);

    PostfixList sols, sol_set;
    filterDuplicateSolutions(sol_set, sol_list);
    if (args_.flags & F_DISPFULL) { //retain all outputs
        sols += sol_set;
    }
    else {  //remove numbers which have no solutions
        filterEmptySolutions(sols, sol_set);
    }
    return sols;
}


Infix convertPostfixInfix(const Postfix& postfix) {
    std::stack<PriInfix> stk;
    PriInfix lchild, rchild;
    for (auto atom : postfix) {
        if (isOp(atom)) {
            int pri = priority(atom);
            rchild = std::move(stk.top());
            stk.pop();
            lchild = std::move(stk.top());
            stk.pop();
            if (lchild.second < pri)    //add brackets for low priority subexpr
                lchild.first = OP_LBK + lchild.first + OP_RBK;
            if (rchild.second < pri || (rchild.second == pri && isNonCommutative(atom)))
                rchild.first = OP_LBK + rchild.first + OP_RBK;
            if (!isNonCommutative(atom) && lchild.first.compare(rchild.first) > 0)  //always small first
                lchild.swap(rchild);
            stk.emplace(lchild.first + atom + rchild.first, pri);
        }
        else {
            stk.emplace(Infix(1, atom), priority(atom));    //number owns the highest priority
        }
    }
    return stk.top().first;
}

std::string decodeExpr(const Expr& expr) {
    std::string str;
    for (auto atom : expr) {
        if (isOp(atom)) str += decode(atom);
        else str += std::to_string(atom);
    }
    return str;
}

std::string decodeSolutions(const PostfixList& sols) {
    std::string str;
    InfixList history;
    for (auto& pexpr : sols) {
        if (isHeader(pexpr)) {
            history.clear();
            if (!str.empty()) str += '\n';
            str += "  ";
            for (auto atom : pexpr) //skip OP_SIG
                str += (isOp(atom)) ? "" : std::to_string(atom) + ' ';
            str += ":  ";
        }
        else {
            Infix iexpr = convertPostfixInfix(pexpr);
            for (auto iter = history.crbegin(); iter < history.crend(); iter++) //filter redundant solutions at infix format
                if (*iter == iexpr) iexpr.clear();
            if (iexpr.empty()) continue;
            history.emplace_back(iexpr);
            str += decodeExpr(iexpr);
            str += "  ";
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
        for (auto atom : args_.numbers) {
            str += std::to_string(atom);
            str += ' ';
        }
        str += ' ';
    }
    if (!args_.operators.empty()) { //specified operators: [OP...]
        str += "operators: ";
        for (auto atom : args_.operators) {
            str += decode(atom);
            str += ' ';
        }
        str += ' ';
    }
    return str;
}


inline void condithrow(const bool condition, const char* message) {
    if (condition) throw ParseError(message);
}

inline void condithrow(const bool condition, const std::string& message) {
    if (condition) throw ParseError(message);
}

inline int nextValidArg(int argc, char* argv[], int idx = 0, int skip = 0) {
    idx += skip;
    do { ++idx; } while (idx < argc && !argv[idx]);
    return (idx < argc && argv[idx]) ? idx : -argc;
}

int parseNumber(const char* arg, size_t start = 0, size_t stop = -1) {
    int x = 0, symbol = 0;
    stop = std::min(stop, std::strlen(arg));
    for (size_t i = start; i < stop; i++) {
        if (i == start && (arg[i] == '+' || arg[i] == '-')) {
            symbol = ',' - arg[i];
            continue;
        }
        condithrow(arg[i] < '0' || arg[i] > '9', "invalid number");
        x = x * 10 + arg[i] - '0';
    }
    condithrow(start + bool(symbol) >= stop, "invalid number");
    return x * (!symbol + symbol);
}

/* optional args:
 * [-d] [-j <n>] [-o <file>] [-p | -P] [-r <min> <max>] [--op=<op>[...]]
 */
int matchOptionalArgs(int argc, char* argv[], int idx, std::vector<bool>& parsed) {
    if (!std::strcmp(argv[idx], "-d") || !std::strcmp(argv[idx], "--display-blanks")) { //display numbers without solutions
        condithrow(parsed.at(0), "duplicate option: " + std::string(argv[idx]));

        args_.flags |= F_DISPFULL;

        parsed.at(0) = true;
        return 1;
    }
    else if (!std::strcmp(argv[idx], "-j") || !std::strcmp(argv[idx], "--jobs")) {  //specify working threads
        condithrow(parsed.at(1), "duplicate option: " + std::string(argv[idx]));

        int ijob = nextValidArg(argc, argv, idx);
        condithrow(ijob < 0, "unspecified value for jobs");
        args_.nthreads = parseNumber(argv[ijob]);
        condithrow(args_.nthreads <= 0, "invalid value for jobs");

        parsed.at(1) = true;
        return 2;
    }
    else if (!std::strcmp(argv[idx], "-o") || !std::strcmp(argv[idx], "--out")) {   //specify output file
        condithrow(parsed.at(2), "duplicate option: " + std::string(argv[idx]));

        int ifile = nextValidArg(argc, argv, idx);
        condithrow(ifile < 0, "unspecified file name");
        args_.outfile.open(argv[ifile], std::ios_base::out | std::ios_base::trunc);
        condithrow(!args_.outfile.is_open(), "unable to open file: " + std::string(argv[ifile]));

        parsed.at(2) = true;
        return 2;
    }
    else if (!std::strcmp(argv[idx], "-p") || !std::strcmp(argv[idx], "--prune")) { //prune at number level
        condithrow(parsed.at(3), "duplicate option: " + std::string(argv[idx]));
        condithrow(parsed.at(4), "conflicting option: " + std::string(argv[idx]));

        args_.flags |= (F_PRUNENUM | F_PRUNEOPS);

        parsed.at(3) = true;
        return 1;
    }
    else if (!std::strcmp(argv[idx], "-P") || !std::strcmp(argv[idx], "--no-prune")) {  //suppress any pruning
        condithrow(parsed.at(4), "duplicate option: " + std::string(argv[idx]));
        condithrow(parsed.at(3), "conflicting option: " + std::string(argv[idx]));

        args_.flags &= ~(F_PRUNENUM | F_PRUNEOPS);

        parsed.at(4) = true;
        return 1;
    }
    else if (!std::strcmp(argv[idx], "-r") || !std::strcmp(argv[idx], "--range")) { //enable exhaustion mode
        condithrow(parsed.at(5), "duplicate option: " + std::string(argv[idx]));

        args_.flags |= F_RANGENUM;
        int imin = nextValidArg(argc, argv, idx);
        int imax = nextValidArg(argc, argv, idx, 1);
        condithrow(imin < 0 || imax < 0, "insufficient arguments for range");
        args_.rmin = parseNumber(argv[imin]);
        args_.rmax = parseNumber(argv[imax]);
        condithrow(args_.rmin < MIN_NUMBER || args_.rmax > MAX_NUMBER, "number too large or too small");
        condithrow(args_.rmin > args_.rmax, "invalid range");

        parsed.at(5) = true;
        return 3;
    }
    else if (!std::strncmp(argv[idx], "--op=", 5)) {    //specify operators
        condithrow(parsed.at(6), "duplicate option: " + std::string(argv[idx]));

        for (int i = 0; 5 + i < std::strlen(argv[idx]); i++) {
            Atom a = encode(argv[idx][5 + i]);
            condithrow(a < OP_MIN || a> OP_MAX, "invalid operator");
            args_.operators += a;
        }

        parsed.at(6) = true;
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
        args_.target = parseNumber(argv[idx]);

        pos++;
        return 1;
    }
    case 1: {   //number list: non-negative integer with limitation
        for (int i = 0, j = 0; i <= std::strlen(argv[idx]); i = ++j) {
            while (argv[idx][j] != '\0' && argv[idx][j] != ':') j++;
            int num = parseNumber(argv[idx], i, j);
            condithrow(num < MIN_NUMBER || num > MAX_NUMBER, "number too large or too small");
            args_.numbers += num;
        }
        condithrow(args_.numbers.empty(), "empty input numbers");

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
    int max_threads = std::thread::hardware_concurrency();
    args_.flags = F_PRUNEOPS;
    args_.nthreads = max_threads;

    try {
        int curr_pos = 0;   //init state for positionsl args
        std::vector<bool> parsed_options(7, false); //init state for options

        int ret, idx = nextValidArg(argc, argv, 0);
        for (; idx >= 0; idx = nextValidArg(argc, argv, idx, ret - 1)) {    //parse options first
            if (ret = matchOptionalArgs(argc, argv, idx, parsed_options)) continue;
            if (ret = matchPositionalArgs(argc, argv, idx, curr_pos)) continue;
            throw ParseError("unknow argument: " + std::string(argv[idx]));
        }

        condithrow(curr_pos < 2, "missing arguments");  //lack of positional args
    }
    catch (ParseError& pe) {
        std::cerr << pe.what() << std::endl;
        return false;
    }

    //post processing for args
    args_.size = (int)args_.numbers.size();
    if (args_.flags & F_RANGENUM) { //exhaustion mode, at least 1 number is ranged
        args_.size = args_.numbers[0];
        args_.numbers = args_.numbers.substr(1, args_.size - 1);
    }
    args_.operators = args_.operators.substr(0, args_.size - 1);
    if (args_.nthreads > max_threads * 2) {
        std::cerr << "warning: too large value for jobs: " << args_.nthreads;
        std::cerr << ", is limited to: " << max_threads * 2 << std::endl;
    }
    args_.nthreads = std::min<int>(args_.nthreads, max_threads * 2);
    return true;
}


int main(int argc, char* argv[]) {
    if (parseArgs(argc, argv)) {
        PostfixList sols = solve();
        std::ostream& out = (args_.outfile.is_open() ? args_.outfile : std::cout);
        out << formatArgs() << '\n';
        out << std::string(80, '-') << '\n';
        out << decodeSolutions(sols) << std::endl;
    }
    return 0;
}
