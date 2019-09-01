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

#include "operatrices_terrain.hh"

#include "biblinternes/bruit/evaluation.hh"
#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/outils/constantes.h"

#include "coeur/donnees_aval.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "wolika/echantillonnage.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

/**
 * Opératrices à considérer : min, max, ajuste (valeur * facteur + decalage | option inverse),
 * step (hermite2), gaussien, ajoute, multiplie, divise, soustrait,
 * random (bruit blanc), égale (blanc si vrai, noir sinon), puissance
 *
 * La plupart d'entre elles ne sont que des opérations sur un seul point, donc
 * peut-être qu'un graphe détail suffira?
 *
 * Voir aussi :
 * A.N.T. de Blender
 * WorldMachine
 * Vue
 */

struct Terrain {
	wlk::grille_dense_2d<float> hauteur{};
	wlk::grille_dense_2d<dls::math::vec3f> normal{};
};

static inline auto extrait_terrain(DonneesAval *da)
{
	return std::any_cast<Terrain *>(da->table["terrain"]);
}

// OpRecréeNormaux ou option à la fin de chaque opératrice
//vec3 compute_normal(vec3 pos, vec3 neighbor_pos) {
//    vec3 neighbor_vec = neighbor_pos-pos;
//    vec3 perp = cross(neighbor_vec, vec3(0.0, 1.0, 0.0));
//    return normalize(cross(neighbor_vec, perp));
//}
//vec3 get_normal(vec3 p) {
//    float x = offsets.x;
//    float z = offsets.y;
//    // TODO maybe make this options, all models for normal calculation are interesting
//    /*
//    return normalize((
//        compute_normal(p, get(vec2(p.x+x, p.z+z))) +
//        compute_normal(p, get(vec2(p.x-x, p.z-z))) +
//        compute_normal(p, get(vec2(p.x+x, p.z-z))) +
//        compute_normal(p, get(vec2(p.x-x, p.z+z))) +
//        compute_normal(p, get(vec2(p.x, p.z+z))) +
//        compute_normal(p, get(vec2(p.x, p.z-z))) +
//        compute_normal(p, get(vec2(p.x+x, p.z))) +
//        compute_normal(p, get(vec2(p.x-x, p.z)))
//    )/8.0);
//    return normalize((
//        compute_normal(p, get(vec2(p.x, p.z+z))) +
//        compute_normal(p, get(vec2(p.x, p.z-z))) +
//        compute_normal(p, get(vec2(p.x+x, p.z))) +
//        compute_normal(p, get(vec2(p.x-x, p.z)))
//    )/4.0);
//    */
//    return normalize(vec3(
//        get(vec2(p.x-x, p.z)).y - get(vec2(p.x+x, p.z)).y,
//        x+z,
//        get(vec2(p.x, p.z-z)).y - get(vec2(p.x, p.z+z)).y
//    ));
//}

static auto calcul_normal(
		wlk::grille_dense_2d<float> const &grille,
		dls::math::vec2f const &p)
{
	auto offsets = dls::math::vec2f(1.0f) / dls::math::vec2f(200.0f);
	float x = offsets.x;
	float y = offsets.y;

	auto hx0 = wlk::echantillonne_lineaire(grille, p.x - x, p.y);
	auto hx1 = wlk::echantillonne_lineaire(grille, p.x + x, p.y);

	auto hy0 = wlk::echantillonne_lineaire(grille, p.x, p.y - y);
	auto hy1 = wlk::echantillonne_lineaire(grille, p.x, p.y - y);

	return normalise(dls::math::vec3f(hx0 - hx1, x+y, hy0 - hy1));
}

/* ************************************************************************** */

class OpVentTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Vent Terrain";
	static constexpr auto AIDE = "";

	OpVentTerrain(Graphe &graphe_parent, Noeud &noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
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

		if (!donnees_aval || !donnees_aval->possede("terrain")) {
			this->ajoute_avertissement("Il n'y a pas de terrain en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto temp = wlk::grille_dense_2d<float>(desc);

		auto direction = 0.0f; // evalue_decimal("direction", contexte.temps_courant);
		auto force = 0.5f;  // evalue_decimal("force", contexte.temps_courant);
		auto force2 = 0.5f;  // evalue_decimal("force2", contexte.temps_courant);
		auto repetitions = 1; // evalue_decimal("repetitions", contexte.temps_courant);

		force *= 1.35f;
		force2 *= 2.0f;

		auto dir = direction * constantes<float>::TAU;

		for (auto r = 0; r < repetitions; ++r) {
			copie_donnees_calque(terrain->hauteur, temp);

			auto index = 0;
			for (auto y = 0; y < desc.resolution.y; ++y) {
				for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
					auto pos_monde = terrain->hauteur.index_vers_unit(dls::math::vec2i(x, y));

					auto h = terrain->hauteur.valeur(index);

					auto poids = 0.5f; // poids_filtre.valeur(index);

					auto s = std::cos(dir);
					auto t = std::cos(dir + constantes<float>::PI / 2.0f);

					auto low = wlk::echantillonne_lineaire(temp, pos_monde.x + s, pos_monde.y + t);
					low = std::min(low, h);

					s = std::cos(dir - constantes<float>::PI);
					t = std::cos(dir - constantes<float>::PI / 2.0f);

					auto high = wlk::echantillonne_lineaire(temp, pos_monde.x + s, pos_monde.y + t);

					auto normal = calcul_normal(temp, pos_monde);
					auto facteur = normal.y; //produit_scalaire(normal, dls::math::vec3f(0.0f, 1.0f, 0.0f);
					auto hauteur = dls::math::entrepolation_lineaire(h, low, facteur * force2);
					hauteur += dls::math::entrepolation_lineaire(h, high, std::acos(facteur) * force);
					hauteur *= 0.5f;

					hauteur = dls::math::entrepolation_lineaire(hauteur, h, dls::math::restreint(poids, 0.0f, 1.0f));

					terrain->hauteur.valeur(index, hauteur);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpErosionTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Érosion Terrain";
	static constexpr auto AIDE = "";

	OpErosionTerrain(Graphe &graphe_parent, Noeud &noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
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

		if (!donnees_aval || !donnees_aval->possede("terrain")) {
			this->ajoute_avertissement("Il n'y a pas de terrain en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto temp = wlk::grille_dense_2d<float>(desc);

		auto repetitions = 1; // evalue_decimal("repetitions", contexte.temps_courant);
		auto invert = false;
		auto shallow = false;
		auto rough = false;
		auto slope = true;

		auto offsets = dls::math::vec2f(1.0f) / dls::math::vec2f(200.0f);

		for (auto r = 0; r < repetitions; ++r) {
			copie_donnees_calque(terrain->hauteur, temp);

			auto index = 0;
			for (auto y = 0; y < desc.resolution.y; ++y) {
				for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
					auto pos_monde = terrain->hauteur.index_vers_unit(dls::math::vec2i(x, y));

					auto uv = pos_monde;
					auto s = offsets.x;
					auto t = offsets.y;

					float weight = 0.5f; // texture2D(filter_weight, uv).r;
					auto pos = wlk::echantillonne_lineaire(temp, uv.x, uv.y);
					auto left = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y);
					auto right = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y);
					auto top = wlk::echantillonne_lineaire(temp, uv.x, uv.y + t);
					auto bottom = wlk::echantillonne_lineaire(temp, uv.x, uv.y - t);
					auto left_top = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y + t);
					auto right_top = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y + t);
					auto left_bottom = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y - t);
					auto right_bottom = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y - t);

					auto a = dls::math::vec4f(left, right, top, bottom);
					auto b = dls::math::vec4f(left_top, right_top, left_bottom, right_bottom);

					float count = 1.0f;
					float sum = pos;
					float result;

					if (invert) {
						for (auto i = 0u; i < 4; ++i) {
							if (a[i] > pos) {
								count += 1.0f;
								sum += a[i];
							}
						}

						if (!rough) {
							for (auto i = 0u; i < 4; ++i) {
								if (b[i] > pos) {
									count += 1.0f;
									sum += b[i];
								}
							}
						}
					}
					else {
						for (auto i = 0u; i < 4; ++i) {
							if (a[i] < pos) {
								count += 1.0f;
								sum += a[i];
							}
						}

						if (!rough) {
							for (auto i = 0u; i < 4; ++i) {
								if (b[i] < pos) {
									count += 1.0f;
									sum += b[i];
								}
							}
						}
					}

					if (slope) {
						auto normal = normalise(dls::math::vec3f(
													left - right,
													s+t,
													bottom - top));

						float factor = normal.y; // normal . up

						if (shallow) {
							factor = 1.0f - factor;
						}
						else {
							factor = factor - 0.05f * count;
						}

						result = dls::math::entrepolation_lineaire(sum/count, pos, factor);
					}
					else {
						result = sum/count;
					}

					result = dls::math::entrepolation_lineaire(result, pos, dls::math::restreint(weight, 0.0f, 1.0f));

					terrain->hauteur.valeur(index, result);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpInclineTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Incline Terrain";
	static constexpr auto AIDE = "";

	OpInclineTerrain(Graphe &graphe_parent, Noeud &noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
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

		if (!donnees_aval || !donnees_aval->possede("terrain")) {
			this->ajoute_avertissement("Il n'y a pas de terrain en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto temp = wlk::grille_dense_2d<float>(desc);
		copie_donnees_calque(terrain->hauteur, temp);

		auto facteur = 0.5f; // evalue_decimal("facteur", contexte.temps_courant);
		auto decalage = 0.5f;  // evalue_decimal("decalage", contexte.temps_courant);
		auto inverse = false;  // evalue_bool("inverse", contexte.temps_courant);

//		facteur = std::pow(facteur * 2.0f, 10.0f);
//		decalage = (decalage - 0.5f) * 10.0f;

		//auto repetition = 1;

		auto index = 0;
		for (auto y = 0; y < desc.resolution.y; ++y) {
			for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
				auto pos_monde = terrain->hauteur.index_vers_unit(dls::math::vec2i(x, y));

				auto normal = calcul_normal(temp, pos_monde);
				auto resultat = normal.y; //produit_scalaire(normal, dls::math::vec3f(0.0f, 1.0f, 0.0f);
				resultat = resultat * facteur + decalage;

				if (inverse) {
					resultat = (1.0f - resultat);
				}

				terrain->hauteur.valeur(index, resultat);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto desc_depuis_hauteur_largeur(int hauteur, int largeur)
{
	auto moitie_x = static_cast<float>(largeur) * 0.5f;
	auto moitie_y = static_cast<float>(hauteur) * 0.5f;

	auto desc = wlk::desc_grille_2d();
	desc.etendue.min = dls::math::vec2f(-moitie_x, -moitie_y);
	desc.etendue.max = dls::math::vec2f( moitie_x,  moitie_y);
	desc.fenetre_donnees = desc.etendue;
	desc.taille_voxel = 1.0;

	return desc;
}

class OpCreeTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Crée Terrain";
	static constexpr auto AIDE = "";

	OpCreeTerrain(Graphe &graphe_parent, Noeud &noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto terrain = Terrain();

		auto da = DonneesAval();
		da.table.insere({ "terrain", &terrain });

		auto desc = desc_depuis_hauteur_largeur(200, 200);

		terrain.hauteur = wlk::grille_dense_2d<float>(desc);

		desc = terrain.hauteur.desc();

		auto graine = 0;
		auto params = bruit::parametres();
		params.taille_bruit = dls::math::vec3f(0.1f, 0.1f, 1.0f);
		params.type_bruit = bruit::type::SIMPLEX;

		bruit::simplex::construit(params, graine);

		auto index = 0;
		for (auto y = 0; y < desc.resolution.y; ++y) {
			for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
				auto xf = static_cast<float>(x) / 200.0f;
				auto yf = static_cast<float>(y) / 200.0f;
				auto valeur = bruit::evalue(params, dls::math::vec3f(xf, yf, 0.0f));

				terrain.hauteur.valeur(index, valeur * 100.0f);
			}
		}

		entree(0)->requiers_corps(contexte, &da);

		index = 0;
		for (auto y = 0; y < desc.resolution.y; ++y) {
			for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
				auto pos_monde = terrain.hauteur.index_vers_monde(dls::math::vec2i(x, y));

				auto h = terrain.hauteur.valeur(index);

				m_corps.ajoute_point(pos_monde.x, h, pos_monde.y);

				if (x < desc.resolution.x - 1 && y < desc.resolution.y - 1) {
					auto poly = m_corps.ajoute_polygone(type_polygone::FERME, 4);

					m_corps.ajoute_sommet(poly, index);
					m_corps.ajoute_sommet(poly, index + 1);
					m_corps.ajoute_sommet(poly, index + 1 + desc.resolution.x);
					m_corps.ajoute_sommet(poly, index + desc.resolution.x);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_terrain(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpCreeTerrain>());
	usine.enregistre_type(cree_desc<OpErosionTerrain>());
	usine.enregistre_type(cree_desc<OpInclineTerrain>());
	usine.enregistre_type(cree_desc<OpVentTerrain>());
}

#pragma clang diagnostic pop
