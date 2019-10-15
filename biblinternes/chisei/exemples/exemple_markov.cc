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

#include "biblinternes/chrono/chronometre_de_portee.hh"
#include "biblinternes/langage/unicode.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

/**
 * PROBLÈMES :
 * - les guillemets ne sont pas ouverts ou fermés
 * - il arrive que des phrases ne contiennent que « m ».
 */

using type_matrice = dls::tableau<dls::tableau<double>>;

static auto MOT_VIDE = dls::vue_chaine("");

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
	CHRONOMETRE_PORTEE(__func__, std::cerr);

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

static auto filtres_mots(dls::tableau<dls::vue_chaine> const &morceaux)
{
	dls::ensemble<dls::vue_chaine> mots_filtres;

	for (auto const &mot : morceaux) {
		if (dls::outils::est_element(mot, ".", ",", "-", "?", "!", ";", ":", "'", "(", ")", "«", "»", "’", "{", "}", "…", "[", "]", "/", "–")) {
			continue;
		}

		mots_filtres.insere(mot);
	}

	return mots_filtres;
}

void test_markov_lettres(dls::tableau<dls::vue_chaine> const &morceaux)
{
	auto mots = filtres_mots(morceaux);

	std::cerr << "Il y a " << mots.taille() << " mots dans le texte.\n";

	// construction de l'index
	dls::ensemble<dls::vue_chaine> lettres;

	for (auto const &mot : mots) {
		for (auto i = 0; i < mot.taille();) {
			auto n = lng::nombre_octets(&mot[i]);

			lettres.insere(dls::vue_chaine(&mot[i], n));

			i += n;
		}
	}

	lettres.insere(MOT_VIDE);

	dls::dico_desordonne<dls::vue_chaine, long> index_avant;
	dls::dico_desordonne<long, dls::vue_chaine> index_arriere;
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
		auto n0 = 0;
		auto idx0 = index_avant[MOT_VIDE];
		auto n1 = lng::nombre_octets(&mot[0]);
		auto idx1 = index_avant[dls::vue_chaine(&mot[0], n1)];

		matrice[idx0][idx1] += 1.0;

		for (auto i = n1; i < mot.taille() - 1;) {
			n0 = lng::nombre_octets(&mot[i]);
			idx0 = index_avant[dls::vue_chaine(&mot[i], n0)];
			n1 = lng::nombre_octets(&mot[i + n0]);
			idx1 = index_avant[dls::vue_chaine(&mot[i + n0], n1)];

			matrice[idx0][idx1] += 1.0;
			i += n0 + n1;
		}

		idx0 = index_avant[MOT_VIDE];
		matrice[idx1][idx0] += 1.0;
	}

//	imprime_matrice("Matrice = \n", matrice, index_arriere);

	converti_fonction_repartition(matrice);

//	imprime_matrice("Distribution cumulée = \n", matrice, index_arriere);

	std::cerr << "Génère mots :\n";
	auto gna = GNA();

	for (auto r = 0; r < 10; ++r) {
		auto lettre_courante = MOT_VIDE;

		while (true) {
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

			if (lettre_courante == MOT_VIDE) {
				break;
			}

			std::cerr << lettre_courante;
		}

		std::cerr << '\n';
	}
}

static auto epelle_mot(dls::vue_chaine const &mot)
{
	dls::tableau<dls::vue_chaine> lettres_mot;
	lettres_mot.pousse(MOT_VIDE);
	lettres_mot.pousse(MOT_VIDE);

	for (auto i = 0; i < mot.taille();) {
		auto n = lng::nombre_octets(&mot[i]);
		lettres_mot.pousse(dls::vue_chaine(&mot[i], n));

		i += n;
	}

	lettres_mot.pousse(MOT_VIDE);

	return lettres_mot;
}

void test_markov_lettres_double(dls::tableau<dls::vue_chaine> const &morceaux)
{
	auto mots = filtres_mots(morceaux);

	std::cerr << "Il y a " << mots.taille() << " mots dans le texte.\n";

	// construction de l'index
	using type_paire_lettres = std::pair<dls::vue_chaine, dls::vue_chaine>;
	dls::ensemble<dls::vue_chaine> lettres;
	dls::ensemble<type_paire_lettres> paires_lettres;

	for (auto const &mot : mots) {
		auto lettres_mot = epelle_mot(mot);

		for (auto i = 0; i < lettres_mot.taille() - 1; ++i) {
			lettres.insere(lettres_mot[i]);
			paires_lettres.insere({ lettres_mot[i], lettres_mot[i + 1] });
		}
	}

	dls::dico_desordonne<dls::vue_chaine, long> index_avant;
	dls::dico_desordonne<long, dls::vue_chaine> index_arriere;
	long courante = 0;

	for (auto l : lettres) {
		index_avant.insere({l, courante});
		index_arriere.insere({courante, l});
		courante += 1;
	}

	dls::dico<type_paire_lettres, long> index_avant_paires;
	dls::dico<long, type_paire_lettres> index_arriere_paires;
	courante = 0;

	for (auto l : paires_lettres) {
		index_avant_paires.insere({l, courante});
		index_arriere_paires.insere({courante, l});
		courante += 1;
	}

	// construction de la matrice
	dls::tableau<dls::tableau<double>> matrice;
	matrice.redimensionne(paires_lettres.taille());

	for (auto &vec : matrice) {
		vec = dls::tableau<double>(lettres.taille());
	}

	for (auto const &mot : mots) {
		auto lettres_mot = epelle_mot(mot);

		for (auto i = 0; i < lettres_mot.taille() - 2; ++i) {
			auto paire = type_paire_lettres(lettres_mot[i], lettres_mot[i + 1]);
			auto lettre = lettres_mot[i + 2];

			auto idx0 = index_avant_paires[paire];
			auto idx1 = index_avant[lettre];

			matrice[idx0][idx1] += 1.0;
		}
	}

//	imprime_matrice("Matrice = \n", matrice, index_arriere);

	converti_fonction_repartition(matrice);

//	imprime_matrice("Distribution cumulée = \n", matrice, index_arriere);

	std::cerr << "Génère mots :\n";
	auto gna = GNA(static_cast<int>(mots.taille()));

	for (auto r = 0; r < 20; ++r) {
		auto l0 = MOT_VIDE;
		auto l1 = MOT_VIDE;

		while (true) {
			// prend le vecteur de la lettre_courante
			auto const &vec = matrice[index_avant_paires[{l0, l1}]];
			auto lettre_courante = dls::vue_chaine();

			// génère une lettre
			auto prob = gna.uniforme(0.0, 1.0);

			for (auto j = 0; j < lettres.taille(); ++j) {
				if (prob <= vec[j]) {
					lettre_courante = index_arriere[j];
					break;
				}
			}

			if (lettre_courante == MOT_VIDE) {
				break;
			}

			std::cerr << lettre_courante;

			l0 = l1;
			l1 = lettre_courante;
		}

		std::cerr << '\n';
	}
}

static auto morcelle(dls::chaine const &texte)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);

	dls::tableau<dls::vue_chaine> morceaux;

	if (texte.est_vide()) {
		return morceaux;
	}

	auto ptr = &texte[0];
	auto taille_mot = 0;

	for (auto i = 0; i < texte.taille();) {
		auto n = lng::nombre_octets(&texte[i]);

		if (n > 1) {
			auto lettre = dls::vue_chaine(&texte[i], n);
			if (dls::outils::est_element(lettre, "«", "»", "’", "…", "–")) {
				if (taille_mot != 0) {
					morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
				}

				morceaux.pousse(lettre);

				taille_mot = 0;
				ptr = &texte[i + n];
			}
			else {
				taille_mot += n;
			}
		}
		else {
			switch (texte[i]) {
				case ' ':
				case '\n':
				{
					if (taille_mot != 0) {
						morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
					}

					taille_mot = 0;
					ptr = &texte[i];
					break;
				}
				case '.':
				case '\'':
				case ',':
				case '-':
				case '(':
				case ')':
				case '{':
				case '}':
				case '?':
				case '!':
				case ':':
				case '[':
				case ']':
				case '/':
				{
					if (taille_mot != 0) {
						morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
					}

					taille_mot = 0;
					ptr = &texte[i];

					morceaux.pousse(dls::vue_chaine(ptr, 1));
					break;
				}
				default:
				{
					if (taille_mot == 0) {
						ptr = &texte[i];
					}

					++taille_mot;
					break;
				}
			}
		}

		i += n;
	}

	if (taille_mot != 0) {
		morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
	}

	return morceaux;
}

static auto dico_minuscule = dls::cree_dico(
			dls::paire(dls::vue_chaine("É"), dls::vue_chaine("é")),
			dls::paire(dls::vue_chaine("È"), dls::vue_chaine("è")),
			dls::paire(dls::vue_chaine("Ê"), dls::vue_chaine("ê")),
			dls::paire(dls::vue_chaine("À"), dls::vue_chaine("à")),
			dls::paire(dls::vue_chaine("Ô"), dls::vue_chaine("ô")),
			dls::paire(dls::vue_chaine("Û"), dls::vue_chaine("î")),
			dls::paire(dls::vue_chaine("Ç"), dls::vue_chaine("ç"))
			);


static auto en_minuscule(dls::chaine const &texte)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);

	auto res = dls::chaine();
	res.reserve(texte.taille());

	for (auto i = 0; i < texte.taille();) {
		auto n = lng::nombre_octets(&texte[i]);

		if (n > 1) {
			auto lettre = dls::vue_chaine(&texte[i], n);

			auto plg_lettre = dico_minuscule.trouve(lettre);

			if (!plg_lettre.est_finie()) {
				lettre = plg_lettre.front().second;
			}

			for (auto j = 0; j < lettre.taille(); ++j) {
				res += lettre[j];
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

void test_markov_mot_simple(dls::tableau<dls::vue_chaine> const &morceaux)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);

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
}

void test_markov_mots_paire(dls::tableau<dls::vue_chaine> const &morceaux)
{
	/* construction de l'index */
	auto mots = dls::ensemble<dls::vue_chaine>();
	auto paire_mots = dls::ensemble<std::pair<dls::vue_chaine, dls::vue_chaine>>();

	for (auto i = 0; i < morceaux.taille() - 1; ++i) {
		mots.insere(morceaux[i]);
		paire_mots.insere({ morceaux[i], morceaux[i + 1] });
	}

	mots.insere(morceaux.back());

	std::cerr << "Il y a " << mots.taille() << " mots dans le texte.\n";
	std::cerr << "Il y a " << paire_mots.taille() << " paires de mots dans le texte.\n";

	paire_mots.insere({ MOT_VIDE, MOT_VIDE });
	paire_mots.insere({ MOT_VIDE, morceaux[0] });
	mots.insere(MOT_VIDE);

	dls::dico_desordonne<dls::vue_chaine, long> index_avant;
	dls::dico_desordonne<long, dls::vue_chaine> index_arriere;
	long courante = 0;

	for (auto mot : mots) {
		index_avant.insere({mot, courante});
		index_arriere.insere({courante, mot});
		courante += 1;
	}

	dls::dico<std::pair<dls::vue_chaine, dls::vue_chaine>, long> index_avant_paires;
	dls::dico<long, std::pair<dls::vue_chaine, dls::vue_chaine>> index_arriere_paires;
	courante = 0;

	for (auto paire : paire_mots) {
		index_avant_paires.insere({paire, courante});
		index_arriere_paires.insere({courante, paire});
		courante += 1;
	}

	/* construction de la matrice */
	dls::tableau<dls::tableau<double>> matrice;
	matrice.redimensionne(paire_mots.taille());

	for (auto &vec : matrice) {
		vec = dls::tableau<double>(mots.taille());
	}

	for (auto i = 0; i < morceaux.taille() - 2; ++i) {
		auto idx0 = index_avant_paires[{ morceaux[i], morceaux[i + 1] }];
		auto idx1 = index_avant[morceaux[i + 2]];

		matrice[idx0][idx1] += 1.0;
	}

	/* conditions de bordures : il y a un mot vide avant et après le texte */
	auto idx0 = index_avant_paires[{ MOT_VIDE, MOT_VIDE }];
	auto idx1 = index_avant[morceaux[0]];

	matrice[idx0][idx1] += 1.0;

	idx0 = index_avant_paires[{ MOT_VIDE, morceaux[0] }];
	idx1 = index_avant[morceaux[1]];

	matrice[idx0][idx1] += 1.0;

//	idx0 = index_avant[morceaux[morceaux.taille() - 1]];
//	idx1 = index_avant[MOT_VIDE];

//	matrice[idx0][idx1] += 1.0;

	converti_fonction_repartition(matrice);

	//imprime_matrice("Matrice = \n", matrice, index_arriere);

	std::cerr << "Génère texte :\n";
	auto gna = GNA();

	auto mot_courant = MOT_VIDE;
	auto mot1 = MOT_VIDE;
	auto mot2 = MOT_VIDE;

	CHRONOMETRE_PORTEE("génération du texte", std::cerr);

	auto nombre_phrases = 5;

	while (nombre_phrases > 0) {
		auto paire_courante = std::pair{ mot1, mot2 };
		/* prend le vecteur du mot_courant */
		auto const &vec = matrice[index_avant_paires[paire_courante]];

		/* génère un mot */
		auto prob = gna.uniforme(0.0, 1.0);

		for (auto j = 0; j < mots.taille(); ++j) {
			if (prob <= vec[j]) {
				mot_courant = index_arriere[j];
				break;
			}
		}

		std::cerr << mot_courant;

		if (mot_courant == dls::vue_chaine(".")) {
			std::cerr << '\n';
			nombre_phrases--;
			//break;
//			mot1 = MOT_VIDE;
//			mot2 = MOT_VIDE;
		}
		else {
			std::cerr << ' ';
		}

		mot1 = mot2;
		mot2 = mot_courant;
	}
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

	test_markov_lettres_double(morceaux);

	std::cerr << "Mémoire consommée : " << memoire::formate_taille(memoire::consommee()) << '\n';

	return 0;
}
