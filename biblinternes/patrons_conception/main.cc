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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "factory.h"
#include "oberver.h"
#include "singleton.h"

#include <memory>
#include <thread>

namespace observer {

void foo(int i)
{
	std::cout << "foo(" << i << ")\n";
}

void bar(int i)
{
	std::cout << "bar(" << i << ")\n";
}

void test()
{
	Barometre barometre;
	Thermometre thermometre;

	MeteoFrance station;

	thermometre.add(&station);

	thermometre.change(31);
	barometre.change(975);

	thermometre.remove(&station);

	thermometre.change(45);

	Subject<event> s;
	s.add(event::blue, std::bind(foo, 42));
	s.add(event::orange, std::bind(foo, 13248));
	s.add(event::green, std::bind(bar, 924));

	s.notify(event::blue);
	s.notify(event::orange);
	s.notify(event::green);
}

}  /* namespace observer */

namespace singleton {

void foo()
{
	//auto sin = Singleton<SoundManager, 0>::instance();
	std::cout << "Thread " << std::this_thread::get_id() << ": " << '\n';
}

void test()
{
	auto count = std::thread::hardware_concurrency();
	dls::tableau<std::thread> threads;

	for (auto i = 0u; i < count; ++i) {
		threads.emplace_back(foo);
	}

	for (auto i = 0u; i < count; ++i) {
		threads[i].join();
	}
}

}  /* namespace singleton */

#include <random>

/**
 * Sélectionne un nombre aléatoire d'objet depuis une liste quand on ne peut
 * savoir combien d'objets il y a la liste (par exemple retour de grep).
 *
 * Selon un nombre à sélectionner k, et l'élément courant n, choisi l'élément
 * avec une probabilité k/n.
 */
void echantillonage_reserve(std::ostream &os)
{
	std::mt19937 gen(134971);
	std::uniform_int_distribution<long> dist(64, 1024);

	dls::tableau<int> donnees;
	donnees.redimensionne(dist(gen));

	std::iota(donnees.debut(), donnees.fin(), 0);

	os << "Il y a " << donnees.taille() << " nombres en tout.\n";

	std::uniform_real_distribution<double> dist2(0.0, 1.0);
	auto nombre_donnees_vues = 0;

	dls::tableau<int> selections;
	selections.reserve(32);

	for (const auto donnee : donnees) {
		++nombre_donnees_vues;

		const auto probabilite_selection = 32.0 / nombre_donnees_vues;

		if (probabilite_selection < dist2(gen)) {
			if (selections.taille() == 32) {
				for (auto &selection : selections) {
					if (probabilite_selection < dist2(gen)) {
						selection = donnee;
						break;
					}
				}
			}
			else {
				selections.pousse(donnee);
			}
		}
	}

	os << "Les numéros sélectionnés sont : \n";

	for (const auto &selection : selections) {
		os << selection << '\n';
	}
}

/* Le nombre d'alvéole est de 2^3 */
static constexpr auto LOG_NOMBRE_ALVEOLES = 3;
static constexpr auto NOMBRE_ALVEOLES = 1 << LOG_NOMBRE_ALVEOLES;

int hash(int i)
{
	return i;
}

int partition(int i)
{
	auto hi = hash(i);
	return hi & LOG_NOMBRE_ALVEOLES;
}

static const uint8_t clz_table_4bit[16] = { 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
int clz( unsigned int x )
{
  int n;
  if ((x & 0xFFFF0000) == 0) {n  = 16; x <<= 16;} else {n = 0;}
  if ((x & 0xFF000000) == 0) {n +=  8; x <<=  8;}
  if ((x & 0xF0000000) == 0) {n +=  4; x <<=  4;}
  n += static_cast<int>(clz_table_4bit[x >> (32 - 4)]);
  return n;
}

/**
 * Compte le nombre d'éléments dans la base de données, quand il n'y a pas assez
 * d'espace pour compter tous les éléments. Ceci est une estimation.
 *
 * Première observation : la probabilité qu'une chaine d'octet aléatoire a k
 * octet de tête avec une valeur de 0 est de 1/(2^k).
 *
 * 00000100 11000011 10010100 01100111
 * 5 zéro au début : p = 1/(2^5) = 1/32.
 *
 *  Pour 128 nombre aléatoire, on aura :
 * - 64 avec 1 zéro
 * - 32 avec 2 zéros
 * - 16 avec 3 zéros
 * - 8 avec 4 zéros
 * - 4 avec 5 zéros
 * - 2 avec 6 zéros
 * - 1 avec 7 zéros
 *
 * Pour avoir un nombre aléatoire depuis une donnée d'entrée, il suffit de
 * calculer son empreinte. HyperLogLog étant une estimation, la collision
 * d'empreinte est un non-problème.
 *
 * Pour éviter les duplications, on considère le maximum de zéro en entête. Le
 * nombre estimé d'éléments sera de 2^max.
 *
 * Pour éviter les erreurs dues aux extrêmes, on calcule la fonction plusieurs
 * fois avec différentes fonctions de hachage.
 */
void hyperloglog(std::ostream &os)
{
	std::mt19937 gen(134971);
	std::uniform_int_distribution<unsigned int> dist(0, std::numeric_limits<unsigned int>::max());

	int max[NOMBRE_ALVEOLES] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	for (unsigned int j = 0; j < 10000; ++j) {
		auto v = dist(gen);

		/* Calcule la partition de l'entrée. */
		auto i = partition(static_cast<int>(v));

		/* Compte le nombre de zéro en entête. */
		auto z = clz(static_cast<unsigned int>(hash(static_cast<int>(v)))); // hash[i](v)

		if (z > 32) {
			os << "Il y a plus de 32 zéro en entête !!\n";
			return;
		}

		/* Mis-à-jour du maximum. */
		max[i] = std::max(max[i], z);
	}

	int max2[NOMBRE_ALVEOLES] = {11, 11, 13, 12, 9, 11, 10, 15};

	/* À FAIRE : Il faut prendre la moyenne harmonique. */
	auto resultat = 0.0;
	for (int i = 0; i < NOMBRE_ALVEOLES; ++i) {
		os << "Alvéole " << i << " : " << max2[i] << '\n';
		resultat += 1.0 / std::pow(2.0, max[i] + 1);
	}

	resultat = 1.0 / resultat;

	resultat *= NOMBRE_ALVEOLES * NOMBRE_ALVEOLES * 0.72134;

	os << "Il y a à peu près : " << resultat << " nombre différents.\n";
}

int main()
{
	hyperloglog(std::cerr);
}
