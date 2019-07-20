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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "reseau_neuronal.h"

#include "../math/matrice/operations.hh"

#include <random>

#define INSCRIPTION_JOURNAL

#ifdef INSCRIPTION_JOURNAL
#	define INSCRIT_JOURNAL std::cout
#else

struct flux_vide {};

template <typename T>
flux_vide &operator<<(flux_vide &flux, const T &)
{
	return flux;
}

flux_vide flux_global;

#define INSCRIT_JOURNAL flux_global

#endif

static Matrice active(const Matrice &matrice, TypeActivation activation)
{
	auto resultat = matrice;

	switch (activation) {
		default:
		case TypeActivation::AUCUNE:
			break;
		case TypeActivation::ULRE:
			dls::math::applique_fonction(resultat, [](const double valeur)
			{
				return std::max(0.0, valeur);
			});
			break;
	}

	return resultat;
}

Matrice CoucheReseau::traduit(const Matrice &matrice)
{
	INSCRIT_JOURNAL << "Début traduction de la matrice d'entrée....\n";

	neurones = matrice;
	auto resultat = neurones;
	auto ceci = this;
	auto couche_suivante = suivante;

	while (couche_suivante != nullptr) {
		INSCRIT_JOURNAL << "\tTaille resultat avant multiplication : "
						<< resultat.nombre_lignes() << 'x'
						<< resultat.nombre_colonnes() << ".\n";

		INSCRIT_JOURNAL << "\tTaille matrice poids : "
						<< ceci->poids.nombre_lignes() << 'x'
						<< ceci->poids.nombre_colonnes() << ".\n";

		resultat *= ceci->poids;

		INSCRIT_JOURNAL << "\tTaille resultat après multiplicatoin : "
						<< resultat.nombre_lignes() << 'x'
						<< resultat.nombre_colonnes() << ".\n";

		resultat += couche_suivante->biais;

		INSCRIT_JOURNAL << "\tTaille resultat après addition : "
						<< resultat.nombre_lignes() << 'x'
						<< resultat.nombre_colonnes() << ".\n";

		resultat = active(resultat, couche_suivante->activation);

		INSCRIT_JOURNAL << "\tTaille resultat après activation : "
						<< resultat.nombre_lignes() << 'x'
						<< resultat.nombre_colonnes() << ".\n";

		couche_suivante->neurones = resultat;

		INSCRIT_JOURNAL << "\tPassage à la couche suivante...\n";

		ceci = couche_suivante;
		couche_suivante = couche_suivante->suivante;
	}

	INSCRIT_JOURNAL << "Fin traduction de la matrice d'entrée....\n";

	return resultat;
}

ReseauNeuronal::~ReseauNeuronal()
{
	for (auto &couche : m_couches) {
		delete couche;
	}
}

CoucheReseau *ReseauNeuronal::ajoute_entree(int taille_entree)
{
	return ajoute_couche(nullptr, taille_entree, TypeActivation::AUCUNE);
}

CoucheReseau *ReseauNeuronal::ajoute_couche(CoucheReseau *couche_precedente, int taille_couche, TypeActivation activation)
{
	INSCRIT_JOURNAL << "Création couche...\n";
	CoucheReseau *couche = new CoucheReseau;
	couche->precendente = couche_precedente;
	couche->suivante = nullptr;
	couche->activation = activation;
	couche->taille = taille_couche;

	couche->biais = Matrice(dls::math::Hauteur(1), dls::math::Largeur(taille_couche));
	couche->neurones = Matrice(dls::math::Hauteur(1), dls::math::Largeur(taille_couche));

	INSCRIT_JOURNAL << "\tCouche " << m_couches.taille() << " :\n";
	INSCRIT_JOURNAL << "\tTaille biais : 1x" << taille_couche << ".\n";
	INSCRIT_JOURNAL << "\tTaille neurones : 1x" << taille_couche << ".\n";

	if (couche_precedente != nullptr) {
		couche_precedente->suivante = couche;
	}

	m_couches.pousse(couche);

	INSCRIT_JOURNAL << "Création couche terminée.\n";
	return couche;
}

CoucheReseau *ReseauNeuronal::ajoute_sortie(CoucheReseau *couche_precedente, int taille_couche)
{
	return ajoute_couche(couche_precedente, taille_couche, TypeActivation::AUCUNE);
}

void ReseauNeuronal::compile()
{
	INSCRIT_JOURNAL << "Compilation réseau neuronal....\n";

	/* Création des matrices de poids. */
	for (auto i = 0l; i < m_couches.taille() - 1; ++i) {
		/* Le nombre de lignes de la matrice de poids est égal au nombre de colonnes de la matrice i. */
		const auto hauteur = m_couches[i]->taille;
		const auto largeur = m_couches[i + 1]->taille;

		m_couches[i]->poids = Matrice(dls::math::Hauteur(hauteur), dls::math::Largeur(largeur));

		INSCRIT_JOURNAL << "\tTaille matrice poids entre couche " << i << " et "
						<< i + 1 << " : " << hauteur << "x" << largeur << ".\n";
	}

	INSCRIT_JOURNAL << "Compilation réseau neuronal terminée.\n";
}

void ReseauNeuronal::initialise_couches(TypeInitialisation initialisation)
{
	switch (initialisation) {
		case TypeInitialisation::ZERO:
			for (auto i = 0l; i < m_couches.taille(); ++i) {
				m_couches[i]->poids.remplie(0);
				m_couches[i]->biais.remplie(0);
				m_couches[i]->neurones.remplie(0);
			}

			break;
		case TypeInitialisation::UNITE:
			for (auto i = 0l; i < m_couches.taille(); ++i) {
				m_couches[i]->poids.remplie(1);
				m_couches[i]->biais.remplie(1);
				m_couches[i]->neurones.remplie(1);
			}

			break;
		case TypeInitialisation::ALEATOIRE:
		{
			std::uniform_real_distribution<double> dist(-1.0, 1.0);
			std::mt19937 rng(19937);

			auto fonction = [&](const double)
			{
				return dist(rng);
			};

			for (auto i = 0l; i < m_couches.taille(); ++i) {
				dls::math::applique_fonction(m_couches[i]->poids, fonction);
				dls::math::applique_fonction(m_couches[i]->biais, fonction);
				dls::math::applique_fonction(m_couches[i]->neurones, fonction);
			}

			break;
		}
	}
}
