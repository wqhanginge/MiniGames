#include <iostream>
#include <fstream>
#include <thread>

#include <string>
#include <stack>
#include <vector>

constexpr double EPS = 1e-5;

/* solve mode
 *  7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+
 * |     |S| F | T |
 * +-+-+-+-+-+-+-+-+
 * T:  task bits
 * F:  search mode bits
 * S:  show bit
 */

constexpr char SM_T_MNMO = 0x00;
constexpr char SM_T_MNSO = 0x01;
constexpr char SM_T_SNMO = 0x02;
constexpr char SM_T_SNSO = 0x03;
constexpr char SM_TSO_MSK = 0x01;
constexpr char SM_TSN_MSK = 0x02;
constexpr char SM_F_MULTI = 0x00;
constexpr char SM_F_SEXPR = 0x04;
constexpr char SM_F_SNUMB = 0x0C;
constexpr char SM_FSEX_MSK = 0x04;
constexpr char SM_FSNU_MSK = 0x08;
constexpr char SM_S_HIDE = 0x00;
constexpr char SM_S_SHOW = 0x10;
constexpr char SM_SSH_MSK = 0x10;

constexpr char OP_ADD = 0x80;
constexpr char OP_SUB = 0x81;
constexpr char OP_MUL = 0x82;
constexpr char OP_DIV = 0x83;
constexpr char OP_LBK = 0x84;
constexpr char OP_RBK = 0x85;
constexpr char OP_NOP = 0x87;
constexpr char OP_MIN = 0x80;
constexpr char OP_MAX = 0x83;
constexpr char OP_MSK = 0x80;
constexpr char OP_NEX_MSK = 0x01;
constexpr char OP_PRI_MSK = 0x02;

constexpr auto USAGE = "\
24point {--aim AIM} {Task Arg [Arg [...]]} [--job N] [--out FILENAME] [--sop | --snum | --sall] [--show]\n\n\
  --aim AIM           specify the aim number, required\n\
  Task                specify one of the following tasks, required\n\
    --exhaust MIN MAX DIGIT\n\
                      find all possible solution\n\
    --search MIN MAX OP [OP2 [...]]\n\
                      search suitable numbers\n\
    --solve NUM1 NUM2 [NUM3 [...]]\n\
                      solve specified numbers\n\
    --test NUM1 NUM2 [NUM3 [...]] OP [OP2 [...]]\n\
                      test specified combination\n\
  --help              show this message\n\
  --job N             threads to run, default to available CPU cores\n\
  --out FILENAME      output result into file\n\
  --sop               find one per operator, default option\n\
  --snum              find one per number\n\
  --sall              find all possible result\n\
  --show              show numbers which have no solutions\
";

/* init state of Postfix:
 * +---+---+---+---+---+---+---+
 * |NUM|NUM|NUM|...|OP |OP |...|
 * +---+---+---+---+---+---+---+
 *   1           d          2d-1
 */

typedef std::string Postfix, Infix;
typedef std::vector<Postfix> Solution;
typedef std::pair<int, Infix> PriInfix;
typedef std::vector<Solution> SList;
typedef std::vector<std::thread> TList;

struct Args {
	char smode;
	int aim;
	int min;
	int max;
	int digit;
	int cthread;
	std::ofstream outfile;
	std::vector<int> nums;
	std::vector<char> ops;
} args_{};


inline bool isOp(const char x) {
	return x & OP_MSK;
}

inline bool isExpr(const std::string& item) {
	return item.size() > args_.digit;
}

inline bool isNumList(const std::string& item) {
	return item.size() == args_.digit;
}

void concat(Solution& dst, Solution& src) {
	dst.insert(dst.cend(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
}


Infix convertPostfixInfix(const Postfix& postfix) {
	std::stack<PriInfix> expr;
	PriInfix lchild, rchild;
	for (auto item : postfix) {
		if (isOp(item)) {
			int pri = !(!(item & OP_PRI_MSK));
			rchild = expr.top();
			expr.pop();
			lchild = expr.top();
			expr.pop();
			if (lchild.first < pri)
				lchild.second = OP_LBK + lchild.second + OP_RBK;
			if (rchild.first < pri || (rchild.first == pri && (item & OP_NEX_MSK)))
				rchild.second = OP_LBK + rchild.second + OP_RBK;
			expr.emplace(pri, lchild.second + item + rchild.second);
		}
		else {
			//number owns the highest priority
			expr.emplace(2, Infix(1, item));
		}
	}
	return expr.top().second;
}

bool isSol(const Postfix& postfix) {
	std::stack<double> stk;
	double lc, rc;
	for (auto item : postfix) {
		if (isOp(item)) {
			rc = stk.top();
			stk.pop();
			lc = stk.top();
			stk.pop();
			switch (item) {
			case OP_ADD: lc += rc; break;
			case OP_SUB: lc -= rc; break;
			case OP_MUL: lc *= rc; break;
			case OP_DIV: lc /= rc; break;
			}
			stk.emplace(lc);
		}
		else {
			stk.emplace(item);
		}
	}
	return std::fabs(stk.top() - args_.aim) < EPS;
}

int permutePostfix(Solution& sols, Postfix& postfix, const int idx, const int op_cnt) {
	if (idx == postfix.size() - 1) {
		int issol = isSol(postfix);
		if (issol)
			sols.emplace_back(postfix);
		return issol;
	}
	int num_sols = 0;
	for (int i = idx; i < postfix.size(); i++) {
		int isop = isOp(postfix[i]);
		if (2 * (op_cnt + isop) >= idx + 1)	//cut invalid expr
			continue;
		std::swap(postfix[i], postfix[idx]);
		num_sols += permutePostfix(sols, postfix, idx + 1, op_cnt + isop);
		std::swap(postfix[i], postfix[idx]);
		if ((args_.smode & SM_FSEX_MSK) && num_sols)	//find one per op
			break;
	}
	return num_sols;
}

int exhaustCombination(Solution& results, Postfix& postfix, const int idx, const char last, const char max) {
	if (idx == postfix.size()) {
		results.emplace_back(postfix);
		return 1;
	}
	int cnt = 0;
	auto& item = postfix[idx];
	for (item = last; item <= max; item++)
		cnt += exhaustCombination(results, postfix, idx + 1, item, max);
	return cnt;
}

int exhaustCCount(const int min, const int max, const int digit) {
	if (min > max) return 0;
	double s = 1, a = max - min, b = 0;
	while (b < digit) s *= ++a / ++b;
	return (int)s;
}

void productPostfix(Solution& results, const Solution& postfixA, const Solution& postfixB) {
	results.reserve(results.size() + postfixA.size() + postfixA.size() * postfixB.size());
	for (auto& a : postfixA) {
		results.emplace_back(a);
		for (auto& b : postfixB) {
			results.emplace_back(a + b);
		}
	}
}


void solveThread(Solution& output, const Solution& input) {
	int nsols = 0;
	for (auto& item : input) {
		if (isNumList(item)) {
			output.emplace_back(item);
			nsols = 0;
			continue;
		}
		if ((args_.smode & SM_FSNU_MSK) && nsols)	//find one per number
			continue;
		Postfix postfix(item);
		nsols += permutePostfix(output, postfix, 0, 0);
	}
}

void solveMultiThread(Solution& output, const Solution& input) {
	if (args_.cthread == 1 || input.size() <= 2) {
		solveThread(output, input);
		return;
	}

	int thread_cnt = std::min(args_.cthread, (int)input.size());
	int step = (int)input.size() / thread_cnt;
	int split = (int)input.size() % thread_cnt;

	SList solutions(thread_cnt);
	TList threads(thread_cnt);
	int start = 0;
	for (int i = 0; i < thread_cnt; i++) {
		int stop = start + step + (i < split);
		threads[i] = std::thread(solveThread, std::ref(solutions[i]), Solution(input.cbegin() + start, input.cbegin() + stop));
		start = stop;
	}

	//combine results
	for (int i = 0; i < thread_cnt; i++) {
		threads[i].join();
		if ((args_.smode & SM_FSNU_MSK) && solutions[i].size() && isExpr(solutions[i].front()))	//remove redundant result in per number mode
			while (output.size() && isExpr(output.back()))
				output.pop_back();
		concat(output, solutions[i]);
	}
}

Solution solve() {
	Solution number_list;
	if (args_.smode & SM_TSN_MSK) {	//single number task
		number_list.emplace_back(args_.nums.cbegin(), args_.nums.cend());
	}
	else {	//multi number task
		Postfix num(args_.digit, 0);
		number_list.reserve(exhaustCCount(args_.min, args_.max, args_.digit));
		exhaustCombination(number_list, num, 0, args_.min, args_.max);
	}

	Solution op_list;
	if (args_.smode & SM_TSO_MSK) {	//single operator task
		op_list.emplace_back(args_.ops.cbegin(), args_.ops.cend());
	}
	else {	//multi operator task
		Postfix op(args_.digit - 1, OP_NOP);
		op_list.reserve(exhaustCCount(OP_MIN, OP_MAX, args_.digit - 1));
		exhaustCombination(op_list, op, 0, OP_MIN, OP_MAX);
	}

	Solution sols, raw_sols, combi;
	productPostfix(combi, number_list, op_list);
	solveMultiThread(raw_sols, combi);
	if (args_.smode & SM_SSH_MSK) {	//show all numbers
		sols = std::move(raw_sols);
	}
	else {	//remove numbers which have no solutions
		sols.reserve(raw_sols.size());
		for (int i = 0; i < raw_sols.size(); i++) {
			if (isNumList(raw_sols[i]) && sols.size() && isNumList(sols.back()))
				sols.pop_back();
			sols.emplace_back(std::move(raw_sols[i]));
		}
		while (sols.size() && isNumList(sols.back()))
			sols.pop_back();
	}
	return sols;
}


std::string decodeInfix(const Infix& infix) {
	std::string expr;
	for (auto item : infix) {
		switch (item) {
		case OP_ADD: expr += '+'; break;
		case OP_SUB: expr += '-'; break;
		case OP_MUL: expr += '*'; break;
		case OP_DIV: expr += '/'; break;
		case OP_LBK: expr += '('; break;
		case OP_RBK: expr += ')'; break;
		case OP_NOP: break;
		default: expr += std::to_string(item); break;
		}
	}
	return expr;
}

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Solution& sols) {
	os << "aim = " << args_.aim << "  ";
	
	if (args_.smode & SM_TSN_MSK) {	//single number task
		os << "numbers: ";
		for (auto item : args_.nums) os << item << ' ';
		os << ' ';
	}
	else {	//multi number task
		os << "min = " << args_.min << "  ";
		os << "max = " << args_.max << "  ";
		os << "digit = " << args_.digit << "  ";
	}

	if (args_.smode & SM_TSO_MSK) {	//single operator task
		os << "operators: ";
		for (auto item : args_.ops) {
			switch (item) {
			case OP_ADD: os << "+ "; break;
			case OP_SUB: os << "- "; break;
			case OP_MUL: os << "* "; break;
			case OP_DIV: os << "/ "; break;
			}
		}
		os << " ";
	}

	os << '\n' << std::string(60, '-');
	for (auto& expr : sols) {
		if (isNumList(expr)) {
			os << '\n';
			for (auto item : expr) os << (int)item << ' ';
			os << ":  ";
		}
		else {
			os << decodeInfix(convertPostfixInfix(expr)) << "  ";
		}
	}
	return os;
}

int parseNumber(const std::string& arg) {
	int x = 0;
	size_t pos = 0;
	x = std::stoi(arg, &pos);
	if (pos != arg.size())
		throw std::invalid_argument("invalid number");
	return x;
}

char parseOperator(const std::string& arg) {
	if (arg.size() != 1)
		throw std::invalid_argument("invalid operator");
	char ch = 0;
	switch (arg[0]) {
	case '+': ch = OP_ADD; break;
	case '-': ch = OP_SUB; break;
	case '*': ch = OP_MUL; break;
	case '/': ch = OP_DIV; break;
	default: throw std::invalid_argument("invalid operator");
	}
	return ch;
}

//true, if successful
//false, if help
//exception, if error
bool parseArgs(const std::vector<std::string>& args) {
	if (args.size() < 1)
		return false;
	bool paim = false, pmode = false, pfile = false, pjob = false, psingle = false, pshow = false;
	int pos = 0;
	while (pos < args.size()) {
		if (!args[pos].compare("--help")) {
			return false;
		}
		else if (!paim && !args[pos].compare("--aim")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 2)
				throw std::invalid_argument("unexpected argument");
			args_.aim = parseNumber(args[start + 1]);

			paim = true;
		}
		else if (!pmode && !args[pos].compare("--solve")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs < 3)
				throw std::invalid_argument("unexpected argument");
			for (int i = start + 1; i < start + nargs; i++) {
				int x = parseNumber(args[i]);
				if (x < 0)
					throw std::invalid_argument("unexpected negative number");
				args_.nums.push_back(x);
			}
			args_.digit = (int)args_.nums.size();
			args_.smode = SM_T_SNMO;

			pmode = true;
		}
		else if (!pmode && !args[pos].compare("--exhaust")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 4)
				throw std::invalid_argument("unexpected argument");
			args_.min = parseNumber(args[start + 1]);
			args_.max = parseNumber(args[start + 2]);
			args_.digit = parseNumber(args[start + 3]);
			if (args_.min > args_.max || args_.min < 0 || args_.digit <= 0)
				throw std::invalid_argument("invalid args");
			args_.smode = SM_T_MNMO;

			pmode = true;
		}
		else if (!pmode && !args[pos].compare("--test")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs < 4 || nargs % 2)
				throw std::invalid_argument("unexpected argument");
			args_.digit = nargs / 2;
			for (int i = start + 1; i < start + 1 + args_.digit; i++) {
				int x = parseNumber(args[i]);
				if (x < 0)
					throw std::invalid_argument("unexpected negative number");
				args_.nums.push_back(x);
			}
			for (int i = start + 1 + args_.digit; i < start + nargs; i++)
				args_.ops.push_back(parseOperator(args[i]));
			args_.smode = SM_T_SNSO;

			pmode = true;
		}
		else if (!pmode && !args[pos].compare("--search")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs < 4)
				throw std::invalid_argument("unexpected argument");
			args_.min = parseNumber(args[start + 1]);
			args_.max = parseNumber(args[start + 2]);
			args_.digit = nargs - 2;
			if (args_.min > args_.max || args_.min < 0)
				throw std::invalid_argument("invalid args");
			for (int i = start + 3; i < start + nargs; i++)
				args_.ops.push_back(parseOperator(args[i]));
			args_.smode = SM_T_MNSO;

			pmode = true;
		}
		else if (!pfile && !args[pos].compare("--out")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 2)
				throw std::invalid_argument("unexpected argument");
			args_.outfile.open(args[start + 1], std::ios_base::out | std::ios_base::trunc);
			if (!args_.outfile.is_open())
				throw std::logic_error("unable to open file");

			pfile = true;
		}
		else if (!pjob && !args[pos].compare("--job")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 2)
				throw std::invalid_argument("unexpected argument");
			int x = parseNumber(args[start + 1]);
			if (x <= 0)
				throw std::invalid_argument("invalid jobs");
			args_.cthread = x;

			pjob = true;
		}
		else if (!psingle && !args[pos].compare("--sop")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 1)
				throw std::invalid_argument("unexpected argument");
			args_.smode |= SM_F_SEXPR;

			psingle = true;
		}
		else if (!psingle && !args[pos].compare("--snum")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 1)
				throw std::invalid_argument("unexpected argument");
			args_.smode |= SM_F_SNUMB;

			psingle = true;
		}
		else if (!psingle && !args[pos].compare("--sall")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 1)
				throw std::invalid_argument("unexpected argument");
			args_.smode |= SM_F_MULTI;

			psingle = true;
		}
		else if (!pshow && !args[pos].compare("--show")) {
			int start = pos++;
			while (pos < args.size() && args[pos].compare(0, 2, "--")) ++pos;
			int nargs = pos - start;

			if (nargs != 1)
				throw std::invalid_argument("unexpected argument");
			args_.smode |= SM_S_SHOW;

			pshow = true;
		}
		else {
			throw std::invalid_argument("unexpected argument");
		}
	}

	if (!paim)
		throw std::invalid_argument("no aim is specified");
	if (!pmode)
		throw std::invalid_argument("no task is specified");
	if (!pjob)
		args_.cthread = std::thread::hardware_concurrency();
	if (!psingle)
		args_.smode |= SM_F_SEXPR;
	if (!pshow)
		args_.smode |= SM_S_HIDE;
	return true;
}


int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++)
		args.emplace_back(argv[i]);
	bool ret;
	try {
		ret = parseArgs(args);
	}
	catch (std::logic_error& e) {
		std::cerr << e.what() << std::endl;
		return 0;
	}
	if (ret) {
		Solution sols = solve();
		if (args_.outfile.is_open())
			args_.outfile << sols << std::endl;
		else
			std::cout << sols << std::endl;
	}
	else {
		std::cout << USAGE << std::endl;
	}
	return 0;
}
