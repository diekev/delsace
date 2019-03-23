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

#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/outils/gna.hh"
#include "bibliotheques/outils/parallelisme.h"

#include "bibloc/logeuse_memoire.hh"

#include "../corps/collision.hh"
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		auto gna = GNA();

		auto volume = memoire::loge<Volume>();
		auto grille_scalaire = memoire::loge<Grille<float>>();
		grille_scalaire->initialise(32, 32, 32);

		for (size_t x = 0; x < 32; ++x) {
			for (size_t y = 0; y < 32; ++y) {
				for (size_t z = 0; z < 32; ++z) {
					grille_scalaire->valeur(x, y, z, gna.uniforme(0.0f, 1.0f));
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
		return "entreface/operatrice_maillage_vers_volume.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto corps = entree(0)->requiers_corps(contexte, donnees_aval);

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

		auto const taille_voxel = evalue_decimal("taille_voxel");
		auto const densite = evalue_decimal("densité");

		auto dim_d = corps->transformation(dls::math::point3d(dim));
		auto res_x = static_cast<size_t>(dim_d.x / static_cast<double>(taille_voxel));
		auto res_y = static_cast<size_t>(dim_d.y / static_cast<double>(taille_voxel));
		auto res_z = static_cast<size_t>(dim_d.z / static_cast<double>(taille_voxel));

		auto volume = memoire::loge<Volume>();
		auto grille_scalaire = memoire::loge<Grille<float>>();
		grille_scalaire->initialise(res_x, res_y, res_z);
		grille_scalaire->min = min;
		grille_scalaire->max = max;
		grille_scalaire->dim = dim;

		boucle_parallele(tbb::blocked_range<size_t>(0, res_x),
						 [&](tbb::blocked_range<size_t> const &plage)
		{
			for (size_t x = plage.begin(); x < plage.end(); ++x) {
				for (size_t y = 0; y < res_y; ++y) {
					for (size_t z = 0; z < res_z; ++z) {
						/* coordonnées objet */
						auto const x_obj = static_cast<float>(x) / static_cast<float>(res_x);
						auto const y_obj = static_cast<float>(y) / static_cast<float>(res_y);
						auto const z_obj = static_cast<float>(z) / static_cast<float>(res_z);

						/* coordonnées mondiales */
						auto const x_mond = x_obj * dim.x + min.x;
						auto const y_mond = y_obj * dim.y + min.y;
						auto const z_mond = z_obj * dim.z + min.z;

						auto rayon = Rayon{};
						rayon.origine = dls::math::vec3f(x_mond, y_mond, z_mond);

						auto axis = axe_dominant_abs(rayon.origine);

						rayon.direction = dls::math::vec3f(0.0f, 0.0f, 0.0f);
						rayon.direction[axis] = 1.0f;

						auto distance = dim.x * 0.5f;

						auto index_prim = cherche_collision(corps, rayon, distance);

						if (index_prim < 0) {
							continue;
						}

						auto prim_coll = corps->prims()->prim(index_prim);

						/* calcul normal au niveau de la prim */
						auto poly = dynamic_cast<Polygone *>(prim_coll);
						auto const &v0 = liste_points->point(poly->index_point(0));
						auto const &v1 = liste_points->point(poly->index_point(1));
						auto const &v2 = liste_points->point(poly->index_point(2));

						auto const e1 = v1 - v0;
						auto const e2 = v2 - v0;
						auto nor_poly = normalise(produit_croix(e1, e2));

						//grille_scalaire->valeur(x, y, z, dist(rng));
						if (produit_scalaire(nor_poly, rayon.direction) < 0.0f) {
							continue;
						}

						rayon.direction = -rayon.direction;
						distance = dim.x * 0.5f;

						if (cherche_collision(corps, rayon, distance) != -1) {
							grille_scalaire->valeur(x, y, z, densite);
						}
					}
				}
			}
		});

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

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
