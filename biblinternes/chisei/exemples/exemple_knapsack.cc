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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

 /**
  * Un algorithme génétique pour le problème de Knapsack 0-1.
  * Étude de cas : 1000 items, taille Knapsack de 50.
  */

#include "../genetique.h"

#include <cassert>
#include <cstdio>

/* ************************************************************************** */

struct DonneesChromosome {
	std::vector<int> items{};
	int aptitude = 0;

	DonneesChromosome() = default;
};

struct DonneesKnapsack {
	std::vector<int> valeurs{};
	std::vector<int> poids{};

	int limit = 0;
	size_t nombre_poids = 0;

	DonneesKnapsack() = default;
};

struct ProblemeKnapsack {
	using type_chromosome = DonneesChromosome;
	using type_donnees = DonneesKnapsack;

	static constexpr auto GENERATIONS_MAX = 50000;
	static constexpr auto ITERATIONS_ETAPE = 5000;
	static constexpr auto TAILLE_POPULATION = 250;
	static constexpr auto TAILLE_ELITE = static_cast<int>(0.2 * 250);
	static constexpr auto PROB_CROISEMENT = 0.35;

	static std::vector<type_chromosome> cree_population(const type_donnees &donnees, std::mt19937 &/*rng*/)
	{
		/* Initialise les chromosome avec le résultat d'un algorithme glouton. */
		std::vector<std::pair<size_t, double>> rvals(donnees.nombre_poids);

		for (size_t i = 0; i < donnees.nombre_poids; ++i) {
			rvals.push_back(std::make_pair(i, static_cast<double>(donnees.valeurs[i]) / donnees.poids[i]));
		}

		std::sort(rvals.begin(), rvals.end(), [](const auto &a, const auto &b)
		{
			return a.second > b.second;
		});

		std::vector<int> index(donnees.nombre_poids, 0);
		auto poids_courant = 0;

		for (const auto &valeur : rvals) {
			const auto k = valeur.first;

			/* Remplissage glouton. */
			if (poids_courant + donnees.poids[k] <= donnees.limit) {
				poids_courant += donnees.poids[k];
				index[k] = 1;
			}
		}

		std::vector<type_chromosome> population;
		population.reserve(TAILLE_POPULATION);
		type_chromosome chromosome;

		for (int i = 0; i < TAILLE_POPULATION; ++i) {
			chromosome.items.resize(donnees.nombre_poids);

			for (size_t j = 0; j < donnees.nombre_poids; ++j) {
				chromosome.items[j] = index[i];
			}

			calcule_aptitude(donnees, chromosome);
			population.push_back(chromosome);
		}

		return population;
	}

	static type_chromosome cree_chromosome(const type_donnees &/*donnees*/, std::mt19937 &/*rng*/)
	{
		return type_chromosome();
	}

	static void calcule_aptitude(const type_donnees &donnees, type_chromosome &chromosome)
	{
		auto apt = 0;
		auto somme_poids = 0;

		for (size_t i = 0, e = chromosome.items.size(); i < e; ++i) {
			somme_poids += chromosome.items[i] * donnees.poids[i];
			apt += chromosome.items[i] * donnees.valeurs[i];
		}

		if (somme_poids > donnees.limit) {
			/* Pénalité pour des solutions invalides. */
			apt -= 7 * (somme_poids - donnees.limit);
		}

		chromosome.aptitude = apt;
	}

	static type_chromosome croise(const type_donnees &donnees, const type_chromosome &chromosome1, const type_chromosome &chromosome2, std::mt19937 &rng)
	{
		std::uniform_real_distribution<double> coin_flip(0.0, 1.0);

		type_chromosome enfant;
		enfant.items.resize(donnees.nombre_poids);
		enfant.aptitude = 0;

		numero7::chisei::croise_si(
					chromosome1.items,
					chromosome2.items,
					enfant.items,
					[&]{ return (coin_flip(rng) < 0.5); });

		return enfant;
	}

	static void mute(type_chromosome &chromosome, std::mt19937 &rng)
	{
		std::uniform_int_distribution<int> dist(0, chromosome.items.size() - 1);

		assert(chromosome.items.size() != 0);

		const auto xi = dist(rng);
		chromosome.items[xi] = !chromosome.items[xi];
	}

	static int aptitude(const type_chromosome &chromosome)
	{
		return chromosome.aptitude;
	}

	static bool compare_aptitude(const type_chromosome &a, const type_chromosome &b)
	{
		return a.aptitude > b.aptitude;
	}

	static bool meilleur_trouve(const type_chromosome &chromosome)
	{
		return chromosome.aptitude == 675;
	}

	static void rappel_pour_meilleur(const type_donnees &/*donnees*/, const type_chromosome &/*chromosome*/)
	{
	}
};

/* ************************************************************************** */

int main()
{
	std::ostream &os = std::cout;

	DonneesKnapsack donnees;
	donnees.limit = 50;

	FILE *f1 = std::fopen("1000_poids.txt", "r");
	FILE *f2 = std::fopen("1000_valeurs.txt", "r");
	auto info = 0;

	while (!std::feof(f1) || !std::feof(f2)) {
		auto dummy = std::fscanf(f1, "%d ", &info);
		assert(dummy != 0);
		donnees.poids.push_back(info);

		info = 0;

		dummy = std::fscanf(f2, "%d ", &info);
		assert(dummy != 0);
		donnees.valeurs.push_back(info);
	}

	std::fclose(f1);
	std::fclose(f2);

	donnees.nombre_poids = donnees.poids.size();

	auto [c, a] = numero7::chisei::lance_algorithme_genetique<ProblemeKnapsack>(os, donnees);

	DonneesChromosome chromosome = c;
	numero7::chisei::DonneesAlgorithme algorithme = a;

	os << "Meilleur résultat : " << chromosome.aptitude << '\n';
	os << "Générations : " << algorithme.generations << '\n';
}
