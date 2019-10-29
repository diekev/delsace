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
#include "biblinternes/structures/matrice_eparse.hh"
#include "biblinternes/structures/tableau.hh"

using type_matrice_ep = matrice_colonne_eparse<double>;

/**
 * PROBLÈMES :
 * - les guillemets ne sont pas ouverts ou fermés
 * - il arrive que des phrases ne contiennent que « m ».
 */

using type_scalaire = double;

struct type_matrice {
	long res_x = 0;
	long res_y = 0;
	dls::tableau<type_scalaire> donnees{};

	type_matrice(long rx, long ry)
		: res_x(rx)
		, res_y(ry)
		, donnees(rx * ry)
	{}

	type_scalaire *operator[](long idx)
	{
		return &donnees[idx * res_x];
	}

	type_scalaire const *operator[](long idx) const
	{
		return &donnees[idx * res_x];
	}
};

static auto loge_matrice(long lignes, long colonnes)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	return type_matrice(colonnes, lignes);
}

static auto MOT_VIDE = dls::vue_chaine("");

template <typename T>
void imprime_matrice(
		dls::chaine const &texte,
		type_matrice const &matrice,
		dls::dico_desordonne<long, T> &index_arriere)
{
	auto courante = 0;

	std::cerr << texte << " :\n";

	for (auto y = 0; y < matrice.res_y; ++y) {
		auto vec = matrice[y];

		std::cerr << index_arriere[courante] << " :";
		auto virgule = ' ';

		for (auto x = 0; x < matrice.res_x; ++x) {
			std::cerr << virgule << vec[x];
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
	static constexpr auto _0 = static_cast<type_scalaire>(0);

	auto nombre_cellules = 0l;

	for (auto y = 0; y < matrice.res_y; ++y) {
		auto vec = matrice[y];
		for (auto x = 0; x < matrice.res_x; ++x) {
			if (vec[x] != 0.0) {
				nombre_cellules++;
			}
		}
	}

	auto ratio = static_cast<double>(nombre_cellules);
	ratio /= static_cast<double>(matrice.res_x * matrice.res_y);

	std::cerr << "La matrice est pleine à " << ratio * 100.0 << " %\n";

	for (auto y = 0; y < matrice.res_y; ++y) {
		auto vec = matrice[y];
		auto total = _0;

		for (auto x = 0; x < matrice.res_x; ++x) {
			total += vec[x];
		}

		// que faire si une lettre n'est suivit par aucune ? on s'arrête ?
		if (total == _0) {
			continue;
		}

		for (auto x = 0; x < matrice.res_x; ++x) {
			vec[x] /= total;
		}

		auto accum = _0;

		for (auto x = 0; x < matrice.res_x; ++x) {
			accum += vec[x];
			vec[x] = accum;
		}
	}
}

static void converti_fonction_repartition(type_matrice_ep &matrice)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	static constexpr auto _0 = static_cast<type_scalaire>(0);

	for (auto y = 0; y < matrice.lignes.taille(); ++y) {
		auto &ligne = matrice.lignes[y];
		auto total = _0;

		for (auto n : ligne) {
			total += n->valeur;
		}

		// que faire si une lettre n'est suivit par aucune ? on s'arrête ?
		if (total == _0) {
			continue;
		}

		for (auto n : ligne) {
			n->valeur /= total;
		}

		auto accum = _0;

		for (auto n : ligne) {
			accum += n->valeur;
			n->valeur = accum;
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
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);
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
	auto matrice = loge_matrice(lettres.taille(), lettres.taille());

	for (auto const &mot : mots) {
		auto n0 = 0;
		auto idx0 = index_avant[MOT_VIDE];
		auto n1 = lng::nombre_octets(&mot[0]);
		auto idx1 = index_avant[dls::vue_chaine(&mot[0], n1)];

		matrice[idx0][idx1] += _1;

		for (auto i = n1; i < mot.taille() - 1;) {
			n0 = lng::nombre_octets(&mot[i]);
			idx0 = index_avant[dls::vue_chaine(&mot[i], n0)];
			n1 = lng::nombre_octets(&mot[i + n0]);
			idx1 = index_avant[dls::vue_chaine(&mot[i + n0], n1)];

			matrice[idx0][idx1] += _1;
			i += n0 + n1;
		}

		idx0 = index_avant[MOT_VIDE];
		matrice[idx1][idx0] += _1;
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
			auto prob = gna.uniforme(_0, _1);

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
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);
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
	courante = 0;

	for (auto l : paires_lettres) {
		index_avant_paires.insere({l, courante});
		courante += 1;
	}

	// construction de la matrice
	auto matrice = loge_matrice(paires_lettres.taille(), lettres.taille());

	for (auto const &mot : mots) {
		auto lettres_mot = epelle_mot(mot);

		for (auto i = 0; i < lettres_mot.taille() - 2; ++i) {
			auto paire = type_paire_lettres(lettres_mot[i], lettres_mot[i + 1]);
			auto lettre = lettres_mot[i + 2];

			auto idx0 = index_avant_paires[paire];
			auto idx1 = index_avant[lettre];

			matrice[idx0][idx1] += _1;
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
			auto prob = gna.uniforme(_0, _1);

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
					taille_mot = 0;
				}

				morceaux.pousse(lettre);
			}
			else {
				if (taille_mot == 0) {
					ptr = &texte[i];
				}

				taille_mot += n;
			}
		}
		else {
			switch (texte[i]) {
				case ' ':
				case '\t':
				case '\n':
				{
					if (taille_mot != 0) {
						morceaux.pousse(dls::vue_chaine(ptr, taille_mot));
						taille_mot = 0;
					}

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
						taille_mot = 0;
					}

					morceaux.pousse(dls::vue_chaine(&texte[i], 1));
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
			dls::paire(dls::vue_chaine("Î"), dls::vue_chaine("î")),
			dls::paire(dls::vue_chaine("Ç"), dls::vue_chaine("ç"))
			);

static auto dico_majuscule = dls::cree_dico(
			dls::paire(dls::vue_chaine("é"), dls::vue_chaine("É")),
			dls::paire(dls::vue_chaine("è"), dls::vue_chaine("È")),
			dls::paire(dls::vue_chaine("ê"), dls::vue_chaine("Ê")),
			dls::paire(dls::vue_chaine("à"), dls::vue_chaine("À")),
			dls::paire(dls::vue_chaine("ô"), dls::vue_chaine("Ô")),
			dls::paire(dls::vue_chaine("î"), dls::vue_chaine("Î")),
			dls::paire(dls::vue_chaine("ç"), dls::vue_chaine("Ç"))
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

static auto capitalise(dls::vue_chaine const &chaine)
{
	dls::chaine resultat;

	auto n = lng::nombre_octets(&chaine[0]);

	if (n > 1) {
		auto lettre = dls::vue_chaine(&chaine[0], n);
		auto plg_lettre = dico_majuscule.trouve(lettre);

		if (!plg_lettre.est_finie()) {
			lettre = plg_lettre.front().second;
		}

		resultat += lettre;
	}
	else {
		resultat += static_cast<char>(toupper(chaine[0]));
	}

	for (auto i = n; i < chaine.taille(); ++i) {
		resultat += chaine[i];
	}

	return resultat;
}

void test_markov_mot_simple(dls::tableau<dls::vue_chaine> const &morceaux)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

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
	auto matrice = loge_matrice(mots.taille(), mots.taille());

	for (auto i = 0; i < morceaux.taille() - 1; ++i) {
		auto idx0 = index_avant[morceaux[i]];
		auto idx1 = index_avant[morceaux[i + 1]];

		matrice[idx0][idx1] += _1;
	}

	/* conditions de bordures : il y a un mot vide avant et après le texte */
	auto idx0 = index_avant[MOT_VIDE];
	auto idx1 = index_avant[morceaux[0]];

	matrice[idx0][idx1] += _1;

	idx0 = index_avant[morceaux[morceaux.taille() - 1]];
	idx1 = index_avant[MOT_VIDE];

	matrice[idx0][idx1] += _1;

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
		auto prob = gna.uniforme(_0, _1);

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

struct magasin_paires {
	dls::dico_desordonne<dls::vue_chaine, dls::dico_desordonne<dls::vue_chaine, long>> donnees{};

	void insere(dls::vue_chaine const &a, dls::vue_chaine const &b, long idx)
	{
		donnees[a].insere({b, idx});
	}

	bool trouve(dls::vue_chaine const &a, dls::vue_chaine const &b)
	{
		auto itera = donnees.trouve(a);

		if (itera == donnees.fin()) {
			return false;
		}

		auto iterb = itera->second.trouve(b);

		if (iterb == itera->second.fin()) {
			return false;
		}

		return true;
	}

	long operator()(dls::vue_chaine const &a, dls::vue_chaine const &b)
	{
		return donnees[a][b];
	}
};

#ifdef ARBRE_MOT
struct FileMot {
	dls::tableau<dls::vue_chaine> mots{};

	void pousse(dls::vue_chaine const &mot)
	{
		mots.pousse(mot);

		if (mots.taille() > 2) {
			mots.pop_front();
		}
	}

	bool est_valide() const
	{
		return mots.taille() == 2;
	}
};

struct Indexeuse {
	dls::ensemble<dls::vue_chaine> mots{};
	FileMot file_courante{};

	dls::dico_desordonne<dls::vue_chaine, long> index_avant{};
	dls::dico_desordonne<long, dls::vue_chaine> index_arriere{};

	struct noeud {
		dls::vue_chaine mot{};
		long index = -1;

		dls::tableau<noeud> enfants{};

		noeud() = default;
	};

	long m_nombre_chaines = 0;
	long m_nombre_branches = 0;

	noeud racine{};

	/* ajoute un mot dans l'indexeuse, en construisant l'arbre d'adjacence */
	void insere(dls::vue_chaine const &mot)
	{
		mots.insere(mot);

		/* pousse le mot dans la file courante, et construit une nouvelle branche dans l'arbre au besoin */
		file_courante.pousse(mot);

		if (file_courante.est_valide()) {
			construit_branche(file_courante);
		}
	}

	void construit_branche(FileMot const &file_mot)
	{
		auto noeud_courant = &racine;

		for (auto i = 0; i < file_mot.mots.taille(); ++i) {
			auto noeud_enfant = static_cast<noeud *>(nullptr);

			for (auto &enfant : noeud_courant->enfants) {
				if (enfant.mot == file_mot.mots[i]) {
					noeud_enfant = &enfant;
					break;
				}
			}

			if (noeud_enfant == nullptr) {
				++m_nombre_branches;
				auto nouveau_noeud = noeud();
				nouveau_noeud.mot = file_mot.mots[i];
				noeud_courant->enfants.pousse(nouveau_noeud);

				noeud_courant = &noeud_courant->enfants.back();
			}
		}
	}

	dls::chaine mot_index(long idx)
	{
		return index_arriere[idx];
	}

	long index_chaine(FileMot const &file_mot)
	{
		auto noeud_courant = &racine;

		for (auto i = 0; i < file_mot.mots.taille(); ++i) {
			for (auto &enfant : noeud_courant->enfants) {
				if (enfant.mot == file_mot.mots[i]) {
					noeud_courant = &enfant;
					break;
				}
			}
		}

		return noeud_courant->index;
	}

	long index_mot(dls::vue_chaine const &mot)
	{
		return index_avant[mot];
	}

	void construit_index()
	{
		/* construit l'index pour les mots seuls */
		long courante = 0;

		for (auto mot : mots) {
			index_avant.insere({mot, courante});
			index_arriere.insere({courante, mot});
			courante += 1;
		}

		/* construit l'index pour les chaines de mots */
		CHRONOMETRE_PORTEE("construction index chaines de mots", std::cerr);
		traverse_arbre(&racine);
	}

	void traverse_arbre(noeud *n)
	{
		if (n->enfants.est_vide()) {
			n->index = m_nombre_chaines++;
			return;
		}

		for (auto &enfant : n->enfants) {
			traverse_arbre(&enfant);
		}
	}

	long nombre_chaines()
	{
		return m_nombre_chaines;
	}

	long nombre_mots()
	{
		return mots.taille();
	}
};

void test_markov_mots_paire2(dls::tableau<dls::vue_chaine> const &morceaux)
{
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

	auto ordre = 2;

	/* construction de l'index */
	auto indexeuse = Indexeuse();

	for (auto i = 0; i < morceaux.taille(); ++i) {
		indexeuse.insere(morceaux[i]);
	}

	indexeuse.construit_index();

	std::cerr << "Il y a " << indexeuse.nombre_mots() << " mots dans le texte.\n";
	std::cerr << "Il y a " << indexeuse.m_nombre_branches << " branches dans l'arbre.\n";
	std::cerr << "Il y a " << indexeuse.nombre_chaines() << " paires de mots dans le texte.\n";

	/* construction de la matrice */
	auto matrice = type_matrice_ep(
				type_ligne(indexeuse.nombre_chaines()),
				type_colonne(indexeuse.nombre_mots()));

	auto file_mot = FileMot();

	/* conditions de bordures : il y a des mots vide avant le texte */
	for (auto i = 0; i < ordre; ++i) {
		file_mot.pousse(MOT_VIDE);
	}

	for (auto i = 0; i < morceaux.taille(); ++i) {
		auto idx0 = indexeuse.index_chaine(file_mot);
		auto idx1 = indexeuse.index_mot(morceaux[i]);

		matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
	}

	matrice_valide(matrice);
	converti_fonction_repartition(matrice);

	//imprime_matrice("Matrice = \n", matrice, index_arriere);

	std::cerr << "Génère texte :\n";
	auto gna = GNA();
	file_mot = FileMot();

	auto mot_courant = MOT_VIDE;

	CHRONOMETRE_PORTEE("génération du texte", std::cerr);

	auto nombre_phrases = 5;
	auto premier_mot = true;
	auto dernier_mot = dls::vue_chaine();

	while (nombre_phrases > 0) {
		/* prend le vecteur du mot_courant */
		auto ligne = matrice.lignes[indexeuse.index_chaine(file_mot)];
		auto n = ligne;

		/* génère un mot */
		auto prob = gna.uniforme(_0, _1);

		while (n != nullptr) {
			if (prob <= n->valeur) {
				mot_courant = indexeuse.mot_index(n->colonne);
				break;
			}

			n = n->suivant;
		}

		if (premier_mot) {
			std::cerr << capitalise(mot_courant);
		}
		else {
			auto espace_avant = !dls::outils::est_element(mot_courant, ",", ".", "’", "'");
			auto espace_apres = !dls::outils::est_element(dernier_mot, "’", "'");

			if (espace_avant && espace_apres) {
				std::cerr << ' ';
			}

			std::cerr << mot_courant;
		}

		dernier_mot = mot_courant;
		premier_mot = false;

		if (mot_courant == dls::vue_chaine(".")) {
			std::cerr << '\n';
			premier_mot = true;
			nombre_phrases--;
		}

		file_mot.pousse(mot_courant);
	}
}
#endif

void test_markov_mots_paire(dls::tableau<dls::vue_chaine> const &morceaux)
{
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

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

	magasin_paires index_avant_paires;
	courante = 0;

	for (auto paire : paire_mots) {
		index_avant_paires.insere(paire.first, paire.second, courante);
		courante += 1;
	}

	std::cerr << "Le magasin possède " << index_avant_paires.donnees.taille() << " paires\n";

	/* construction de la matrice */
	auto matrice = type_matrice_ep(type_ligne(paire_mots.taille()), type_colonne(mots.taille()));

	for (auto i = 0; i < morceaux.taille() - 2; ++i) {
		auto idx0 = index_avant_paires(morceaux[i], morceaux[i + 1]);
		auto idx1 = index_avant[morceaux[i + 2]];

		matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
	}

	/* conditions de bordures : il y a un mot vide avant et après le texte */
	auto idx0 = index_avant_paires(MOT_VIDE, MOT_VIDE);
	auto idx1 = index_avant[morceaux[0]];

	matrice(type_ligne(idx0), type_colonne(idx1)) += _1;

	idx0 = index_avant_paires(MOT_VIDE, morceaux[0]);
	idx1 = index_avant[morceaux[1]];

	matrice(type_ligne(idx0), type_colonne(idx1)) += _1;

//	idx0 = index_avant[morceaux[morceaux.taille() - 1]];
//	idx1 = index_avant[MOT_VIDE];

//	matrice(type_ligne(idx0), type_colonne(idx1)) += _1;

//	matrice_valide(matrice);
	tri_lignes_matrice(matrice);
	converti_fonction_repartition(matrice);

	//imprime_matrice("Matrice = \n", matrice, index_arriere);

	std::cerr << "Génère texte :\n";
	auto gna = GNA();

	auto mot_courant = MOT_VIDE;
	auto mot1 = MOT_VIDE;
	auto mot2 = MOT_VIDE;

	CHRONOMETRE_PORTEE("génération du texte", std::cerr);

	auto nombre_phrases = 5;
	auto premier_mot = true;
	auto dernier_mot = dls::vue_chaine();

	while (nombre_phrases > 0) {
		/* prend le vecteur du mot_courant */
		if (!index_avant_paires.trouve(mot1, mot2)) {
			std::cerr << "Ne peut pas trouver l'index de la paire <" << mot1 << "," << mot2 << "> !\n";
			break;
		}

		/* génère un mot */
		auto prob = gna.uniforme(_0, _1);

		auto &ligne = matrice.lignes[index_avant_paires(mot1, mot2)];

		for (auto n : ligne) {
			if (prob <= n->valeur) {
				mot_courant = index_arriere[n->colonne];
				break;
			}
		}

		if (premier_mot) {
			std::cerr << capitalise(mot_courant);
		}
		else {
			auto espace_avant = !dls::outils::est_element(mot_courant, ",", ".", "’", "'");
			auto espace_apres = !dls::outils::est_element(dernier_mot, "’", "'");

			if (espace_avant && espace_apres) {
				std::cerr << ' ';
			}

			std::cerr << mot_courant;
		}

		dernier_mot = mot_courant;
		premier_mot = false;

		if (mot_courant == dls::vue_chaine(".")) {
			std::cerr << '\n';
			premier_mot = true;
			nombre_phrases--;
			//break;
//			mot1 = MOT_VIDE;
//			mot2 = MOT_VIDE;
		}
		mot1 = mot2;
		mot2 = mot_courant;
	}
}

static inline auto est_mot_vide(dls::vue_chaine const &mot)
{
	return dls::outils::est_element(
				mot,
				"’",
				"'",
				".",
				",",
				"(",
				")",
				"{",
				"}",
				"[",
				"]",
				"?",
				";",
				"!",
				":",
				"*",
				"«",
				"»",
				"-",
				"l",
				"la",
				"le",
				"les",
				"ne",
				"qu",
				"que",
				"qui",
				"quoi",
				"c",
				"ce",
				"ces",
				"cet",
				"cette",
				"d",
				"de",
				"des",
				"à",
				"et",
				"ou",
				"en",
				"m");
}

static auto trouve_synonymes(dls::tableau<dls::vue_chaine> const &morceaux)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);

	/* construction de l'index */
	auto mots = dls::ensemble<dls::vue_chaine>();

	for (auto morceau : morceaux) {
		if (est_mot_vide(morceau)) {
			continue;
		}

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

	/* construction de la matrice d'adjacence */
	auto matrice = loge_matrice(mots.taille(), mots.taille());
	auto taille_fenetre = 2l;

	static constexpr auto _1 = static_cast<type_scalaire>(1);

	for (auto i = 0l; i < morceaux.taille(); ++i) {
		if (est_mot_vide(morceaux[i])) {
			continue;
		}

		auto idx0 = index_avant[morceaux[i]];

		for (auto j = std::max(0l, i - taille_fenetre); j <= std::min(i + taille_fenetre, morceaux.taille() - 1); ++j) {
			if (j == i) {
				continue;
			}

			if (est_mot_vide(morceaux[j])) {
				continue;
			}

			auto idx1 = index_avant[morceaux[j]];

			matrice[idx0][idx1] += _1 + static_cast<type_scalaire>(taille_fenetre - std::abs(j - i));
		}
	}

	//imprime_matrice("matrice", matrice, index_arriere);

	/* prend un mot aléatoire */
	auto gna = GNA();

	auto calcule_adjancences = [&](dls::chaine const &mot, long idx_mot, type_scalaire *vec_mot)
	{
		static constexpr auto _0 = static_cast<type_scalaire>(0);

		CHRONOMETRE_PORTEE("calcule_adjancences", std::cerr);

		dls::tableau<dls::paire<long, type_scalaire>> adjacences;
		adjacences.reserve(mots.taille() - 1);

		for (auto i = 0; i < mots.taille(); ++i) {
			if (i == idx_mot) {
				continue;
			}

			auto const &vec = matrice[i];

			auto adj = _0;

			for (auto j = 0; j < mots.taille(); ++j) {
				adj += vec_mot[j] * vec[j];
			}

			adjacences.pousse({ i, adj });
		}

		std::sort(adjacences.debut(), adjacences.fin(), [](dls::paire<long, type_scalaire> const &a, dls::paire<long, type_scalaire> const &b)
		{
			return a.second > b.second;
		});

		std::cerr << "Synonymes pour '" << mot << "' :\n";

		if (adjacences[0].second == _0) {
			std::cerr << "\taucun synonyme\n";
			return;
		}

		for (auto i = 0; i < std::min(10l, adjacences.taille()); ++i) {
			if (adjacences[i].second == _0) {
				break;
			}

			std::cerr << '\t' << index_arriere[adjacences[i].premier] << " (" << adjacences[i].second << ")\n";
		}
	};

	for (auto k = 0; k < 5; ++k) {
		auto idx_mot = gna.uniforme(1l, mots.taille() - 1);
		auto const &vec_mot = matrice[idx_mot];

		auto mot = index_arriere[idx_mot];

		calcule_adjancences(mot, idx_mot, vec_mot);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "Utilisation : exemple_markov FICHIER\n";
		return 1;
	}

	CHRONOMETRE_PORTEE(__func__, std::cerr);

	//auto texte = dls::chaine("Le roi est mort. Vive le roi. La reine est morte. La reine est vivante.");
	auto texte = dls::contenu_fichier(argv[1]);
	texte = en_minuscule(texte);

	auto morceaux = morcelle(texte);

//	std::cerr << "Les cents premier morceaux : \n";

//	for (auto i = 0; i < 100; ++i) {
//		std::cerr << morceaux[i] << ' ';
//	}

//	std::cerr << '\n';

	std::cerr << "Il y a " << morceaux.taille() << " morceaux dans le texte.\n";

#ifdef ARBRE_MOT
	test_markov_mots_paire2(morceaux);
#else
	test_markov_mots_paire(morceaux);
#endif

	std::cerr << "Mémoire consommée : " << memoire::formate_taille(memoire::consommee()) << '\n';

	return 0;
}
