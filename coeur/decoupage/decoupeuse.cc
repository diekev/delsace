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
#include <sstream>

#include "erreur.h"
#include "nombres.h"
#include "unicode.h"

/* ************************************************************************** */

static size_t trouve_fin_ligne(const char *debut, const char *fin)
{
	size_t pos = 0;

	while (debut != fin) {
		++pos;

		if (*debut == '\n') {
			break;
		}

		++debut;
	}

	return pos;
}

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
	{ IDENTIFIANT_GUILLEMET, '"' },
	{ IDENTIFIANT_DIESE, '#' },
	{ IDENTIFIANT_POURCENT, '%' },
	{ IDENTIFIANT_ESPERLUETTE, '&' },
	{ IDENTIFIANT_APOSTROPHE, '\'' },
	{ IDENTIFIANT_PARENTHESE_OUVRANTE, '(' },
	{ IDENTIFIANT_PARENTHESE_FERMANTE, ')' },
	{ IDENTIFIANT_FOIS, '*' },
	{ IDENTIFIANT_PLUS, '+' },
	{ IDENTIFIANT_VIRGULE, ',' },
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

/* ************************************************************************** */

decoupeuse_texte::decoupeuse_texte(const char *debut, const char *fin)
	: m_debut_orig(debut)
	, m_debut(debut)
	, m_fin(fin)
{
	m_ligne_courante = std::string_view(m_debut, trouve_fin_ligne(debut, fin));
}

void decoupeuse_texte::genere_morceaux()
{
	std::string mot_courant = "";

	while (m_debut != m_fin) {
		const auto nombre_octet = nombre_octets(m_debut);

		if (nombre_octet == 1) {
			analyse_caractere_simple(mot_courant);
		}
		else if (nombre_octet >= 2 && nombre_octet <= 4) {
			/* Les caractères spéciaux ne peuvent être des caractères unicode
			 * pour le moment, donc on les copie directement dans le tampon du
			 * mot_courant. */
			for (int i = 0; i < nombre_octet; ++i) {
				mot_courant.push_back(this->caractere_courant());
				this->avance();
			}
		}
		else {
			/* Le caractère (octet) courant est invalide dans le codec unicode. */
			lance_erreur("Le codec Unicode ne peut comprendre le caractère !");
		}
	}

	if (!mot_courant.empty()) {
		lance_erreur("Des caractères en trop se trouve à la fin du texte !");
	}
}

const std::vector<DonneesMorceaux> &decoupeuse_texte::morceaux() const
{
	return m_morceaux;
}

decoupeuse_texte::iterateur decoupeuse_texte::begin()
{
	return m_morceaux.begin();
}

decoupeuse_texte::iterateur decoupeuse_texte::end()
{
	return m_morceaux.end();
}

void decoupeuse_texte::avance(int n)
{
	for (int i = 0; i < n; ++i) {
		if (this->caractere_courant() == '\n') {
			++m_compte_ligne;
			m_position_ligne = 0;

			m_ligne_courante = std::string_view(m_debut + 1, trouve_fin_ligne(m_debut + 1, m_fin));
		}
		else {
			++m_position_ligne;
		}

		++m_debut;
	}
}

char decoupeuse_texte::caractere_courant() const
{
	return *m_debut;
}

char decoupeuse_texte::caractere_voisin(int n) const
{
	return *(m_debut + n);
}

void decoupeuse_texte::pousse_mot(std::string &mot_courant, int identifiant)
{
	m_morceaux.push_back({ mot_courant, identifiant });
	mot_courant = "";
}

void decoupeuse_texte::lance_erreur(const std::string &quoi) const
{
	std::stringstream ss;
	ss << "Erreur : ligne:" << m_compte_ligne << ":\n";
	ss << m_ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (int i = 0; i < m_position_ligne; i += nombre_octets(&m_ligne_courante[i])) {
		if (m_ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}
	}

	ss << "^~~~\n";
	ss << quoi;

	throw erreur::frappe(ss.str().c_str());
}

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
void decoupeuse_texte::analyse_caractere_simple(std::string &mot_courant)
{
	int idc = IDENTIFIANT_NUL;

	if (est_espace_blanc(this->caractere_courant())) {
		if (!mot_courant.empty()) {
			this->pousse_mot(mot_courant, id_chaine(mot_courant));
		}

		this->avance();
	}
	else if (est_caractere_special(this->caractere_courant(), idc)) {
		if (!mot_courant.empty()) {
			this->pousse_mot(mot_courant, id_chaine(mot_courant));
		}

		mot_courant.push_back(this->caractere_courant());
		mot_courant.push_back(this->caractere_voisin());

		auto id = id_caractere_double(mot_courant);

		if (id != IDENTIFIANT_NUL) {
			this->pousse_mot(mot_courant, id);
			this->avance(2);
			return;
		}

		mot_courant = "";

		if (this->caractere_courant() == '.') {
			if (this->caractere_voisin() == '.') {
				if (this->caractere_voisin(2) != '.') {
					lance_erreur("Un point est manquant ou un point est en trop !\n");
				}

				mot_courant = "...";
				this->pousse_mot(mot_courant, IDENTIFIANT_TROIS_POINT);
				this->avance(3);
			}
			else if (est_nombre_decimal(this->caractere_voisin())) {
				int id_nombre;
				int compte = extrait_nombre(m_debut, m_fin, mot_courant, id_nombre);
				this->avance(compte);
				this->pousse_mot(mot_courant, id_nombre);
			}
			else {
				lance_erreur("Point inattendu !\n");
			}
		}
		else if (this->caractere_courant() == '"') {
			// Saute le premier guillemet.
			this->avance();

			while (m_debut != m_fin) {
				if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
					break;
				}

				mot_courant.push_back(this->caractere_courant());
				this->avance();
			}

			// Saute le dernier guillemet.
			this->avance();

			this->pousse_mot(mot_courant, IDENTIFIANT_CHAINE_LITTERALE);
		}
		else if (this->caractere_courant() == '\'') {
			// Saute la première apostrophe.
			this->avance();

			mot_courant.push_back(this->caractere_courant());
			this->avance();

			// Saute la dernière apostrophe.
			if (this->caractere_courant() != '\'') {
				lance_erreur("Plusieurs caractères détectés dans un caractère simple !\n");
			}
			this->avance();

			this->pousse_mot(mot_courant, IDENTIFIANT_CARACTERE);
		}
		else if (this->caractere_courant() == '#') {
			// ignore commentaire
			while (this->caractere_courant() != '\n') {
				this->avance();
			}
		}
		else {
			mot_courant.push_back(this->caractere_courant());
			this->pousse_mot(mot_courant, idc);
			this->avance();
		}
	}
	else if (est_nombre_decimal(this->caractere_courant()) && mot_courant.empty()) {
		int id_nombre;
		const auto compte = extrait_nombre(m_debut, m_fin, mot_courant, id_nombre);
		this->avance(compte);
		this->pousse_mot(mot_courant, id_nombre);
	}
	else {
		mot_courant.push_back(this->caractere_courant());
		this->avance();
	}
}
