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

#include "operatrices_volume.hh"

#include <random>

#include "bibliotheques/outils/definitions.hh"

#include "../corps/volume.hh"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceAdvectionVolume;
class OperatriceConditionLimite;
class OperatriceAjoutFlottabilite;
class OperatriceResolutionPression;

/* ************************************************************************** */

class OperatriceCreationVolume : public OperatriceCorps {
public:
	static constexpr auto NOM = "Créer volume";
	static constexpr auto AIDE = "";

	explicit OperatriceCreationVolume(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		INUTILISE(rectangle);
		INUTILISE(temps);

		m_corps.reinitialise();

		std::mt19937 rng(19937);
		std::uniform_real_distribution dist(0.0f, 1.0f);

		auto volume = new Volume();
		auto grille_scalaire = new Grille<float>;
		grille_scalaire->initialise(32, 32, 32);

		for (size_t x = 0; x < 32; ++x) {
			for (size_t y = 0; y < 32; ++y) {
				for (size_t z = 0; z < 32; ++z) {
					grille_scalaire->valeur(x, y, z, dist(rng));
				}
			}
		}

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceMaillageVersVolume : public OperatriceCorps {
public:
	static constexpr auto NOM = "Maillage vers Volume";
	static constexpr auto AIDE = "";

	explicit OperatriceMaillageVersVolume(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		auto const taille_voxel = 0.1;

		m_corps.reinitialise();

		auto corps = entree(0)->requiers_corps(rectangle, temps);

		if (corps == nullptr) {
			ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps->prims();

		if (prims->taille() == 0) {
			ajoute_avertissement("Aucune primitive dans le corps !");
			return EXECUTION_ECHOUEE;
		}

		/* calcul boite englobante */
		auto min = dls::math::vec3f(std::numeric_limits<float>::max());
		auto max = dls::math::vec3f(std::numeric_limits<float>::min());

		auto liste_points = corps->points();

		for (auto i = 0; i < liste_points->taille(); ++i) {
			auto point = liste_points->point(i);

			for (size_t j = 0; j < 3; ++j) {
				if (point[j] < min[j]) {
					min[j] = point[j];
				}
				else if (point[j] > max[j]) {
					max[j] = point[j];
				}
			}
		}

		auto dim = max - min;

		auto dim_d = corps->transformation(dls::math::point3d(dim));
		auto res_x = static_cast<size_t>(dim_d.x / taille_voxel);
		auto res_y = static_cast<size_t>(dim_d.y / taille_voxel);
		auto res_z = static_cast<size_t>(dim_d.z / taille_voxel);

		std::mt19937 rng(19937);
		std::uniform_real_distribution dist(0.0f, 1.0f);

		auto volume = new Volume();
		auto grille_scalaire = new Grille<float>;
		grille_scalaire->initialise(res_x, res_y, res_z);

		for (size_t x = 0; x < res_x; ++x) {
			for (size_t y = 0; y < res_y; ++y) {
				for (size_t z = 0; z < res_z; ++z) {
					grille_scalaire->valeur(x, y, z, dist(rng));
				}
			}
		}

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);
		m_corps.transformation = corps->transformation;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_volume(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationVolume>());
	usine.enregistre_type(cree_desc<OperatriceMaillageVersVolume>());
}

#pragma clang diagnostic pop
