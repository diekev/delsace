#include <algorithm>
#include <iostream>
#include <sstream>

/**
 * Generates the following pattern (with 4 rows):
 *
 * ########
 *  ######
 *   ####
 *    ##
 */
void ex_2_1(std::istream &is, std::ostream &os)
{
	os << "Enter the number of rows (must be an even number): ";

	int rows;
	is >> rows;

	if (rows & 0x1) {
		rows &= ~0x1;
		os << "Odd number of rows given, rounding down to " << rows << "\n";
	}

	const auto columns = rows * 2;

	for (int j = 0; j < rows; ++j) {
		for (auto i = 0; i < j; ++i) {
			os << ' ';
		}

		for (auto i = 0, ie = columns - (j << 1); i < ie; ++i) {
			os << '#';
		}

		os << '\n';
	}
}

/**
 * Generates the following pattern (with 8 rows):
 *
 *    ##
 *   ####
 *  ######
 * ########
 * ########
 *  ######
 *   ####
 *    ##
 *
 * Here the idea is to generate a sequence of number such that we can use it for
 * both the spaces and hash marks. In order to do so, we first consider for each
 * line the number of spaces and hash marks, they are respectively:
 * 3, 2, 1, 0, 0, 1, 2, 3
 * 2, 4, 6, 8, 8, 6, 4, 2
 *
 * We will consider this sequence:
 * 6, 4, 2, 0, 0, 2, 4, 6
 *
 * Dividing by two will give us the spaces, whilst subtracting it from 8 will
 * give us the number of hash marks.
 *
 * The trick here to achieve simetry is to use odd numbers, therefore the
 * sequence to generate will be:
 * 7, 5, 3, 1, 1, 3, 5, 7
 *
 * To get it we simply multiply the current value of the row by two and then
 * subtract 9 from it (i.e.: 8 * 2 - 9 = 7), but this gives:
 * 7, 5, 3, 1, -1, -3, -5, -7
 * thus we'll consider the absolute value of the result, giving: f(x) = |2x - 9|
 */
void ex_2_2(std::istream &is, std::ostream &os)
{
	os << "Enter the number of rows (must be an even number): ";

	int rows;
	is >> rows;

	if (rows & 0x1) {
		rows &= ~0x1;
		os << "Odd number of rows given, rounding down to " << rows << "\n";
	}

	for (int j = rows; j > 0; --j) {
		const auto count = std::abs((j << 1) - (rows + 1));

		/* should be `(count - 1) / 2` but integer division does it for us */
		for (int i = 0, ie = count / 2; i < ie; ++i) {
			os << ' ';
		}

		for (int i = 0, ie = (rows + 1) - count; i < ie; ++i) {
			os << '#';
		}

		os << '\n';
	}
}

/**
 * Generates the following pattern (with 8 rows):
 *
 * #            #
 *  ##        ##
 *   ###    ###
 *    ########
 *    ########
 *   ###    ###
 *  ##        ##
 * #            #
 *
 * For each line we need to output n0 spaces, n1 hashtags, n2 spaces, n3
 * hashtags and n4 spaces. Let's write down their numbers in a table:
 * n0 n1  n2 n3 n4
 *  0  1  12  1  0
 *  1  2   8  2  1
 *  2  3   4  3  2
 *  3  4   0  4  3
 *  3  4   0  4  3
 *  2  3   4  3  2
 *  1  2   8  2  1
 *  0  1  12  1  0
 *
 * We can note that n0 = n4 and n1 = n3, n1 = n0 + 1 and n3 = n4 + 1.
 * Also n2 = 14 - (n0 + n1 + n3 + n4) = 14 - 2(n0 + n1).
 *
 * Let's generate the sequence for n0:
 * 0, 1, 2, 3, 3, 2, 1, 0 (x)
 *
 * We will use the same approach as ex_2_2, using odd numbers:
 * 0, 2, 4, 6, 6, 4, 2, 0 (x * 2)
 * 1, 3, 5, 7, 7, 5, 3, 1 (x + 1)
 * 7, 5, 3, 1, 1, 3, 5, 7 (8 - x)
 *
 * The last line is the same as the one in ex_2_2 (f(x) = |2x - 9|). So the
 * sequence for n0 can be gererated with f(x) = (8 - |2x - 9| - 1) / 2
 *
 * Recall that n1 = n0 + 1, and n2 = 14 - 2(n0 + n1).
 */
void ex_2_3(std::istream &is, std::ostream &os)
{
	os << "Enter the number of rows (must be an even number): ";

	int rows;
	is >> rows;

	if (rows & 0x1) {
		rows &= ~0x1;
		os << "Odd number of rows given, rounding down to " << rows << "\n";
	}

	const auto columns = rows * 2 - 2;

	for (int i = 1; i <= rows; ++i) {
		const auto n0 = (rows - std::abs((i << 1) - (rows + 1)) - 1) >> 1;
		const auto n1 = n0 + 1;
		const auto n2 = columns - 2 * (n0 + n1);

		for (int e = 0; e < n0; ++e) {
			os << ' ';
		}

		for (int j = 0; j < n1; ++j) {
			os << '#';
		}

		for (int j = 0; j < n2; ++j) {
			os << ' ';
		}

		for (int j = 0; j < n1; ++j) {
			os << '#';
		}

		for (int e = 0; e < n0; ++e) {
			os << ' ';
		}

		os << '\n';
	}
}

void ex_2_4(std::istream &is, std::ostream &os)
{
	os << "Enter the number of rows (must be an even number): ";
	int rows;

	is >> rows;

	if ((rows & 0x1) != 0) {
		rows &= ~0x1;
		os << "Odd number of rows given, rounding down to " << rows << "\n";
	}

	const auto columns = rows + 2;

	for (int i = 0; i <= rows; ++i) {
		for (int j = 0; j <= columns; ++j) {
			const auto sign = ((i + j) % 2 == 0) ? '#' : ' ';
			os << sign;
		}

		os << '\n';
	}
}

auto double_digit_value(int digit) -> int
{
	int doubled_digit = digit * 2;
	return (doubled_digit > 10) ? 1 + doubled_digit % 10 : doubled_digit;
}

void luhn_formula(std::istream &is, std::ostream &os)
{
	auto checksum_odd_length = 0;
	auto checksum_even_length = 0;
	auto position = 1;

	os << "Enter a number: ";
	auto digit = is.get();

	while (digit != 10) {
		if (position % 2 == 0) {
			checksum_odd_length += double_digit_value(digit - '0');
			checksum_even_length += digit - '0';
		}
		else {
			checksum_odd_length += digit - '0';
			checksum_even_length += double_digit_value(digit - '0');
		}
		digit = is.get();
		position++;
	}

	auto checksum = ((position - 1) % 2 == 0) ? checksum_even_length : checksum_odd_length;

	os << "Checksum is " << checksum << ".\n";

	if (checksum % 10 == 0) {
		os << "Checksum is divisible by 10. Valid.\n";
	}
	else {
		os << "Checksum is not divisible by 10. Invalid.\n";
	}
}

/* ISBN-10 & ISBN-13 number check.
 * Also generate checkdigit if missing.
 */
void ex_2_5(std::istream &is, std::ostream &os)
{
	os << "Enter an ISBN number: ";

	std::string isbn;
	is >> isbn;

	auto checksum_10 = 0;
	auto checksum_13 = 0;
	auto position = 0;
	auto multiplier = 10;

	for (const auto &ch : isbn) {
		checksum_10 += (ch == 'X') ? 10 : multiplier * (ch - '0');
		checksum_13 += (position % 2 == 0) ? 3 * (ch - '0') : (ch - '0');
		++position;
		--multiplier;
	}

	auto is_isbn10 = (position - 1 <= 10);

	if (!is_isbn10 && position - 1 < 13) {
		const auto checkdigit = 10 - (checksum_13 % 10);
		checksum_13 += checkdigit;
		os << "ISBN-13 number.\n";
		os << "Checkdigit is missing, computing it...\n";
	}

	if (is_isbn10 && position - 1 < 10) {
		const auto checkdigit = 11 - (checksum_10 % 11);
		checksum_10 += checkdigit;
		os << "ISBN-10 number.\n";
		os << "Checkdigit is missing, computing it...\n";
	}

	const auto checksum = (is_isbn10) ? checksum_10 : checksum_13;
	const auto checkdigit = (is_isbn10) ? 11 : 10;
	if (checksum % checkdigit == 0) {
		os << "Valid ISBN number.\n";
	}
	else {
		os << "Invalid ISBN number.\n";
	}
}

int binary_to_decimal(int binary)
{
	auto decimal = 0l, rem = 0l, base = 1l;

	while (binary > 0) {
		rem = binary % 10;
		decimal = decimal + rem * base;
		base *= 2;
		binary /= 10;
	}

	return decimal;
}

std::string decimal_to_binary(const int decimal)
{
	std::string s("");
	auto x = decimal;

	do {
		s.push_back('0' + (x & 1));
	} while(x >>= 1);

	std::reverse(s.begin(), s.end());

	return s;
}

/**
 * This program converts a binary number into a base-10 number, et vice versa.
 */
void ex_2_6(std::istream &is, std::ostream &os)
{
	os << "Enter a binary number: ";
	auto num = 0;
	is >> num;

	auto decimal = binary_to_decimal(num);
	os << "The decimal equivalent of " << num << " is " << decimal << '\n';

	std::string s = decimal_to_binary(decimal);

	os << "The binary equivalent of " << decimal << " is " << s << '\n';
}

char dec_to_hex(int num)
{
	if (num >= 10) {
		switch (num) {
			case 0xa: return 'a';
			case 0xb: return 'b';
			case 0xc: return 'c';
			case 0xd: return 'd';
			case 0xe: return 'e';
			case 0xf: return 'f';
		}
	}

	return '0' + num;
}

std::string convert_to_hex(int value)
{
	std::string str("");

	int quotient = value;
	int remainder = value;

	while (quotient >= 16) {
		remainder = quotient % 16;
		quotient /= 16;

		str.push_back(dec_to_hex(remainder));
	}

	str.push_back(dec_to_hex(quotient));

	std::reverse(str.begin(), str.end());

	return str;
}

int hex_to_dec(char hex)
{
	switch (hex) {
		case '0': return 0x0;
		case '1': return 0x1;
		case '2': return 0x2;
		case '3': return 0x3;
		case '4': return 0x4;
		case '5': return 0x5;
		case '6': return 0x6;
		case '7': return 0x7;
		case '8': return 0x8;
		case '9': return 0x9;
		case 'a': return 0xa;
		case 'b': return 0xb;
		case 'c': return 0xc;
		case 'd': return 0xd;
		case 'e': return 0xe;
		case 'f': return 0xf;
	}

	return -1;
}

int hex_str_to_dec(const std::string &hex)
{
	size_t num = hex.size();
	int result = 0;
	size_t idx = 0;

	while (num--) {
		result += hex_to_dec(hex[num]) * std::pow(16, idx++);
	}

	return result;
}

std::string hex_to_bin(char hex)
{
	switch (hex) {
		case '0': return "0000";
		case '1': return "0001";
		case '2': return "0010";
		case '3': return "0011";
		case '4': return "0100";
		case '5': return "0101";
		case '6': return "0110";
		case '7': return "0111";
		case '8': return "1000";
		case '9': return "1001";
		case 'a': return "1010";
		case 'b': return "1011";
		case 'c': return "1100";
		case 'd': return "1101";
		case 'e': return "1110";
		case 'f': return "1111";
	}

	return "";
}

std::string hex_str_to_bin_str(const std::string &hex)
{
	std::string result;
	result.reserve(hex.size() * 4);

	for (size_t i(0); i < hex.size(); ++i) {
		result += hex_to_bin(hex[i]);
	}

	return result;
}

char bin_to_hex(std::string hex)
{
	std::cout << hex << "\n";
	if      (hex == "0000") return '0';
	else if (hex == "0001") return '1';
	else if (hex == "0010") return '2';
	else if (hex == "0011") return '3';
	else if (hex == "0100") return '4';
	else if (hex == "0101") return '5';
	else if (hex == "0110") return '6';
	else if (hex == "0111") return '7';
	else if (hex == "1000") return '8';
	else if (hex == "1001") return '9';
	else if (hex == "1010") return 'a';
	else if (hex == "1011") return 'b';
	else if (hex == "1100") return 'c';
	else if (hex == "1101") return 'd';
	else if (hex == "1110") return 'e';
	else                    return 'f';
}

std::string bin_str_to_hex_str(const std::string &bin)
{
	std::string result;
	result.reserve(bin.size() / 4);

	for (size_t i(0); i < bin.size(); i += 4) {
		result.push_back(bin_to_hex(bin.substr(i, 4)));
	}

	return result;
}

enum {
	BINARY_TO_DECIM = 0,
	BINARY_TO_HEX = 1,
	DECIM_TO_BINARY = 2,
	DECIM_TO_HEX = 3,
	HEX_TO_BINARY = 4,
	HEX_TO_DECIM = 5
};

/**
 * This program converts between decimal, hexadecimal and binary
 */
void ex_2_7(std::istream &is, std::ostream &os)
{
	os << "Choose from what to convert to what: (Enter the corresponding number)\n";
	os << "1. from binary to decimal\n";
	os << "2. from binary to hexadecimal\n";
	os << "3. from decimal to binary\n";
	os << "4. from decimal to hexadecimal\n";
	os << "5. from hexadecimal to binary\n";
	os << "6. from hexadecimal to decimal\n";

	auto choice = -1;
	is >> choice;

	if (choice == -1) {
		os << "Exiting.\n";
		return;
	}

	os << "Enter a number: ";

	int decimal;
	std::string hex, binary;

	switch (choice - 1) {
		case BINARY_TO_DECIM:
			is >> binary;
			decimal = binary_to_decimal(std::atoi(binary.c_str()));
			break;
		case BINARY_TO_HEX:
			is >> binary;
			hex = bin_str_to_hex_str(binary);
			break;
		case DECIM_TO_BINARY:
			is >> decimal;
			binary = decimal_to_binary(decimal);
			break;
		case DECIM_TO_HEX:
			is >> decimal;
			hex = convert_to_hex(decimal);
			break;
		case HEX_TO_BINARY:
			is >> hex;
			binary = hex_str_to_bin_str(hex);
			break;
		case HEX_TO_DECIM:
			is >> hex;
			decimal = hex_str_to_dec(hex);
			break;
	}

	os << "Result: ";
	if (choice == 1 || choice == 6) {
		os << decimal << '\n';
	}
	else if (choice == 2 || choice == 4) {
		os << hex << '\n';
	}
	else if (choice == 3 || choice == 5) {
		os << binary << '\n';
	}
}

/**
 * This program outputs some stats about the text it was fed:
 * - number of words
 * - max word length
 * - max vowel count
 */
void ex_2_9(std::istream &is, std::ostream &os)
{
	os << "Type a phrase and hit enter: ";
	char letter = is.get();
	auto num_words = 0;
	auto cur_length = 0, max_length = 0;
	auto cur_vowels = 0, max_vowel = 0;
	std::string vowels("aeiouyAEIOUY");

	while (letter != 10) {
		os << letter;

		if (letter == ' ') {
			cur_length = -1;
			cur_vowels = 0;
			num_words++;
		}

		if (vowels.find(letter) != std::string::npos) {
			cur_vowels++;
		}

		cur_length++;
		letter = is.get();

		if (cur_length > max_length) {
			max_length = cur_length;
		}

		if (cur_vowels > max_vowel) {
			max_vowel = cur_vowels;
		}
	}

	/* We always have one more word than spaces */
	num_words++;
	os << '\n';
	os << "Number of words: " << num_words << '\n';
	os << "The longest word has a length of: " << max_length << '\n';
	os << "The greatest number of vowels in a word is: " << max_vowel << '\n';
}

int main()
{
	std::istream &is = std::cin;
	std::ostream &os = std::cout;

	os << "Choose an exercise:\n";
	os << "1. Pattern generation.\n";
	os << "2. Pattern generation.\n";
	os << "3. Pattern generation.\n";
	os << "4. Pattern generation.\n";
	os << "5. ISBN number checking.\n";
	os << "6. Convert numbers between binary and base-10.\n";
	os << "7. Convert numbers between decimal, hexadecimal and binary.\n";
	os << "8. String statistics.\n";

	int exercise;
	is >> exercise;

	switch (exercise) {
		case 1: ex_2_1(is, os); break;
		case 2: ex_2_2(is, os); break;
		case 3: ex_2_3(is, os); break;
		case 4: ex_2_4(is, os); break;
		case 5: ex_2_5(is, os); break;
		case 6: ex_2_6(is, os); break;
		case 7: ex_2_7(is, os); break;
		case 8: ex_2_9(is, os); break;
		default: break;
	}
}
