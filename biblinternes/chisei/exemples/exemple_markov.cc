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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

using type_matrice = dls::tableau<dls::tableau<double>>;

static void imprime_matrice(
		dls::chaine const &texte,
		type_matrice const &matrice,
		dls::dico_desordonne<long, char> &index_arriere)
{
	auto courante = 0;

	std::cerr << texte;

	for (auto const &vec : matrice) {
		std::cerr << index_arriere[courante] << " :";
		auto virgule = ' ';

		for (auto const &v : vec) {
			std::cerr << virgule << v;
			virgule = ',';
		}

		std::cerr << '\n';
		courante += 1;
	}
}

int main()
{
	// que faire pour les accents ?
	dls::chaine mots[] = {
		dls::chaine("apopathodiaphulatophobe"),
		dls::chaine("anticonstitutionnellement"),
		dls::chaine("rencontre"),
		dls::chaine("plantes"),
		dls::chaine("interdites"),
		dls::chaine("presidence"),
		dls::chaine("cannabis"),
		dls::chaine("roi"),
		dls::chaine("coissant"),
		dls::chaine("pain"),
		dls::chaine("au"),
		dls::chaine("chocolat"),
		dls::chaine("raisin"),
		dls::chaine("chocolatine"),
		dls::chaine("tresse"),
		dls::chaine("tante"),
		dls::chaine("dithyrambique"),
	};

	// construction de l'index
	dls::ensemble<char> lettres;

	for (auto const &mot : mots) {
		for (auto l : mot) {
			lettres.insere(l);
		}
	}

	dls::dico_desordonne<char, long> index_avant;
	dls::dico_desordonne<long, char> index_arriere;
	long courante = 0;

	for (auto l : lettres) {
		index_avant.insere({l, courante});
		index_arriere.insere({courante, l});
		courante += 1;
	}

	// construction de la matrice
	dls::tableau<dls::tableau<double>> matrice;
	matrice.redimensionne(lettres.taille());

	for (auto &vec : matrice) {
		vec = dls::tableau<double>(lettres.taille());
	}

	for (auto const &mot : mots) {
		for (auto i = 0; i < mot.taille() - 1; ++i) {
			auto idx0 = index_avant[mot[i]];
			auto idx1 = index_avant[mot[i + 1]];

			matrice[idx0][idx1] += 1.0;
		}
	}

//	imprime_matrice("Matrice = \n", matrice, index_arriere);

	// converti en fonction de distribution cumulative
	for (auto &vec : matrice) {
		auto total = 0.0;

		for (auto const &v : vec) {
			total += v;
		}

		// que faire si une lettre n'est suivit par aucune ? on s'arrête ?
		if (total == 0.0) {
			continue;
		}

		for (auto &v : vec) {
			v /= total;
		}

		auto accum = 0.0;

		for (auto &v : vec) {
			accum += v;
			v = accum;
		}
	}

//	imprime_matrice("Distribution cumulée = \n", matrice, index_arriere);

	std::cerr << "Génère mots :\n";
	auto gna = GNA();

	for (auto r = 0; r < 5; ++r) {
		auto lettre_depart = index_arriere[gna.uniforme(0l, lettres.taille() - 1)];
		auto lettre_courante = lettre_depart;

		auto res = dls::chaine();
		res += lettre_depart;

		auto nombre_lettres = 11; //gna.uniforme(0l, 19l);

		for (auto i = 0; i < nombre_lettres; ++i) {
			// prend le vecteur de la lettre_courante
			auto const &vec = matrice[index_avant[lettre_courante]];

			// génère une lettre
			auto prob = gna.uniforme(0.0, 1.0);

			for (auto j = 0; j < lettres.taille(); ++j) {
				if (prob <= vec[j]) {
					lettre_courante = index_arriere[j];
					break;
				}
			}

			res += lettre_courante;
		}

		std::cerr << res << '\n';
	}

	return 0;
}
