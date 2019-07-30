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

#include "arbre_hbe.hh"

static inline auto axe_depuis_direction(Axe const direction)
{
	if (direction == Axe::Y) {
		return 1u;
	}

	if (direction == Axe::Z) {
		return 2u;
	}

	return 0u;
}

double calcul_cout_scission(
		double const scission,
		const ArbreHBE::Noeud &noeud,
		dls::tableau<long> &references,
		Axe const direction,
		dls::tableau<BoiteEnglobante> const &boites_alignees,
		long &compte_gauche,
		long &compte_droite)
{
	auto const axe_scission = axe_depuis_direction(direction);

	auto boite_gauche = BoiteEnglobante{};
	auto gauche = 0;
	auto boite_droite = BoiteEnglobante{};
	auto droite = 0;

	/* Parcours les références du noeud et fusionne les dans 'gauche' ou 'droite' */
	for (auto i = 0; i < noeud.nombre_references; ++i) {
		auto const idx_ref = references[i];
		auto const &reference = boites_alignees[idx_ref];

		if (reference.centroide[axe_scission] <= scission) {
			gauche++;
			boite_gauche.etend(reference);
		}
		else {
			droite++;
			boite_droite.etend(reference);
		}
	}

	compte_gauche = gauche;
	compte_droite = droite;

	if (gauche == 0 || droite == 0) {
		return std::pow(10.0f, 100);
	}

	auto cout_gauche = gauche * boite_gauche.aire_surface();
	auto cout_droite = droite * boite_droite.aire_surface();

	return std::fabs(cout_gauche - cout_droite);
}

double trouve_meilleure_scission(
		ArbreHBE::Noeud &noeud,
		dls::tableau<long> &references,
		Axe const direction,
		unsigned int const qualite,
		dls::tableau<BoiteEnglobante> const &boites_alignees,
		long &compte_gauche,
		long &compte_droite)
{
	auto const axe_scission = axe_depuis_direction(direction);

	auto candidates = dls::tableau<double>(static_cast<long>(qualite));
	auto longueur_axe = noeud.limites.max[axe_scission] - noeud.limites.min[axe_scission];
	auto taille_scission = longueur_axe / static_cast<double>(qualite + 1);
	auto taille_courante = taille_scission;

	/* crée des scissions isométriques */
	for (auto &candidate : candidates) {
		candidate = noeud.limites.min[axe_scission] + taille_courante;
		taille_courante += taille_scission;
	}

	/* Calculons tous les coûts de scission pour ne garder que le plus petit. */
	auto gauche = 0l;
	auto droite = 0l;
	auto gauche_courante = 0l;
	auto drotie_courante = 0l;

	auto meilleur_cout = constantes<double>::INFINITE;
	auto meilleure_scission = constantes<double>::INFINITE;

	for (auto const &candidate : candidates) {
		auto cout = calcul_cout_scission(
					candidate,
					noeud,
					references,
					direction,
					boites_alignees,
					gauche_courante,
					drotie_courante);

		if (cout < meilleur_cout) {
			meilleur_cout = cout;
			meilleure_scission = candidate;
			gauche = gauche_courante;
			droite = drotie_courante;
		}
	}

	compte_gauche = gauche;
	compte_droite = droite;

	return meilleure_scission;
}
