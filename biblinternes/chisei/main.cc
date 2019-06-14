/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "learner.h"
#include "includes.h"

#include <iostream>
#include <random>

#include "../../biblexternes/docopt/docopt.hh"

#include "../math/statistique.hh"

#define SERIAL

#ifndef SERIAL
#	include <atomic>
#	include <tbb/parallel_for.h>
#	include <tbb/parallel_reduce.h>
#endif

static const char usage[] = R"(
Artificial Intelligence research and development.

Usage:
    ai --rnd_word
    ai --wage_gap
    ai --learner
    ai --neural_net
    ai (-h | --help)
    ai --version

Options:
    -h, --help      Show this screen.
    --version       Show version.
    --learner       Test Learner.
    --rnd_word      Generate random words.
    --wage_gap      Test Wage Gap Theory.
    --neural_net    Test Neural Network.
)";

struct LetterData {
	unsigned char letter;
	size_t frequency;
	double probabilite;
};

static LetterData letters_data[] = {
	{ 'a', 711, 0.0 },
	{ 'b', 114, 0.0 },
	{ 'c', 318, 0.0 },
	{ 'd', 367, 0.0 },
	{ 'e', 1210, 0.0 },
	{ 'f', 111, 0.0 },
	{ 'g', 123, 0.0 },
	{ 'h', 111, 0.0 },
	{ 'i', 659, 0.0 },
	{ 'j', 34, 0.0 },
	{ 'k', 29, 0.0 },
	{ 'l', 496, 0.0 },
	{ 'm', 262, 0.0 },
	{ 'n', 639, 0.0 },
	{ 'o', 502, 0.0 },
	{ 'p', 249, 0.0 },
	{ 'q', 65, 0.0 },
	{ 'r', 607, 0.0 },
	{ 's', 651, 0.0 },
	{ 't', 592, 0.0 },
	{ 'u', 449, 0.0 },
	{ 'v', 111, 0.0 },
	{ 'w', 17, 0.0 },
	{ 'x', 38, 0.0 },
	{ 'y', 46, 0.0 },
	{ 'z', 15, 0.0 },
};

static void test_frequency(std::ostream &os)
{
	auto max = 0ul;

	for (const LetterData &data : letters_data) {
		max = std::max(data.frequency, max);
	}

	for (LetterData &data : letters_data) {
		data.probabilite = static_cast<double>(data.frequency) / static_cast<double>(max);
	}

	std::sort(std::begin(letters_data), std::end(letters_data),
			  [](const auto &a, const auto &b)

	{
		return a.frequency < b.frequency;
	});

	std::uniform_real_distribution<double> random_letter(0.0, 1.0);
	std::mt19937 rng(3541567981);

	for (int j = 0; j < 100; ++j) {
		for (int i = 0; i < 9; ++i) {
			const auto probabilite = random_letter(rng);

			auto fini = false;

			while (!fini) {
				for (const LetterData &data : letters_data) {
					if (probabilite <= data.probabilite) {
						os << data.letter;
						fini = true;
						break;
					}
				}
			}
		}

		os << '\n';
	}
}

struct RaceData {
	const unsigned char index;
	const char *name;
	const int population_per_thousand;
	double avg_income;
	double total_income;
	size_t number_of_income;
	double probabilite;
};

static RaceData us_races[] = {
	{ 0, "Hawaian",    2, 0.0, 0.0, 0, 0.0 },
	{ 1, "Native",     9, 0.0, 0.0, 0, 0.0 },
	{ 2, "Asian",     48, 0.0, 0.0, 0, 0.0 },
	{ 3, "Other",     91, 0.0, 0.0, 0, 0.0 },
	{ 4, "Black",    126, 0.0, 0.0, 0, 0.0 },
	{ 5, "White",    724, 0.0, 0.0, 0, 0.0 },
};

#ifndef SERIAL
static std::atomic<int> thread_index(0);
static std::vector<unsigned char> indices(1000);

struct ThreadRaceData {
	std::vector<RaceData> races;

	ThreadRaceData()
	    : races(std::begin(us_races), std::end(us_races))
	{}

	ThreadRaceData(ThreadRaceData &, tbb::split)
	    : races(std::begin(us_races), std::end(us_races))
	{}

	void operator()(const tbb::blocked_range<int> &r)
	{
		std::uniform_real_distribution<double> random_income(1000.0, 100000.0);
		std::mt19937 income_rng(35415679 + thread_index);

		std::uniform_int_distribution<int> random_index(0, 999);
		std::mt19937 index_rng(3541567981 + thread_index);

		thread_index.fetch_add(1, std::memory_order_relaxed);

		for (auto i = r.begin(); i < r.end(); ++i) {
			const auto index = random_index(index_rng);
			const auto income = random_income(income_rng);

			RaceData &race = races[indices[index]];
			race.total_income += income;
			race.number_of_income += 1;
		}
	}

	void join(ThreadRaceData &t)
	{
		for (RaceData &race : races) {
			auto other = t.races[race.index];
			race.total_income += other.total_income;
			race.number_of_income += other.number_of_income;
		}
	}
};
#endif

static void test_wage_gap(std::ostream &os)
{
	auto max = 0;

	for (const RaceData &race : us_races) {
		max = std::max(race.population_per_thousand, max);
	}

	for (RaceData &race : us_races) {
		race.probabilite = race.population_per_thousand / static_cast<double>(max);
	}

#ifdef SERIAL
	std::uniform_real_distribution<double> random_income(1000.0, 100000.0);
	std::mt19937 income_rng(35415679);

	std::uniform_real_distribution<double> random_index(0.0, 1.0);
	std::mt19937 index_rng(3541567981);

	for (int j = 0; j < 300000000; ++j) {
		const auto probabilite = random_index(index_rng);
		const auto income = random_income(income_rng);

		auto fini = false;

		while (!fini) {
			for (auto &race : us_races) {
				if (probabilite <= race.probabilite) {
					race.total_income += income;
					race.number_of_income += 1;
					fini = true;
					break;
				}
			}
		}
	}
#else
	ThreadRaceData race_data;
	tbb::parallel_reduce(tbb::blocked_range<int>(0, 300000000), race_data);

	for (const RaceData &race : race_data.races) {
		auto &other = us_races[race.index];
		other.total_income += race.total_income;
		other.number_of_income += race.number_of_income;
	}
#endif

	std::vector<double> revenus_moyens;

	for (RaceData &race : us_races) {
		race.avg_income = race.total_income / static_cast<double>(race.number_of_income);
		revenus_moyens.push_back(race.avg_income);
		os << race.name << ", average income: " << race.avg_income << '\n';
	}

	const auto moyenne = dls::math::moyenne<double>(revenus_moyens.begin(), revenus_moyens.end());
	const auto ecart_type = dls::math::ecart_type<double>(revenus_moyens.begin(), revenus_moyens.end(), moyenne);

	os << "Moyenne : " << moyenne << '\n';
	os << "Écart type : " << ecart_type << '\n';
	os << "Opportunité : " << (1.0 - ecart_type / moyenne) << '\n';
}

int main(int argc, char **argv)
{
	auto args = dls::docopt::docopt(usage, { argv + 1, argv + argc }, true, "AI 0.1");

	const auto &do_learner = dls::docopt::get_bool(args, "--learner");
	const auto &do_rnd_word = dls::docopt::get_bool(args, "--rnd_word");
	const auto &do_wage_gap = dls::docopt::get_bool(args, "--wage_gap");
	const auto &do_neural_net = dls::docopt::get_bool(args, "--neural_net");

	if (do_learner) {
		std::ostream &os = std::cout;
		learner ai;

		while (true) {
			os << "\nYOU: ";

			std::string phrase;
			std::getline(std::cin, phrase);

			os << "\nCOMPUTER: ";
			ai.respond(phrase);
		}
	}
	else if (do_rnd_word) {
		test_frequency(std::cout);
	}
	else if (do_wage_gap) {
		test_wage_gap(std::cout);
	}
	else if (do_neural_net) {
		test_neural_net(std::cout);
	}
}
