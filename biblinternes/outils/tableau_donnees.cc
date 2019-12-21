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

#include "tableau_donnees.hh"

ChaineUTF8::ChaineUTF8(const char *chn)
	: chaine(chn)
{
	calcule_taille();
}

ChaineUTF8::ChaineUTF8(const dls::chaine &chn)
	: chaine(chn)
{
	calcule_taille();
}

long ChaineUTF8::taille() const
{
	return m_taille;
}

void ChaineUTF8::calcule_taille()
{
	for (auto i = 0l; i < chaine.taille(); ) {
		auto n = lng::nombre_octets(&chaine[i]);
		m_taille += 1;
		i += n;
	}
}

std::ostream &operator<<(std::ostream &os, const ChaineUTF8 &chaine)
{
	os << chaine.chaine;
	return os;
}

Tableau::Tableau(const std::initializer_list<ChaineUTF8> &titres)
	: nombre_colonnes(static_cast<long>(titres.size()))
{
	ajoute_ligne(titres);

	for (auto i = 0; i < nombre_colonnes; ++i) {
		alignements.pousse(Alignement::GAUCHE);
	}
}

void Tableau::alignement(int idx, Alignement a)
{
	alignements[idx] = a;
}

void Tableau::ajoute_ligne(const std::initializer_list<ChaineUTF8> &valeurs)
{
	assert(static_cast<long>(valeurs.size()) <= nombre_colonnes);

	auto ligne = Ligne{};

	for (auto const &valeur : valeurs) {
		ligne.colonnes.pousse(valeur);
	}

	lignes.pousse(ligne);
}

static void imprime_ligne_demarcation(dls::tableau<long> const &tailles_max_colonnes)
{
	for (auto i = 0; i < tailles_max_colonnes.taille(); ++i) {
		std::cout << '+' << '-';

		for (auto j = 0; j < tailles_max_colonnes[i]; ++j) {
			std::cout << '-';
		}

		std::cout << '-';
	}

	std::cout << '+' << '\n';
}

static void imprime_ligne(
		Tableau::Ligne const &ligne,
		dls::tableau<long> const &tailles_max_colonnes,
		dls::tableau<Alignement> const &alignements)
{
	for (auto i = 0; i < ligne.colonnes.taille(); ++i) {
		auto const &colonne = ligne.colonnes[i];

		std::cout << '|' << ' ';

		if (alignements[i] == Alignement::DROITE) {
			for (auto j = 0; j < tailles_max_colonnes[i] - colonne.taille(); ++j) {
				std::cout << ' ';
			}
		}

		std::cout << colonne;

		if (alignements[i] == Alignement::GAUCHE) {
			for (auto j = colonne.taille(); j < tailles_max_colonnes[i]; ++j) {
				std::cout << ' ';
			}
		}

		std::cout << ' ';
	}

	for (auto i = ligne.colonnes.taille(); i < tailles_max_colonnes.taille(); ++i) {
		std::cout << '|' << ' ';

		for (auto j = 0; j < tailles_max_colonnes[i]; ++j) {
			std::cout << ' ';
		}

		std::cout << ' ';
	}

	std::cout << '|' << '\n';
}

void imprime_tableau(Tableau &tableau)
{
	// pour chaque ligne, calcul la taille maximale de la colonne
	dls::tableau<long> tailles_max_colonnes{};
	long nombre_colonnes = 0;

	for (auto const &ligne : tableau.lignes) {
		nombre_colonnes = std::max(nombre_colonnes, ligne.colonnes.taille());
	}

	tailles_max_colonnes.redimensionne(nombre_colonnes);

	for (auto const &ligne : tableau.lignes) {
		for (auto i = 0; i < ligne.colonnes.taille(); ++i) {
			tailles_max_colonnes[i] = std::max(tailles_max_colonnes[i], ligne.colonnes[i].taille());
		}
	}

	/* ajout de marges */
	auto taille_ligne = 0l;
	for (auto const &taille : tailles_max_colonnes) {
		taille_ligne += taille + 2;
	}

	/* impression */
	imprime_ligne_demarcation(tailles_max_colonnes);
	imprime_ligne(tableau.lignes[0], tailles_max_colonnes, tableau.alignements);
	imprime_ligne_demarcation(tailles_max_colonnes);

	for (auto i = 1; i < tableau.lignes.taille(); ++i) {
		imprime_ligne(tableau.lignes[i], tailles_max_colonnes, tableau.alignements);
	}

	imprime_ligne_demarcation(tailles_max_colonnes);
}
