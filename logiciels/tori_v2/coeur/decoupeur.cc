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

namespace langage {

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

static PaireIndentifiantChaine paires_identifiant[] = {
	{ IDENTIFIANT_IMPRIME, "imprime" },
	{ IDENTIFIANT_EXPRIME, "exprime" },
	{ IDENTIFIANT_CHRONOMETRE, "chronomètre" },
	{ IDENTIFIANT_TEMPS, "temps" },
	{ IDENTIFIANT_FONCTION, "fonction" },
	{ IDENTIFIANT_RETOURNE, "retourne" },
	{ IDENTIFIANT_EGALITE, "==" },
	{ IDENTIFIANT_INEGALITE, "!=" },
	{ IDENTIFIANT_INFERIEUR_EGAL, "<=" },
	{ IDENTIFIANT_SUPERIEUR_EGAL, ">=" },
	{ IDENTIFIANT_VRAI, "vrai" },
	{ IDENTIFIANT_FAUX, "faux" },
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

static PaireIndentifiantCaractere paires_identifiant_caractere[] = {
	{ IDENTIFIANT_ACCOLADE_OUVERTE, '{' },
	{ IDENTIFIANT_ACCOLADE_FERMEE, '}' },
	{ IDENTIFIANT_PARENTHESE_OUVERTE, '(' },
	{ IDENTIFIANT_PARENTHESE_FERMEE, ')' },
	{ IDENTIFIANT_CROCHET_OUVERT, '[' },
	{ IDENTIFIANT_CROCHET_FERME, ']' },
	{ IDENTIFIANT_EGAL, '=' },
	{ IDENTIFIANT_POINT_VIRGULE, ';' },
	{ IDENTIFIANT_VIRGULE, ',' },
	{ IDENTIFIANT_ADDITION, '+' },
	{ IDENTIFIANT_SOUSTRACTION, '-' },
	{ IDENTIFIANT_DIVISION, '/' },
	{ IDENTIFIANT_MULTIPLICATION, '*' },
	{ IDENTIFIANT_NON, '~' },
	{ IDENTIFIANT_ET, '&' },
	{ IDENTIFIANT_OU, '|' },
	{ IDENTIFIANT_OUX, '^' },
	{ IDENTIFIANT_INFERIEUR, '<' },
	{ IDENTIFIANT_SUPERIEUR, '>' },
	{ IDENTIFIANT_GUILLEMET, '"' },
	{ IDENTIFIANT_NOUVELLE_LIGNE, '\n' },
	{ IDENTIFIANT_DOUBLE_POINT, ':' },
};

template <typename Iterateur, typename T, typename TypeComparaison>
Iterateur recherche_binaire(
		Iterateur debut,
		Iterateur fin,
		const T &valeur,
		const TypeComparaison &comparaison)
{
	auto iterateur = std::lower_bound(debut, fin, valeur, comparaison);

	if (iterateur != fin && !(*iterateur < valeur)) {
		return iterateur;
	}

	return fin;
}

static int trouve_identifiant(const std::string &chaine)
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

	return IDENTIFIANT_NUL;
}

static int trouve_identifiant_caractere(const char caractere)
{
	auto iterateur = std::lower_bound(
				std::begin(paires_identifiant_caractere),
				std::end(paires_identifiant_caractere),
				PaireIndentifiantCaractere{IDENTIFIANT_NUL, caractere},
				comparaison_paires_identifiant_caractere);

	if (iterateur != std::end(paires_identifiant_caractere)) {
		if ((*iterateur).caractere == caractere) {
			return (*iterateur).identifiant;
		}
	}

	return IDENTIFIANT_NUL;
}

static bool est_nombre(const char caractere)
{
	return (caractere >= '0' && caractere <= '9') || caractere == '.';
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
	std::sort(std::begin(paires_identifiant),
			  std::end(paires_identifiant),
			  comparaison_paires_identifiant);

	std::sort(std::begin(paires_identifiant_caractere),
			  std::end(paires_identifiant_caractere),
			  comparaison_paires_identifiant_caractere);

	m_lignes.push_back({&m_chaine[m_position],
						trouve_taille_ligne(m_chaine, m_position)});
}

void Decoupeur::impression_debogage(const std::string &quoi)
{
	std::cout << "Trouvé symbole " << quoi
			  << ", ligne : " << m_ligne
			  << ", position : " << m_position_ligne << ".\n";
}

void Decoupeur::saute_espaces_blancs()
{
	auto caractere = m_chaine[m_position];

	while (caractere == ' ' || caractere == '\t') {
		++m_position;
		++m_position_ligne;

		if (m_position == m_chaine.size()) {
			m_fini = true;
			break;
		}

		caractere = m_chaine[m_position];
	}

	if (caractere == '\n') {
		m_lignes.push_back({&m_chaine[m_position + 1],
							trouve_taille_ligne(m_chaine, m_position + 1)});

		++m_ligne;
		m_position_ligne = 0;
	}
}

char Decoupeur::regarde_caractere_suivant()
{
	if (m_position >= m_chaine.size()) {
		return '\0';
	}

	return m_chaine[m_position];
}

char Decoupeur::caractere_suivant()
{
	m_caractere_courant = m_chaine[m_position++];
	++m_position_ligne;

	if (m_position == m_chaine.size()) {
		m_fini = true;
	}

	return m_caractere_courant;
}

char Decoupeur::caractere_courant()
{
	return m_caractere_courant;
}

std::string Decoupeur::decoupe_chaine_litterale()
{
	std::string mot_courant;

	while (caractere_suivant() != '"') {
		mot_courant.push_back(caractere_courant());
	}

	return mot_courant;
}

std::string Decoupeur::decoupe_nombre()
{
	std::string mot_courant;

	while (est_nombre(caractere_courant())) {
		mot_courant.push_back(caractere_courant());

		if (!est_nombre(regarde_caractere_suivant())) {
			break;
		}

		caractere_suivant();
	}

	return mot_courant;
}

void Decoupeur::decoupe()
{
#ifdef DEBOGUE_DECOUPEUR
	std::cout << "Démarrage de l'analyse d'une chaîne de "
			  << m_chaine.size() << " caractères.\n";
#endif

	std::string mot_courant;
	m_ligne = 0;
	m_position = 0;
	m_position_ligne = 0;
	int identifiant = IDENTIFIANT_NUL;

	while (!m_fini) {
		saute_espaces_blancs();
		const auto caractere = caractere_suivant();

		identifiant = trouve_identifiant_caractere(caractere);

		if (identifiant != IDENTIFIANT_NUL) {
			if (!mot_courant.empty()) {
				ajoute_identifiant(
							IDENTIFIANT_CHAINE_CARACTERE,
							m_lignes[m_ligne],
							m_ligne,
							m_position_ligne,
							mot_courant);

				mot_courant = "";
			}

			if (identifiant == IDENTIFIANT_GUILLEMET) {
				mot_courant = decoupe_chaine_litterale();
				identifiant = IDENTIFIANT_CHAINE_CARACTERE;
			}
			else {
				mot_courant.push_back(caractere);
			}
		}
		else if (caractere >= '0' && caractere <= '9') {
			if (!mot_courant.empty()) {
				throw ErreurFrappe(
							m_lignes[m_ligne],
							m_ligne,
							m_position_ligne,
							"Identifiant inconnu !");
			}

			mot_courant = decoupe_nombre();
			identifiant = IDENTIFIANT_NOMBRE;
		}
		else {
			mot_courant.push_back(caractere);
			identifiant = trouve_identifiant(mot_courant);
		}

		if (identifiant != IDENTIFIANT_NUL) {
#ifdef DEBOGUE_DECOUPEUR
			impression_debogage(mot_courant);
#endif

			ajoute_identifiant(
						identifiant,
						m_lignes[m_ligne],
						m_ligne,
						m_position_ligne,
						mot_courant);

			mot_courant = "";
		}
	}

#ifdef DEBOGUE_DECOUPEUR
	std::cout << "Fin de l'analyse. Ligne : " << m_ligne
			  << ". Position : " << m_position << ".\n";
#endif

	if (!mot_courant.empty()) {
		throw ErreurFrappe(
					m_lignes[m_ligne],
					m_ligne,
					m_position_ligne,
					"Le script ne semble pas terminé !");
	}
}

void Decoupeur::ajoute_identifiant(int identifiant, const std::string_view &ligne, int numero_ligne, int position_ligne, const std::string &contenu)
{
	m_identifiants.push_back({identifiant, numero_ligne, position_ligne, contenu, ligne});
}

const std::vector<DonneesMorceaux> &Decoupeur::morceaux() const
{
	return m_identifiants;
}

}  /* namespace langage */
