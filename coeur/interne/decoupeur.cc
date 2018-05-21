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

#include "decoupeur.h"

#include <algorithm>
#include <iostream>

#include "erreur.h"
#include "morceaux.h"

//#define DEBOGUE_DECOUPEUR

#ifdef DEBOGUE_DECOUPEUR
# define LOG std::cerr
#else
struct Loggeur {};

template <typename T>
Loggeur &operator<<(Loggeur &, const T &) {}

Loggeur loggeur_principal;

# define LOG loggeur_principal
#endif

namespace kangao {

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

/* Liste générée par genere_donnees_mot_cle.py */
static PaireIndentifiantChaine paires_identifiant[] = {
	{ IDENTIFIANT_ACTION, "action" },
	{ IDENTIFIANT_ATTACHE, "attache" },
	{ IDENTIFIANT_BARRE_OUTILS, "barre_outils" },
	{ IDENTIFIANT_BOUTON, "bouton" },
	{ IDENTIFIANT_CASE, "case" },
	{ IDENTIFIANT_CHAINE, "chaine" },
	{ IDENTIFIANT_COLONNE, "colonne" },
	{ IDENTIFIANT_COULEUR, "couleur" },
	{ IDENTIFIANT_DISPOSITION, "disposition" },
	{ IDENTIFIANT_DOSSIER, "dossier" },
	{ IDENTIFIANT_DECIMAL, "décimal" },
	{ IDENTIFIANT_ENTIER, "entier" },
	{ IDENTIFIANT_ENTREE, "entrée" },
	{ IDENTIFIANT_FAUX, "faux" },
	{ IDENTIFIANT_FEUILLE, "feuille" },
	{ IDENTIFIANT_FICHIER_ENTREE, "fichier_entrée" },
	{ IDENTIFIANT_FICHIER_SORTIE, "fichier_sortie" },
	{ IDENTIFIANT_ICONE, "icône" },
	{ IDENTIFIANT_INFOBULLE, "infobulle" },
	{ IDENTIFIANT_INTERFACE, "interface" },
	{ IDENTIFIANT_ITEMS, "items" },
	{ IDENTIFIANT_LIGNE, "ligne" },
	{ IDENTIFIANT_LISTE, "liste" },
	{ IDENTIFIANT_LOGIQUE, "logique" },
	{ IDENTIFIANT_MAX, "max" },
	{ IDENTIFIANT_MENU, "menu" },
	{ IDENTIFIANT_MIN, "min" },
	{ IDENTIFIANT_METADONNEE, "métadonnée" },
	{ IDENTIFIANT_NOM, "nom" },
	{ IDENTIFIANT_ONGLET, "onglet" },
	{ IDENTIFIANT_PAS, "pas" },
	{ IDENTIFIANT_PRECISION, "précision" },
	{ IDENTIFIANT_QUAND, "quand" },
	{ IDENTIFIANT_RELATION, "relation" },
	{ IDENTIFIANT_RESULTAT, "résultat" },
	{ IDENTIFIANT_SORTIE, "sortie" },
	{ IDENTIFIANT_SEPARATEUR, "séparateur" },
	{ IDENTIFIANT_VALEUR, "valeur" },
	{ IDENTIFIANT_VECTEUR, "vecteur" },
	{ IDENTIFIANT_VRAI, "vrai" },
	{ IDENTIFIANT_ETIQUETTE, "étiquette" },
};

/* Liste générée par genere_donnees_mot_cle.py */
static PaireIndentifiantChaine paires_caracteres_double[] = {
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

/* Liste générée par genere_donnees_mot_cle.py */
static PaireIndentifiantCaractere paires_identifiant_caractere[] = {
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
	{ IDENTIFIANT_TILDE, '~' },
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
				throw ErreurFrappe("", 0, 0, "Erreur ! Le nombre contient un point en trop !\n");
			}

			etat = ETAT_NOMBRE_POINT;
		}

		chaine.push_back(*debut++);
		++compte;
	}

	return compte;
}

/* ************************************************************************** */

size_t trouve_taille_ligne(const std::string_view &chaine, size_t pos)
{
	const auto debut = pos;

	while (pos < chaine.size() && chaine[pos] != '\n') {
		++pos;
	}

	return pos - debut;
}

Decoupeur::Decoupeur(const std::string_view &chaine)
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

void Decoupeur::impression_debogage(const std::string &quoi)
{
	std::cout << "Trouvé symbole " << quoi
			  << ", ligne : " << m_ligne
			  << ", position : " << m_position_ligne << ".\n";
}

void Decoupeur::avance(int compte)
{
	m_position += compte;
	m_debut += compte;
	m_position_ligne += compte;
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
void Decoupeur::decoupe()
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
				if (*(m_debut + 1) == '.') {
					if (*(m_debut + 2) != '.') {
						throw ErreurFrappe(m_lignes[m_ligne - 1],
										   m_ligne,
										   m_position_ligne,
										   "Erreur : un point est manquant ou un point est en trop !\n");
					}

					LOG << "Découpe trois point.\n";

					ajoute_identifiant(IDENTIFIANT_TROIS_POINT, "...");
					mot_courant = "";
					avance(3);
				}
				else if (est_nombre(*(m_debut + 1))) {
					LOG << "Découpe nombre commentaire par un point.\n";
					int compte = extrait_nombre(m_debut, m_fin, mot_courant);
					avance(compte);
					ajoute_identifiant(IDENTIFIANT_NOMBRE, mot_courant);
					mot_courant = "";
				}
				else {
					throw ErreurFrappe(m_lignes[m_ligne - 1],
										 m_ligne,
										 m_position_ligne,
										 "Erreur : point innatendu !\n");
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
			else if (*m_debut == '#') {
				LOG << "Découpe commentaire.\n";
				// ignore commentaire
				while (*m_debut != '\n') {
					avance();
				}
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
			int compte = extrait_nombre(m_debut, m_fin, mot_courant);
			avance(compte);
			ajoute_identifiant(IDENTIFIANT_NOMBRE, mot_courant);
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

void Decoupeur::ajoute_identifiant(int identifiant, const std::string &contenu)
{
	m_identifiants.push_back({identifiant, m_ligne, m_position_ligne, contenu, m_lignes[m_ligne - 1]});
}

const std::vector<DonneesMorceaux> &Decoupeur::morceaux() const
{
	return m_identifiants;
}

}  /* namespace kangao */
