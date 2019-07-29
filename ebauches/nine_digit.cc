#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <tbb/parallel_for.h>
#include <vector>

#include "../../repos/utils/util/profiling.h"

#define GENETIC_ALGORITHM

template <typename I, typename T>
auto madd_iter(I iter, T value) -> T
{
	return *iter * value;
}

template <typename I, typename T, typename... Ts>
auto madd_iter(I iter, T value, Ts... values) -> T
{
	return *iter * value + madd_iter(++iter, values...);
}

#ifdef GENETIC_ALGORITHM

int check_digit(const std::vector<int> &n)
{
	int fit = 0;

	/* the fifth number should always be 5 */
	if (n[4] == 5) {
		fit += 8;
	}

	/* the second number should always be a multiple of 2 */
	if (n[1] % 2 == 0) {
		fit += 7;
	}

	if ((n[0] + n[1] + n[2]) % 3 == 0) {
		fit += 6;
	}

	if ((2 * n[2] + n[3]) % 4 == 0) {
		fit += 5;
	}

//	auto first_six = 4 * (n[0] + n[1] + n[2] + n[3] + n[4]) + n[5];
	auto first_six = madd_iter(n.begin(), 4, 4, 4, 4, 4, 1);
	if (first_six % 6 == 0) {
		fit += 4;
	}

//	auto first_eight = 4 * n[5] + 2 * n[6] + n[7];
	auto first_eight = madd_iter(n.begin() + 5, 4, 2, 1);
	if (first_eight % 8 == 0) {
		fit += 3;
	}

//	auto first_seven = n[0] + 5 * n[1] + 4 * n[2] + 6 * n[3] + 2 * n[4] + 3 * n[5] + n[6];
	auto first_seven = madd_iter(n.begin(), 1, 5, 4, 6, 2, 3, 1);
	if (first_seven % 7 == 0) {
		fit += 2;
	}

	if (fit == 35) {
		fit = 36;
	}

	return fit;
}

class DNA {
	std::vector<int> m_genes;
	float m_fitness;
	size_t m_genes_size_sqr;
	float m_inv_gene_size;

public:
	DNA()
		: m_genes(9)
		, m_fitness(0.0f)
		, m_genes_size_sqr(1296)
		, m_inv_gene_size(1.0f / m_genes_size_sqr)
	{
		std::iota(m_genes.begin(), m_genes.end(), 1);
	}

	DNA(std::mt19937 &rng)
		: DNA()
	{
		std::shuffle(m_genes.begin(), m_genes.end(), rng);
	}

	void compute_fitness()
	{
		int fit = check_digit(m_genes);
		m_fitness = static_cast<float>(fit * fit) * m_inv_gene_size;
	}

	float fitness() const
	{
		return m_fitness;
	}

	DNA crossover(const DNA &other, std::mt19937 &rng) const
	{
		std::uniform_real_distribution<float> coin_flip(0.0f, 1.0f);
		DNA child;

		if (this->fitness() >= other.fitness()) {
			child.m_genes = this->m_genes;
		}
		else {
			child.m_genes = other.m_genes;
		}

		for (size_t i(0), ie = m_genes.size(); i < ie; ++i) {
			if (coin_flip(rng) < 0.5f) {
				const auto &a = m_genes[i] - 1;
				const auto &b = other.m_genes[i] - 1;
				std::swap(child.m_genes[a], child.m_genes[b]);
			}
		}

//		std::cout << "Parent A: " << *this;
//		std::cout << "Parent B: " << other;
//		std::cout << "Child:    " << child;
//		std::cout << "===============================\n";

		return child;
	}

	void mutate(std::mt19937 &rng)
	{
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		std::uniform_int_distribution<int> random_int(0, 8);

		for (size_t i(0), ie = m_genes.size(); i < ie; ++i) {
			if (dist(rng) < 0.01f) {
				std::swap(m_genes[random_int(rng)], m_genes[random_int(rng)]);
			}
		}
	}

	friend std::ostream &operator<<(std::ostream &os, const DNA &dna)
	{
		for (const auto &allele : dna.m_genes) {
			os << allele;
		}

		os << " (" << dna.m_fitness << ")\n";

		return os;
	}
};

using population_t = std::vector<DNA>;
using mating_pool_t = std::vector<size_t>;

void compute_fitness(population_t &population)
{
	auto compute_fitness_op = [&](const tbb::blocked_range<size_t> &r)
	{
		for (size_t i = r.begin(), ie = r.end(); i != ie; ++i) {
			population[i].compute_fitness();
		}
	};

	tbb::parallel_for(tbb::blocked_range<size_t>(0, population.size()),
	                  compute_fitness_op);
}

bool generate_mating_pool(mating_pool_t &mating_pool, population_t &population)
{
	float lowest = 1.0f;

	for (const auto &individual : population) {
		const auto &fitness = individual.fitness();

		if (fitness == 1.0f) {
			std::cout << "Exact match: " << individual;
			return true;
		}
		else if (fitness == 0.0f) {
			continue;
		}
		else if (fitness < lowest) {
			lowest = fitness;
		}
	}

	int multiplier = 1;
	while (lowest < 1.0f) {
		lowest *= 10;
		multiplier *= 10;
	}

	size_t i = 0;
	for (const auto &individual : population) {
		const auto &num = static_cast<int>(individual.fitness() * multiplier);
		const auto &prev_size = mating_pool.size();
		mating_pool.resize(prev_size + num);
		std::fill_n(mating_pool.begin() + prev_size, num, i++);
	}

	assert(mating_pool.size() > 1);

	return false;
}

population_t mate_population(const mating_pool_t &mating_pool,
                             const population_t &population,
                             std::mt19937 &rng)
{
	std::uniform_int_distribution<size_t> mating_dist(0, mating_pool.size() - 1);
	population_t new_population(population.size());

	for (auto &child : new_population) {
		auto a = mating_pool[mating_dist(rng)];
		auto b = mating_pool[mating_dist(rng)];

		while (a == b) {
			b = mating_pool[mating_dist(rng)];
		}

		const auto &mate_a = population[a];
		const auto &mate_b = population[b];

		child = mate_a.crossover(mate_b, rng);
		child.mutate(rng);
	}

	return new_population;
}

void find_best_match(population_t &population)
{
	float best_match = 0.0f;
	DNA best_dna;

	for (auto &individual : population) {
		individual.compute_fitness();

		if (individual.fitness() > best_match) {
			best_match = individual.fitness();
			best_dna = individual;
		}
	}

	std::cout << "Best match: " << best_dna;
}

population_t create_population(const size_t population_size,
                               std::mt19937 &rng)
{
	population_t population;
	population.reserve(population_size);

	for (size_t i = 0; i < population_size; ++i) {
		population.emplace_back(rng);
	}

	return population;
}

int main()
{
	Timer(__func__);

	std::mt19937 rng(19937);

	population_t population = create_population(150, rng);

	const auto &max_iter = 10000;
	auto iter = 0;

	while (iter++ < max_iter) {
		compute_fitness(population);

		mating_pool_t mating_pool;
		const bool done = generate_mating_pool(mating_pool, population);

		if (done) {
			std::cout << "Iterations: " << iter << "\n";
			break;
		}

		population = mate_population(mating_pool, population, rng);
	}

	if (iter >= max_iter) {
		find_best_match(population);
	}
}

#else

bool check_digit(const std::vector<int> &n)
{
	/* the fifth number should always be 5 */
	if (n[4] != 5) {
		return false;
	}

	/* the second number should always be a multiple of 2 */
	if (n[1] % 2 != 0) {
		return false;
	}

	if ((n[0] + n[1] + n[2]) % 3 != 0) {
		return false;
	}

	if ((2 * n[2] + n[3]) % 4 != 0) {
		return false;
	}

//	auto first_six = 4 * (n[0] + n[1] + n[2] + n[3] + n[4]) + n[5];
	auto first_six = madd_iter(n.begin(), 4, 4, 4, 4, 4, 1);
	if (first_six % 6 != 0) {
		return false;
	}

//	auto first_eight = 4 * n[5] + 2 * n[6] + n[7];
	auto first_eight = madd_iter(n.begin() + 5, 4, 2, 1);
	if (first_eight % 8 != 0) {
		return false;
	}

//	auto first_seven = n[0] + 5 * n[1] + 4 * n[2] + 6 * n[3] + 2 * n[4] + 3 * n[5] + n[6];
	auto first_seven = madd_iter(n.begin(), 1, 5, 4, 6, 2, 3, 1);
	if (first_seven % 7 != 0) {
		return false;
	}

	return true;
}

int main()
{
	Timer(__func__);

	std::vector<int> digits(9);
	std::iota(digits.begin(), digits.end(), 1);
	std::mt19937 rng(19937);

	auto count = 0;

	std::cout << "Expected result: 381654729\n";
	std::cout << "Computed result: ";

	while (!check_digit(digits)) {
		std::shuffle(digits.begin(), digits.end(), rng);
		count++;
	}

	for (const auto &d : digits) {
		std::cout << d;
	}

	std::cout << "\nIterations: " << count << "\n";
}

#endif
