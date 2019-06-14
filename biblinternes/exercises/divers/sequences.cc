#include <iostream>
#include <iterator>
#include <vector>

template <typename Container>
void print_vector(std::ostream &os, Container container)
{
	using value_type = typename Container::value_type;

	std::copy(container.begin(), container.end(),
	          std::ostream_iterator<value_type>(os, " "));

	os << "\n";
}

int main()
{
	std::vector<int> stern_brocot(2, 1);
	auto count = 100;

	for (int i = 1; i <= count; ++i) {
		stern_brocot.push_back(stern_brocot[i] + stern_brocot[i - 1]);
		stern_brocot.push_back(stern_brocot[i]);
	}

	print_vector(std::cout, stern_brocot);

	std::cout << "Golomb's sequence:\n";
	std::vector<int> golomb(50);
	golomb[0] = 1;

	for (int i = 1; i < 50; ++i) {
		golomb[i] = (1 + golomb[i - golomb[golomb[i - 1]]]);
	}

	print_vector(std::cout, golomb);
}
