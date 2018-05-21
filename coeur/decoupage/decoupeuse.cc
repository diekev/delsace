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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "decoupeuse.h"

#include <algorithm>
#include <iostream>
#include <cstring>

#include "erreur.h"

/* ************************************************************************** */

struct paire_identifiant_chaine {
	int identifiant;
	std::string chaine;
};

static bool comparaison_paires_identifiant(
		const paire_identifiant_chaine &a,
		const paire_identifiant_chaine &b)
{
	return a.chaine < b.chaine;
}

const paire_identifiant_chaine paires_mots_cles[] = {
	{ IDENTIFIANT_ARRETE, "arrête" },
	{ IDENTIFIANT_ASSOCIE, "associe" },
	{ IDENTIFIANT_BOOLEEN, "booléen" },
	{ IDENTIFIANT_BOUCLE, "boucle" },
	{ IDENTIFIANT_CHAINE, "chaîne" },
	{ IDENTIFIANT_CLASSE, "classe" },
	{ IDENTIFIANT_CONSTANT, "constant" },
	{ IDENTIFIANT_CONSTRUCTEUR, "constructeur" },
	{ IDENTIFIANT_DE, "de" },
	{ IDENTIFIANT_DESTRUCTEUR, "destructeur" },
	{ IDENTIFIANT_DECIMAL32, "décimal32" },
	{ IDENTIFIANT_DECIMAL64, "décimal64" },
	{ IDENTIFIANT_DEFAUT, "défaut" },
	{ IDENTIFIANT_ENTIER16, "entier16" },
	{ IDENTIFIANT_ENTIER16NS, "entier16ns" },
	{ IDENTIFIANT_ENTIER32, "entier32" },
	{ IDENTIFIANT_ENTIER32NS, "entier32ns" },
	{ IDENTIFIANT_ENTIER64, "entier64" },
	{ IDENTIFIANT_ENTIER64NS, "entier64ns" },
	{ IDENTIFIANT_ENTIER8, "entier8" },
	{ IDENTIFIANT_ENTIER8NS, "entier8ns" },
	{ IDENTIFIANT_ENUM, "enum" },
	{ IDENTIFIANT_EXPRIME, "exprime" },
	{ IDENTIFIANT_FAUX, "faux" },
	{ IDENTIFIANT_FONCTION, "fonction" },
	{ IDENTIFIANT_GABARIT, "gabarit" },
	{ IDENTIFIANT_IMPORTE, "importe" },
	{ IDENTIFIANT_IMPRIME, "imprime" },
	{ IDENTIFIANT_INDEX, "index" },
	{ IDENTIFIANT_OPERATEUR, "opérateur" },
	{ IDENTIFIANT_RETOURNE, "retourne" },
	{ IDENTIFIANT_SI, "si" },
	{ IDENTIFIANT_SINON, "sinon" },
	{ IDENTIFIANT_SOIT, "soit" },
	{ IDENTIFIANT_SORTIE, "sortie" },
	{ IDENTIFIANT_VRAI, "vrai" },
	{ IDENTIFIANT_ECHEC, "échec" },
};

const paire_identifiant_chaine paires_caracteres_double[] = {
	{ IDENTIFIANT_DIFFERENCE, "!=" },
	{ IDENTIFIANT_ESP_ESP, "&&" },
	{ IDENTIFIANT_ET_EGAL, "&=" },
	{ IDENTIFIANT_FOIS_EGAL, "*=" },
	{ IDENTIFIANT_PLUS_PLUS, "++" },
	{ IDENTIFIANT_PLUS_EGAL, "+=" },
	{ IDENTIFIANT_MOINS_MOINS, "--" },
	{ IDENTIFIANT_MOINS_EGAL, "-=" },
	{ IDENTIFIANT_FLECHE, "->" },
	{ IDENTIFIANT_TROIS_POINT, "..." },
	{ IDENTIFIANT_DIVISE_EGAL, "/=" },
	{ IDENTIFIANT_DECALAGE_GAUCHE, "<<" },
	{ IDENTIFIANT_INFERIEUR_EGAL, "<=" },
	{ IDENTIFIANT_EGALITE, "==" },
	{ IDENTIFIANT_SUPERIEUR_EGAL, ">=" },
	{ IDENTIFIANT_DECALAGE_DROITE, ">>" },
	{ IDENTIFIANT_OUX_EGAL, "^=" },
	{ IDENTIFIANT_OU_EGAL, "|=" },
	{ IDENTIFIANT_BARE_BARRE, "||" },
};

struct paire_identifiant_caractere {
	int identifiant;
	char caractere;
};

static bool comparaison_paires_caractere(
		const paire_identifiant_caractere &a,
		const paire_identifiant_caractere &b)
{
	return a.caractere < b.caractere;
}

const paire_identifiant_caractere paires_caracteres_speciaux[] = {
	{ IDENTIFIANT_EXCLAMATION, '!' },
	{ IDENTIFIANT_GUILLEMENT, '"' },
	{ IDENTIFIANT_DIESE, '#' },
	{ IDENTIFIANT_POURCENT, '%' },
	{ IDENTIFIANT_ESPERLUETTE, '&' },
	{ IDENTIFIANT_APOSTROPHE, '\'' },
	{ IDENTIFIANT_PARENTHESE_OUVRANTE, '(' },
	{ IDENTIFIANT_PARENTHESE_FERMANTE, ')' },
	{ IDENTIFIANT_FOIS, '*' },
	{ IDENTIFIANT_PLUS, '+' },
	{ IDENTIFIANT_MOINS, '-' },
	{ IDENTIFIANT_POINT, '.' },
	{ IDENTIFIANT_DIVISE, '/' },
	{ IDENTIFIANT_DOUBLE_POINT, ':' },
	{ IDENTIFIANT_POINT_VIRGULE, ';' },
	{ IDENTIFIANT_INFERIEUR, '<' },
	{ IDENTIFIANT_EGAL, '=' },
	{ IDENTIFIANT_SUPERIEUR, '>' },
	{ IDENTIFIANT_CROCHET_OUVRANT, '[' },
	{ IDENTIFIANT_CROCHET_FERMANT, ']' },
	{ IDENTIFIANT_CHAPEAU, '^' },
	{ IDENTIFIANT_ACCOLADE_OUVRANTE, '{' },
	{ IDENTIFIANT_BARRE, '|' },
	{ IDENTIFIANT_ACCOLADE_FERMANTE, '}' },
};

bool est_espace_blanc(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}

bool est_nombre(char c)
{
	return (c >= '0') && (c <= '9');
}

bool est_caractere_special(char c, int &i)
{
	auto iterateur = std::lower_bound(
				std::begin(paires_caracteres_speciaux),
				std::end(paires_caracteres_speciaux),
				paire_identifiant_caractere{IDENTIFIANT_NUL, c},
				comparaison_paires_caractere);

	if (iterateur != std::end(paires_caracteres_speciaux)) {
		if ((*iterateur).caractere == c) {
			i = (*iterateur).identifiant;
			return true;
		}
	}

	return false;
}

int id_caractere_double(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
				std::begin(paires_caracteres_double),
				std::end(paires_caracteres_double),
				paire_identifiant_chaine{IDENTIFIANT_NUL, chaine},
				comparaison_paires_identifiant);

	if (iterateur != std::end(paires_caracteres_double)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return IDENTIFIANT_NUL;
}

int id_chaine(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
				std::begin(paires_mots_cles),
				std::end(paires_mots_cles),
				paire_identifiant_chaine{IDENTIFIANT_NUL, chaine},
				comparaison_paires_identifiant);

	if (iterateur != std::end(paires_mots_cles)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return IDENTIFIANT_CHAINE_CARACTERE;
}

enum {
	ETAT_NOMBRE_POINT,
	ETAT_NOMBRE_EXPONENTIEL,
	ETAT_NOMBRE_DEBUT,
};

int extrait_nombre(const char *debut, const char *fin, std::string &chaine)
{
	int compte = 0;
	int etat = ETAT_NOMBRE_DEBUT;

	while (debut != fin) {
		if (!est_nombre(*debut) && *debut != '.') {
			break;
		}

		if (*debut == '.') {
			if ((*(debut + 1) == '.') && (*(debut + 2) == '.')) {
				break;
			}

			if (etat == ETAT_NOMBRE_POINT) {
				throw erreur::frappe("Erreur ! Le nombre contient un point en trop !\n");
			}

			etat = ETAT_NOMBRE_POINT;
		}

		chaine.push_back(*debut++);
		++compte;
	}

	return compte;
}

/* ************************************************************************** */

decoupeuse_texte::decoupeuse_texte(const char *debut, const char *fin)
	: m_debut(debut)
	, m_fin(fin)
{}

// si caractere blanc:
//    ajoute mot
// sinon si caractere speciale:
//    ajoute mot
//    si caractere suivant constitue caractere double
//        ajoute mot caractere double
//    sinon
//        si caractere est '.':
//            decoupe nombre ou trois point
//        sinon si caractere est '"':
//            decoupe chaine caractere littérale
//        sinon si caractere est '#':
//            decoupe commentaire
//        sinon si caractere est '\'':
//            decoupe caractere
//        sinon:
//        	ajoute mot caractere simple
// sinon si nombre et mot est vide:
//    decoupe nombre
// sinon:
//    ajoute caractere mot courant
void decoupeuse_texte::genere_morceaux()
{
	std::string mot_courant = "";

	const char *debut = m_debut;
	const char *fin = m_fin;

	while (debut != fin) {
		int idc = IDENTIFIANT_NUL;

		if (est_espace_blanc(*debut)) {
			if (!mot_courant.empty()) {
				m_morceaux.push_back({ mot_courant, id_chaine(mot_courant) });
				mot_courant = "";
			}

			++debut;
		}
		else if (est_caractere_special(*debut, idc)) {
			if (!mot_courant.empty()) {
				m_morceaux.push_back({ mot_courant, id_chaine(mot_courant) });
				mot_courant = "";
			}

			mot_courant.push_back(*debut);
			mot_courant.push_back(*(debut + 1));

			auto id = id_caractere_double(mot_courant);

			if (id != IDENTIFIANT_NUL) {
				m_morceaux.push_back({ mot_courant, id });
				mot_courant = "";
				debut += 2;
				continue;
			}

			mot_courant = "";

			if (*debut == '.') {
				if (*(debut + 1) == '.') {
					if (*(debut + 2) != '.') {
						throw erreur::frappe("Erreur : un point est manquant ou un point est en trop !\n");
					}

					m_morceaux.push_back({ "...", IDENTIFIANT_TROIS_POINT });
					mot_courant = "";
					debut += 3;
				}
				else if (est_nombre(*(debut + 1))) {
					int compte = extrait_nombre(debut, fin, mot_courant);
					debut += compte;
					m_morceaux.push_back({ mot_courant, IDENTIFIANT_NOMBRE });
					mot_courant = "";
				}
				else {
					throw erreur::frappe("Erreur : point innatendu !\n");
				}
			}
			else if (*debut == '"') {
				// Saute le premier guillemet.
				++debut;

				while (debut != fin) {
					if (*debut == '"' && *(debut - 1) != '\\') {
						break;
					}
					mot_courant.push_back(*debut++);
				}

				// Saute le dernier guillemet.
				if (*debut != '\'') {
					throw erreur::frappe("Erreur : plusieurs caractère détectés !\n");
				}
				++debut;

				m_morceaux.push_back({ mot_courant, IDENTIFIANT_CHAINE_LITTERALE });
				mot_courant = "";
			}
			else if (*debut == '\'') {
				// Saute le premier guillemet.
				++debut;

				mot_courant.push_back(*debut++);

				// Saute le dernier guillemet.
				++debut;

				m_morceaux.push_back({ mot_courant, IDENTIFIANT_CARACTERE });
				mot_courant = "";
			}
			else if (*debut == '#') {
				// ignore commentaire
				while (*debut != '\n') {
					++debut;
				}
			}
			else {
				mot_courant.push_back(*debut);
				m_morceaux.push_back({ mot_courant, idc });
				mot_courant = "";
				++debut;
			}
		}
		else if (est_nombre(*debut) && mot_courant.empty()) {
			int compte = extrait_nombre(debut, fin, mot_courant);
			debut += compte;
			m_morceaux.push_back({ mot_courant, IDENTIFIANT_NOMBRE });
			mot_courant = "";
		}
		else {
			mot_courant.push_back(*debut++);
		}
	}

	if (!mot_courant.empty()) {
		throw erreur::frappe("Erreur : des caractères en trop se trouve à la fin du texte !");
	}
}

decoupeuse_texte::iterateur decoupeuse_texte::begin()
{
	return m_morceaux.begin();
}

decoupeuse_texte::iterateur decoupeuse_texte::end()
{
	return m_morceaux.end();
}
