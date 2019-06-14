#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " string\n";
		return 1;
	}

	std::string input = argv[1];

	if (input.size() == 1) {
		std::cout << "-1\n";
		return 0;
	}

	std::vector<int> results;

	size_t open_paren = 0;
	size_t clos_paren = 0;
	size_t hash_tags = 0;

	for (size_t i(0); i < input.size(); ++i) {
		if (input[i] == '(') {
			++open_paren;
		}
		else if (input[i] == ')') {
			++clos_paren;
		}
		else {
			++hash_tags;
		}
	}

	if ((open_paren <= clos_paren) && (hash_tags != 0)) {
		std::cout << "-1\n";
		return 0;
	}

	open_paren = 0;
	clos_paren = 0;

	for (size_t i(0); i < input.size(); ++i) {
		if (input[i] == '(') {
			++open_paren;
		}
		else if (input[i] == ')') {
			++clos_paren;
		}
		else  if (input[i] == '#') {
			if (i < input.size()) {
				if ((open_paren != 0) && (clos_paren == 0)) {
					if (input[i + 1] != ')') {
						continue;
					}

					++clos_paren;
					++i;
				}
			}

			int diff = open_paren - clos_paren;
			clos_paren += diff;

			results.push_back(diff);
		}
	}

	for (size_t i(0); i < results.size(); ++i) {
		std::cout << results[i] << "\n";
	}

	return 0;
}
