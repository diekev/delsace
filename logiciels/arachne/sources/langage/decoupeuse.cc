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

#include "erreur.h"

//#define DEBOGUE_DECOUPEUR

#ifdef DEBOGUE_DECOUPEUR
# define LOG std::cerr
#else
struct Loggeur {};

template <typename T>
Loggeur &operator<<(Loggeur &l, const T &) { return l; }

Loggeur loggeur_principal;

# define LOG loggeur_principal
#endif

namespace arachne {

/* ************************************************************************** */

struct PaireIndentifiantChaine {
	int identifiant;
	std::string chaine;
};

static bool comparaison_paires_identifiant(
		const PaireIndentifiantChaine &a,
		const PaireIndentifiantChaine &b)
{
	return a.chaine < b.chaine;
}

/* Liste générée par genere_listes_mot_cle.py */
static const PaireIndentifiantChaine paires_identifiant[] = {
	{ IDENTIFIANT_BASE_DE_DONNEES, "BASE_DE_DONNÉES" },
	{ IDENTIFIANT_CHARGE, "CHARGE" },
	{ IDENTIFIANT_CREE, "CRÉE" },
	{ IDENTIFIANT_ET, "ET" },
	{ IDENTIFIANT_FAUX, "FAUX" },
	{ IDENTIFIANT_FICHIER, "FICHIER" },
	{ IDENTIFIANT_NUL, "NUL" },
	{ IDENTIFIANT_OU, "OU" },
	{ IDENTIFIANT_RETOURNE, "RETOURNE" },
	{ IDENTIFIANT_SI, "SI" },
	{ IDENTIFIANT_SUPPRIME, "SUPPRIME" },
	{ IDENTIFIANT_TROUVE, "TROUVE" },
	{ IDENTIFIANT_UTILISE, "UTILISE" },
	{ IDENTIFIANT_VRAI, "VRAI" },
	{ IDENTIFIANT_ECRIT, "ÉCRIT" },
};

/* Liste générée par genere_listes_mot_cle.py */
static PaireIndentifiantChaine paires_caracteres_double[] = {
	{ IDENTIFIANT_DIFFERENCE, "!=" },
	{ IDENTIFIANT_INFERIEUR_EGAL, "<=" },
	{ IDENTIFIANT_EGALITE, "==" },
	{ IDENTIFIANT_SUPERIEUR_EGAL, ">=" },
};

struct PaireIndentifiantCaractere {
	int identifiant;
	char caractere;
};

static bool comparaison_paires_identifiant_caractere(
		const PaireIndentifiantCaractere &a,
		const PaireIndentifiantCaractere &b)
{
	return a.caractere < b.caractere;
}

/* Liste générée par genere_listes_mot_cle.py */
static PaireIndentifiantCaractere paires_identifiant_caractere[] = {
	{ IDENTIFIANT_GUILLEMET, '"' },
	{ IDENTIFIANT_APOSTROPHE, '\'' },
	{ IDENTIFIANT_PARENTHESE_OUVRANTE, '(' },
	{ IDENTIFIANT_PARENTHESE_FERMANTE, ')' },
	{ IDENTIFIANT_VIRGULE, ',' },
	{ IDENTIFIANT_POINT, '.' },
	{ IDENTIFIANT_DOUBLE_POINT, ':' },
	{ IDENTIFIANT_POINT_VIRGULE, ';' },
	{ IDENTIFIANT_INFERIEUR, '<' },
	{ IDENTIFIANT_SUPERIEUR, '>' },
	{ IDENTIFIANT_CROCHET_OUVRANT, '[' },
	{ IDENTIFIANT_CROCHET_FERMANT, ']' },
	{ IDENTIFIANT_ACCOLADE_OUVRANTE, '{' },
	{ IDENTIFIANT_ACCOLADE_FERMANTE, '}' },
};

static bool est_nombre(const char caractere)
{
	return (caractere >= '0' && caractere <= '9');
}

bool est_espace_blanc(char caractere)
{
	return caractere == ' ' || caractere == '\n' || caractere == '\t';
}

bool est_caractere_special(char c, int &i)
{
	auto iterateur = std::lower_bound(
				std::begin(paires_identifiant_caractere),
				std::end(paires_identifiant_caractere),
				PaireIndentifiantCaractere{IDENTIFIANT_NUL, c},
				comparaison_paires_identifiant_caractere);

	if (iterateur != std::end(paires_identifiant_caractere)) {
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
				PaireIndentifiantChaine{IDENTIFIANT_NUL, chaine},
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
				std::begin(paires_identifiant),
				std::end(paires_identifiant),
				PaireIndentifiantChaine{IDENTIFIANT_NUL, chaine},
				comparaison_paires_identifiant);

	if (iterateur != std::end(paires_identifiant)) {
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

int extrait_nombre(const char *debut, const char *fin, std::string &chaine, int &id)
{
	int compte = 0;
	int etat = ETAT_NOMBRE_DEBUT;
	id = IDENTIFIANT_NOMBRE;

	while (debut != fin) {
		if (!est_nombre(*debut) && *debut != '.') {
			break;
		}

		if (*debut == '.') {
			if (etat == ETAT_NOMBRE_POINT) {
				throw ErreurFrappe("", 0, 0, "Erreur ! Le nombre contient un point en trop !\n");
			}

			etat = ETAT_NOMBRE_POINT;
			id = IDENTIFIANT_NOMBRE_DECIMAL;
		}

		chaine.push_back(*debut++);
		++compte;
	}

	return compte;
}

/* ************************************************************************** */

size_t trouve_taille_ligne(const std::string &chaine, size_t pos)
{
	const auto debut = pos;

	while (pos < chaine.size() && chaine[pos] != '\n') {
		++pos;
	}

	return pos - debut;
}

Decoupeuse::Decoupeuse(const std::string &chaine)
	: m_chaine(chaine)
	, m_position(0)
	, m_position_ligne(0)
	, m_ligne(0)
	, m_fini(false)
	, m_caractere_courant('\0')
{
	m_lignes.push_back({&m_chaine[m_position],
						trouve_taille_ligne(m_chaine, m_position)});
}

void Decoupeuse::impression_debogage(const std::string &quoi)
{
	std::cout << "Trouvé symbole " << quoi
			  << ", ligne : " << m_ligne
			  << ", position : " << m_position_ligne << ".\n";
}

void Decoupeuse::avance(int compte)
{
	m_position += compte;
	m_debut += compte;
	m_position_ligne += compte;
}

Decoupeuse::iterateur Decoupeuse::begin()
{
	return m_identifiants.begin();
}

Decoupeuse::iterateur Decoupeuse::end()
{
	return m_identifiants.end();
}

Decoupeuse::iterateur_const Decoupeuse::cbegin() const
{
	return m_identifiants.cbegin();
}

Decoupeuse::iterateur_const Decoupeuse::cend() const
{
	return m_identifiants.end();
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
void Decoupeuse::decoupe()
{
	LOG << "Démarrage de l'analyse d'une chaîne de "
		<< m_chaine.size() << " caractères.\n";

	std::string mot_courant;
	m_ligne = 1;
	m_position = 0;
	m_position_ligne = 0;

	m_debut = &m_chaine[0];
	m_fin = &m_chaine[m_chaine.size()];

	while (m_debut != m_fin) {
		int idc = IDENTIFIANT_NUL;

		if (est_espace_blanc(*m_debut)) {
			if (!mot_courant.empty()) {
				LOG << "Découpe mot courrant après espace blanc.\n";
				ajoute_identifiant(id_chaine(mot_courant), mot_courant);
				mot_courant = "";
			}

			if (*m_debut == '\n') {
				m_lignes.push_back({&m_chaine[m_position + 1],
									trouve_taille_ligne(m_chaine, m_position + 1)});

				++m_ligne;
				m_position_ligne = 0;
			}

			avance();
		}
		else if (est_caractere_special(*m_debut, idc)) {
			if (!mot_courant.empty()) {
				LOG << "Découpe mot courrant après caractère spécial.\n";
				ajoute_identifiant(id_chaine(mot_courant), mot_courant);
				mot_courant = "";
			}

			mot_courant.push_back(*m_debut);
			mot_courant.push_back(*(m_debut + 1));

			auto id = id_caractere_double(mot_courant);

			if (id != IDENTIFIANT_NUL) {
				LOG << "Découpe caractère double.\n";
				ajoute_identifiant(id, mot_courant);
				mot_courant = "";
				avance(2);
				continue;
			}

			mot_courant = "";

			if (*m_debut == '.') {
				if (est_nombre(*(m_debut + 1))) {
					LOG << "Découpe nombre commentaire par un point.\n";
					int compte = extrait_nombre(m_debut, m_fin, mot_courant, id);
					avance(compte);
					ajoute_identifiant(id, mot_courant);
					mot_courant = "";
				}
				else {
					mot_courant.push_back(*m_debut);
					ajoute_identifiant(IDENTIFIANT_POINT, mot_courant);
					mot_courant = "";
					avance();
				}
			}
			else if (*m_debut == '"') {
				LOG << "Découpe chaîne littérale.\n";

				// Saute le premier guillemet.
				avance();

				while (m_debut != m_fin) {
					if (*m_debut == '"' && *(m_debut - 1) != '\\') {
						break;
					}

					mot_courant.push_back(*m_debut);
					avance();
				}

				// Saute le dernier guillemet.
				avance();

				ajoute_identifiant(IDENTIFIANT_CHAINE_LITTERALE, mot_courant);
				mot_courant = "";
			}
			else if (*m_debut == '\'') {
				LOG << "Découpe lettre.\n";
				// Saute la première apostrophe.
				avance();

				mot_courant.push_back(*m_debut);
				avance();

				// Saute la dernière apostrophe.
				if (*m_debut != '\'') {
					throw ErreurFrappe(m_lignes[m_ligne - 1],
									   m_ligne,
									   m_position_ligne,
									   "Erreur : plusieurs caractère détectés !\n");
				}

				avance();

				ajoute_identifiant(IDENTIFIANT_CARACTERE, mot_courant);
				mot_courant = "";
			}
			else {
				LOG << "Découpe caractère simple.\n";
				mot_courant.push_back(*m_debut);
				ajoute_identifiant(idc, mot_courant);
				mot_courant = "";
				avance();
			}
		}
		else if (est_nombre(*m_debut) && mot_courant.empty()) {
			LOG << "Découpe nombre.\n";
			int id;
			int compte = extrait_nombre(m_debut, m_fin, mot_courant, id);
			avance(compte);
			ajoute_identifiant(id, mot_courant);
			mot_courant = "";
		}
		else {
			mot_courant.push_back(*m_debut);
			avance();
		}
	}

	LOG << "Fin de l'analyse. Ligne : " << m_ligne
		<< ". Position : " << m_position << ".\n";

	if (!mot_courant.empty()) {
		throw ErreurFrappe(
					m_lignes[m_ligne - 1],
					m_ligne,
					m_position_ligne,
					"Le script ne semble pas terminé !");
	}
}

void Decoupeuse::ajoute_identifiant(int identifiant, const std::string &contenu)
{
	DonneesMorceaux donnees;
	donnees.identifiant = identifiant;
	donnees.numero_ligne = m_ligne;
	donnees.position_ligne = m_position_ligne;
	donnees.contenu = contenu;
	donnees.ligne = m_lignes[m_ligne - 1];

	m_identifiants.push_back(donnees);
}

const std::vector<DonneesMorceaux> &Decoupeuse::morceaux() const
{
	return m_identifiants;
}

}  /* namespace arachne */

