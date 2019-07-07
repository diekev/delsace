
#include "jit/compiler.h"
#include "jit/function.h"
#include "jit/postfix.h"

#include "static/filebuffer.h"
#include "static/scanner.h"

#include "makego.h"

#include "../../biblexternes/docopt/docopt.hh"
#include <iostream>

static const char usage[] = R"(
Compiler.

Usage:
    compiler --jit
    compiler --adder
    compiler --makego FILE
    compiler --scanner FILE
    compiler --postfix --expression=<expr>
    compiler (-h | --help)
    compiler --version

Options:
	-h, --help           Show this screen.
	--version            Show version.
	--jit                Test JIT compiler.
	--makego             Test MakeGO compiler.
	--postfix            Test PostFix evaluator.
	--expression=<expr>  Expression to use for postfix evaluation.
	--scanner            Test scanner.
	--adder              Test adder.
)";

static void test_jit()
{
	code_vector code;

	/* mov %rdi %rax */
	code.insert(code.end(), { 0x48, 0x89, 0xf8 });

	int c, dummy;
	while ((c = std::fgetc(stdin)) != '\n' && c != EOF) {
		if (c == ' ') {
			continue;
		}

		auto operator_ = static_cast<char>(c);
		long operand;

		dummy = std::scanf("%ld", &operand);

		/* mov operand, %rdi */
		code.insert(code.end(), { 0x48, 0xbf });

		code.push_value(code.end(), operand);

		switch (operator_) {
			case '+':
				/* add %rdi, %rax */
				code.insert(code.end(), { 0x48, 0x01, 0xf8 });
				break;
			case '-':
				/* sub %rdi, %rax */
				code.insert(code.end(), { 0x48, 0x29, 0xf8 });
				break;
			case '*':
				/* imul %rdi, %rax */
				code.insert(code.end(), { 0x48, 0x0f, 0xaf, 0xc7 });
				break;
			case '/':
				/* xor %rdx, %rdx */
				code.insert(code.end(), { 0x48, 0x31, 0xd2 });
				/* idiv  %rdi */
				code.insert(code.end(), { 0x48, 0xf7, 0xff });
				break;
		}
	}

	/* ret */
	code.insert(code.end(), { 0xc3 });

	long init;
	unsigned long term;
	dummy = std::scanf("%ld %lu", &init, &term);

	auto recurrence = function<long(long)>(code.begin(), code.end());

	for (long i = 0, x = init; i <= static_cast<long>(term); i++, x = recurrence(x)) {
		std::fprintf(stderr, "Term %lu: %ld\n", i, x);
	}

	static_cast<void>(dummy);
}

static void test_scanner(const std::string &filename)
{
	FileBuffer map(filename);

	if (!map) {
		std::printf("Failed to open file or file is empty!\n");
		return;
	}

	std::printf("Source: %s\n", map.begin());

	try {
		Scanner scanner(map.begin(), map.end());
		scanner.read();

		float result = scan_expression(scanner);

		if (scanner.token() != Token::EndOfFile) {
            throw SyntaxException(SyntaxError::UnexpectedToken,
                                  scanner.line());
        }

		std::printf("Result: %f\n", static_cast<double>(result));
	}
	catch (const SyntaxException &e) {
		std::cerr << "Syntax error at line: " << e.line << ": ";

		switch (e.error) {
			case SyntaxError::InvalidToken:
				std::cerr << "invalid token.\n";
				break;
			case SyntaxError::PrimaryExpected:
				std::cerr << "expected a primary expression.\n";
				break;
			case SyntaxError::DivideByZero:
				std::cerr << "division by zero.\n";
				break;
			case SyntaxError::RightParenthesisExpected:
				std::cerr << "missing right parenthesis.\n";
				break;
			case SyntaxError::UnexpectedToken:
				std::cerr << "unexpected token\n";
				break;
		}
	}
}

static function<int64_t(int64_t)> adder(int64_t n)
{
	code_vector code;
	code.insert(code.end(), { 0x48, 0xB8 } );

	code.push_value(code.end(), n);

	/* add %rdi, %rax */
	code.insert(code.end(), { 0x48, 0x01, 0xF8 });

	/* ret */
	code.insert(code.end(), { 0xC3 });

	return function<int64_t(int64_t)>(code.begin(), code.end());
}

template <typename T>
static auto debug_queue(dls::file<T> queue)
{
	while (!queue.est_vide()) {
		std::cerr << queue.front() << '\n';
		queue.pop();
	}
}

#if 0
static function<int(void)> evaluate_postfix(dls::file<std::string> &expression)
{
	code_vector code;

	/* mov %rdi %rax */
	code.insert(code.end(), { 0x48, 0x89, 0xf8 });

//	code.insert(code.end(), { 0x48, 0xB8 } );
//	code.push_value(code.end(), 0);
	/* Push a zero on the stack in case the expression starts with a negative
	 * number, or is empty. */

	while (!expression.empty()) {
		auto token = expression.front();
		expression.pop();

		if (is_operator(token)) {
			if (token == "+") {
				/* add %rdi, %rax */
				code.insert(code.end(), { 0x48, 0x01, 0xf8 });
			}
			else if (token == "-") {
				/* sub %rdi, %rax */
				code.insert(code.end(), { 0x48, 0x29, 0xf8 });
			}
			else if (token == "*") {
				/* imul %rdi, %rax */
				code.insert(code.end(), { 0x48, 0x0f, 0xaf, 0xc7 });
			}
			else if (token == "/") {
				/* xor %rdx, %rdx */
				code.insert(code.end(), { 0x48, 0x31, 0xd2 });
				/* idiv  %rdi */
				code.insert(code.end(), { 0x48, 0xf7, 0xff });
			}

			continue;
		}
		else {
			int val = std::stoi(token);

			/* mov operand, %rdi */
			code.insert(code.end(), { 0x48, 0xbf });
			code.push_value(code.end(), val);

			continue;
		}
	}

	code.insert(code.end(), { 0xC3 });

	return function<int(void)>(code.begin(), code.end());
}
#else
static function<int(void)> evaluate_postfix(dls::file<std::string> &expression)
{
	jit::state state;

	state.emit(jit::operation::mov_reg, jit::reg::r1, jit::reg::r0);

	int stack_ptr = 0;

	while (!expression.est_vide()) {
		auto token = expression.front();
		expression.defile();

		if (is_operator(token)) {
			if (token == "+") {
				state.emit(jit::operation::pop, stack_ptr, jit::reg::fp, jit::reg::r0);
				stack_ptr -= static_cast<int>(sizeof(int));
				state.emit(jit::operation::add, jit::reg::r1, jit::reg::r0);
			}
			else if (token == "-") {
				state.emit(jit::operation::pop, stack_ptr, jit::reg::fp, jit::reg::r0);
				stack_ptr -= static_cast<int>(sizeof(int));
				state.emit(jit::operation::sub, jit::reg::r1, jit::reg::r0);
			}
			else if (token == "*") {
				state.emit(jit::operation::pop, stack_ptr, jit::reg::fp, jit::reg::r0);
				stack_ptr -= static_cast<int>(sizeof(int));
				state.emit(jit::operation::mul, jit::reg::r1, jit::reg::r0);
			}
			else if (token == "/") {
				state.emit(jit::operation::pop, stack_ptr, jit::reg::fp, jit::reg::r0);
				stack_ptr -= static_cast<int>(sizeof(int));
				state.emit(jit::operation::div, jit::reg::r1, jit::reg::r0);
			}

			continue;
		}
		else {
			int val = std::stoi(token);

			/* mov operand, %rdi */
			state.emit(jit::operation::push, stack_ptr, jit::reg::fp, jit::reg::r0);
			stack_ptr += static_cast<int>(sizeof(int));

			state.emit(jit::operation::mov, val, jit::reg::r0);

			continue;
		}
	}

	state.emit(jit::operation::ret);

	state.finalize();

	std::cerr << std::hex << std::showbase;

	for (auto i = 0ul; i < state.code().size(); i += 4) {
		std::cerr << *(reinterpret_cast<int *>(&(state.code()[i]))) << ' ';
	}

	std::cerr << '\n';

	return function<int(void)>((state.code()).begin(), (state.code()).end());
}
#endif

int main(int argc, char *argv[])
{
	auto args = dls::docopt::docopt(usage, { argv + 1, argv + argc }, true, "Compiler 0.1");

	const auto filename = dls::docopt::get_string(args, "FILE");
	const auto do_jit = dls::docopt::get_bool(args, "--jit");
	const auto do_makego = dls::docopt::get_bool(args, "--makego");
	const auto do_postfix = dls::docopt::get_bool(args, "--postfix");
	const auto do_scanner = dls::docopt::get_bool(args, "--scanner");
	const auto do_adder = dls::docopt::get_bool(args, "--adder");
	const auto postfix_expression = dls::docopt::get_string(args, "--expression=");

	if (do_jit) {
		test_jit();
	}
	else if (do_makego) {
		read_file(filename);
	}
	else if (do_postfix) {
		auto queue = postfix(postfix_expression);
		auto func = evaluate_postfix(queue);

		func();
	}
	else if (do_scanner) {
		test_scanner(filename);
	}
	else if (do_adder) {
		auto f = adder(42);
		auto f2 = f(53);

		std::cout << f(10) << " " << f2 << "\n";
	}
}
