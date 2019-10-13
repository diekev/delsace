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

#include "biblinternes/langage/unicode.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

using type_matrice = dls::tableau<dls::tableau<double>>;

template <typename T>
void imprime_matrice(
		dls::chaine const &texte,
		type_matrice const &matrice,
		dls::dico_desordonne<long, T> &index_arriere)
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

// converti les lignes de la matrice en fonction de distribution cumulative
static void converti_fonction_repartition(type_matrice &matrice)
{
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
}

void test_markov_lettres()
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

	converti_fonction_repartition(matrice);

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
}

static auto MOT_VIDE = dls::vue_chaine("");

static auto morcelle(dls::chaine const &texte)
{
	dls::tableau<dls::vue_chaine> morceaux;

	if (texte.est_vide()) {
		return morceaux;
	}

	auto ptr = &texte[0];
	auto taille_mot = 0;

	for (auto i = 0; i < texte.taille(); ++i) {
		if (texte[i] == ' ') {
			if (taille_mot != 0) {
				morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
			}

			taille_mot = 0;
			ptr = &texte[i];
			continue;
		}

		if (texte[i] == '\n') {
			if (taille_mot != 0) {
				morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
			}

			taille_mot = 0;
			ptr = &texte[i];
			continue;
		}

		if (dls::outils::est_element(texte[i], '.', '\'', ',')) {
			if (taille_mot != 0) {
				morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
			}

			taille_mot = 0;
			ptr = &texte[i];

			morceaux.pousse(dls::vue_chaine(ptr, 1));

			continue;
		}

		if (taille_mot == 0) {
			ptr = &texte[i];
		}

		++taille_mot;
	}

	if (taille_mot != 0) {
		morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
	}

	return morceaux;
}

static auto en_minuscule(dls::chaine const &texte)
{
	auto res = dls::chaine();
	res.reserve(texte.taille());

	for (auto i = 0; i < texte.taille();) {
		auto n = lng::nombre_octets(&texte[i]);

		if (n > 1) {
			for (auto j = 0; j < n; ++j) {
				res += texte[i + j];
			}

			i += n;
		}
		else {
			res += static_cast<char>(tolower(texte[i]));
			++i;
		}
	}

	return res;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "Utilisation : exemple_markov FICHIER\n";
		return 1;
	}

	//auto texte = dls::chaine("Le roi est mort. La reine est morte.");
	auto texte = dls::contenu_fichier(argv[1]);
	texte = en_minuscule(texte);

	auto morceaux = morcelle(texte);

	std::cerr << "Il y a " << morceaux.taille() << " morceaux dans le texte.\n";

	/* construction de l'index */
	auto mots = dls::ensemble<dls::vue_chaine>();

	for (auto morceau : morceaux) {
		mots.insere(morceau);
	}

	std::cerr << "Il y a " << mots.taille() << " mots dans le texte.\n";

	mots.insere(MOT_VIDE);

	dls::dico_desordonne<dls::vue_chaine, long> index_avant;
	dls::dico_desordonne<long, dls::vue_chaine> index_arriere;
	long courante = 0;

	for (auto mot : mots) {
		index_avant.insere({mot, courante});
		index_arriere.insere({courante, mot});
		courante += 1;
	}

	/* construction de la matrice */
	dls::tableau<dls::tableau<double>> matrice;
	matrice.redimensionne(mots.taille());

	for (auto &vec : matrice) {
		vec = dls::tableau<double>(mots.taille());
	}

	for (auto i = 0; i < morceaux.taille() - 1; ++i) {
		auto idx0 = index_avant[morceaux[i]];
		auto idx1 = index_avant[morceaux[i + 1]];

		matrice[idx0][idx1] += 1.0;
	}

	/* conditions de bordures : il y a un mot vide avant et après le texte */
	auto idx0 = index_avant[MOT_VIDE];
	auto idx1 = index_avant[morceaux[0]];

	matrice[idx0][idx1] += 1.0;

	idx0 = index_avant[morceaux[morceaux.taille() - 1]];
	idx1 = index_avant[MOT_VIDE];

	matrice[idx0][idx1] += 1.0;

	converti_fonction_repartition(matrice);

	//imprime_matrice("Matrice = \n", matrice, index_arriere);

	std::cerr << "Génère texte :\n";
	auto gna = GNA();

	auto mot_depart = MOT_VIDE;
	auto mot_courant = mot_depart;

	while (true) {
		/* prend le vecteur du mot_courant */
		auto const &vec = matrice[index_avant[mot_courant]];

		/* génère un mot */
		auto prob = gna.uniforme(0.0, 1.0);

		for (auto j = 0; j < mots.taille(); ++j) {
			if (prob <= vec[j]) {
				mot_courant = index_arriere[j];
				break;
			}
		}

		std::cerr << mot_courant;

		if (mot_courant == MOT_VIDE) {
			std::cerr << '\n';
			break;
		}

		std::cerr << ' ';
	}

	std::cerr << "Mémoire consommée : " << memoire::formate_taille(memoire::consommee()) << '\n';

	return 0;
}
