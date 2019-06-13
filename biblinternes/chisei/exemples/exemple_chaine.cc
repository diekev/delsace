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
  * Un algorithme génétique exemple pour deviner une chaine de caractère
  * aléatoire.
  */

#include "../genetique.h"

/* ************************************************************************** */

struct DonneeMembre {
	std::string adn{""};
	double aptitude{0.0};

	DonneeMembre() = default;
};

struct DonneesChaine {
	std::string cible{""};
	size_t taille{0ul};
	double taille_inverse{0.0};

	DonneesChaine() = default;
};

struct TraitPopulation {
	using type_chromosome = DonneeMembre;
	using type_donnees = DonneesChaine;

	static constexpr auto GENERATIONS_MAX = 50000;
	static constexpr auto ITERATIONS_ETAPE = 5000;
	static constexpr auto TAILLE_POPULATION = 150;
	static constexpr auto TAILLE_ELITE = static_cast<int>(0.2 * 150);
	static constexpr auto PROB_CROISEMENT = 0.95;

	static std::vector<type_chromosome> cree_population(const type_donnees &donnees, std::mt19937 &rng)
	{
		std::vector<type_chromosome> population;
		population.reserve(TAILLE_POPULATION);

		for (int i = 0; i < TAILLE_POPULATION; ++i) {
			population.push_back(cree_chromosome(donnees, rng));
		}

		return population;
	}

	static type_chromosome cree_chromosome(const type_donnees &donnees, std::mt19937 &rng)
	{
		DonneeMembre donnee;
		donnee.adn = numero7::chisei::chaine_aleatoire(rng, donnees.taille);
		donnee.aptitude = 0.0f;

		calcule_aptitude(donnees, donnee);

		return donnee;
	}

	static void calcule_aptitude(const type_donnees &donnees, type_chromosome &chromosome)
	{
		const auto fit = numero7::chisei::compte_correspondances(chromosome.adn, donnees.cible);
		chromosome.aptitude = static_cast<double>(fit * fit) * donnees.taille_inverse;
	}

	static type_chromosome croise(const type_donnees &donnees, const type_chromosome &chromosome1, const type_chromosome &chromosome2, std::mt19937 &rng)
	{
		std::uniform_real_distribution<double> coin_flip(0.0, 1.0);

		type_chromosome enfant;
		enfant.adn.resize(donnees.taille);
		enfant.aptitude = 0.0f;

		numero7::chisei::croise_si(
					chromosome1.adn,
					chromosome2.adn, enfant.adn,
					[&]{ return (coin_flip(rng) < 0.5); });

		return enfant;
	}

	static void mute(type_chromosome &chromosome, std::mt19937 &rng)
	{
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		std::uniform_int_distribution<char> lettre_aleatoire(-128, 127);

		std::transform(chromosome.adn.begin(), chromosome.adn.end(), chromosome.adn.begin(),
					   [&](char c)
		{
			if (dist(rng) < 0.01) {
				return static_cast<char>(lettre_aleatoire(rng));
			}

			return c;
		});
	}

	static double aptitude(const type_chromosome &chromosome)
	{
		return chromosome.aptitude;
	}

	static bool compare_aptitude(const type_chromosome &a, const type_chromosome &b)
	{
		return a.aptitude > b.aptitude;
	}

	static bool meilleur_trouve(const type_chromosome &chromosome)
	{
		return chromosome.aptitude == 1.0;
	}

	static void rappel_pour_meilleur(const type_donnees &/*donnees*/, const type_chromosome &/*chromosome*/)
	{
	}
};

/* ************************************************************************** */

int main()
{
	std::ostream &os = std::cout;

	DonneesChaine donnees;
	donnees.cible = "mlkjdf qsmdikjfqskd jmlskdfj eijuqkdqs sdkjamlekmrqo xiçbhjb qlm re";
	donnees.taille = donnees.cible.size();
	donnees.taille_inverse = 1.0 / (donnees.taille * donnees.taille);

	auto [c, a] = numero7::chisei::lance_algorithme_genetique<TraitPopulation>(os, donnees);

	DonneeMembre chromosome = c;
	numero7::chisei::DonneesAlgorithme algorithme = a;

	os << "Meilleur résultat : " << chromosome.adn << " (" << chromosome.aptitude << ")\n";
	os << "Générations : " << algorithme.generations << '\n';
}
