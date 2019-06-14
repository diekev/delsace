#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>

template <typename C>
void print_array(C array)
{
	for (const auto &element : array) {
		std::cout << element << " ";
	}
	std::cout << std::endl;
}

template<typename C>
void shuffle_deck(C &to_shuffle, C &from, C tri)
{
	from = to_shuffle;
	for (auto i(0ul), e(from.size()); i < e; ++i) {
		to_shuffle[i] = from[tri[i]];
	}
	print_array(to_shuffle);
}

template<typename C>
void shuffle_deck(C &to_shuffle, C &from, bool is_even)
{
	from = to_shuffle;
	auto size = from.size() - 1;
	if (is_even) {
		for (auto i(0ul); i < size; ++i) {
			to_shuffle[(i * 2) % (size)] = from[i] ;
		}
	}
	else {
		for (auto i(1ul); i < size; ++i) {
			to_shuffle[(i * 2 + 1) % (size)] = from[i];
		}
	}
	print_array(to_shuffle);
}

template <typename T>
std::string to_binary(T x)
{
	std::string s;
	do {
		s.push_back('0' + (x & 1));
	} while(x >>= 1);
	std::reverse(s.begin(), s.end());
	return s;
}

int main()
{
	const int num_cards = 52;

	std::array<int, num_cards / 2> first_half;
	std::array<int, num_cards / 2> second_half;;

	std::iota(first_half.begin(), first_half.end(), 0);
	std::iota(second_half.begin(), second_half.end(), num_cards / 2);

//	print_array(first_half);
//	print_array(second_half);

	std::array<int, num_cards> tri_pair;
	std::array<int, num_cards> tri_impair;

	std::iota(tri_pair.begin(), tri_pair.end(), 0);
	std::iota(tri_impair.begin(), tri_impair.end(), 0);

	auto index1(0), index2(0);
	for (auto i(0ul); (i < first_half.size() || i < second_half.size()); ++i) {
//		std::cout << i << "\n";
		if (i < first_half.size()) {
			tri_pair[index1++] = first_half[i];
			tri_impair[index2++] = second_half[i];
		}
		if (i < second_half.size()) {
			tri_pair[index1++] = second_half[i];
			tri_impair[index2++] = first_half[i];
		}
	}

//	print_array(tri_pair);
//	print_array(tri_impair);

	std::array<int, num_cards> deck, shuffled_deck;

	std::iota(deck.begin(), deck.end(), 0);
//	print_array(deck);
	std::iota(shuffled_deck.begin(), shuffled_deck.end(), 0);

	int iter = 1;

	do {
		iter++;
	}
	while ((iter * 2) % (num_cards - 1) != 1);

	auto carte(-1);
	std::cout << "Choisissez une carte (entre 1 et 52):\n";
	std::cin >> carte;

	auto index(-1);
	std::cout << "Choisissez un nombre entre 1 et 52:\n";
	std::cin >> index;

	auto it = std::find(shuffled_deck.begin(), shuffled_deck.end(), carte);
	std::rotate(shuffled_deck.begin(), it, it + 1);

	auto bin_num = to_binary(index - 1);
	print_array(shuffled_deck);
	for (auto i(0ul); i < bin_num.length(); ++i) {
		shuffle_deck(shuffled_deck, deck, (bin_num[i] == '0') ? tri_pair : tri_impair);
	}

	if (shuffled_deck[index - 1] == carte) {
		std::cout << "La carte a été trouvé !\n";
	}
}
