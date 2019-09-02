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
#include "biblinternes/structures/dico_fixe.hh"

#include "coeur/contexte_evaluation.hh"
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
 * Terragen
 * Vue
 * WorldMachine
 */

struct Terrain {
	wlk::grille_dense_2d<float> hauteur{};
	wlk::grille_dense_2d<dls::math::vec3f> normal{};
};

static inline auto extrait_terrain(DonneesAval *da)
{
	return std::any_cast<Terrain *>(da->table["terrain"]);
}

static auto calcul_normal(
		dls::math::vec3f const &pos,
		dls::math::vec3f const &pos_voisin)
{
	auto vec = pos_voisin - pos;
	auto perp = produit_croix(vec, dls::math::vec3f(0.0f, 1.0f, 0.0f));
	return normalise(produit_croix(vec, perp));
}

static auto echantillonne_position(
		wlk::grille_dense_2d<float> const &grille,
		dls::math::vec2f const &p)
{
	auto h = wlk::echantillonne_lineaire(grille, p.x, p.y);
	return dls::math::vec3f(p.x, h, p.y);
}

static auto calcul_normal4(
		wlk::grille_dense_2d<float> const &grille,
		dls::math::vec2f const &p)
{
	auto desc = grille.desc();
	auto const res_x = desc.resolution.x;
	auto const res_y = desc.resolution.y;

	auto const x = 1.0f / static_cast<float>(res_x);
	auto const y = 1.0f / static_cast<float>(res_y);

	auto pos = echantillonne_position(grille, p);

	dls::math::vec2f decalages[4] = {
		dls::math::vec2f(p.x, p.y + y),
		dls::math::vec2f(p.x, p.y - y),
		dls::math::vec2f(p.x + x, p.y),
		dls::math::vec2f(p.x - x, p.y),
	};

	auto normal = dls::math::vec3f();

	for (auto i = 0; i < 4; ++i) {
		normal += calcul_normal(pos, echantillonne_position(grille, decalages[i]));
	}

	return normalise(-normal);
}

static auto calcul_normal8(
		wlk::grille_dense_2d<float> const &grille,
		dls::math::vec2f const &p)
{
	auto desc = grille.desc();
	auto const res_x = desc.resolution.x;
	auto const res_y = desc.resolution.y;

	auto const x = 1.0f / static_cast<float>(res_x);
	auto const y = 1.0f / static_cast<float>(res_y);

	auto pos = echantillonne_position(grille, p);

	dls::math::vec2f decalages[8] = {
		dls::math::vec2f(p.x + x, p.y + y),
		dls::math::vec2f(p.x - x, p.y - y),
		dls::math::vec2f(p.x + x, p.y - y),
		dls::math::vec2f(p.x - x, p.y + y),
		dls::math::vec2f(p.x, p.y + y),
		dls::math::vec2f(p.x, p.y - y),
		dls::math::vec2f(p.x + x, p.y),
		dls::math::vec2f(p.x - x, p.y),
	};

	auto normal = dls::math::vec3f();

	for (auto i = 0; i < 8; ++i) {
		normal += calcul_normal(pos, echantillonne_position(grille, decalages[i]));
	}

	return normalise(-normal);
}

static auto calcul_normal(
		wlk::grille_dense_2d<float> const &grille,
		dls::math::vec2f const &p)
{
	auto desc = grille.desc();
	auto const res_x = desc.resolution.x;
	auto const res_y = desc.resolution.y;

	auto const x = 1.0f / static_cast<float>(res_x);
	auto const y = 1.0f / static_cast<float>(res_y);

	auto hx0 = wlk::echantillonne_lineaire(grille, p.x - x, p.y);
	auto hx1 = wlk::echantillonne_lineaire(grille, p.x + x, p.y);

	auto hy0 = wlk::echantillonne_lineaire(grille, p.x, p.y - y);
	auto hy1 = wlk::echantillonne_lineaire(grille, p.x, p.y + y);

	return normalise(dls::math::vec3f(hx0 - hx1, x + y, hy0 - hy1));
}

enum {
	NORMAUX_DIFF_CENTRE,
	NORMAUX_VOISINS_4,
	NORMAUX_VOISINS_8,
};

static void calcul_normaux(Terrain &terrain, int const ordre)
{
	auto desc = terrain.hauteur.desc();
	auto const &grille = terrain.hauteur;
	auto &normaux = terrain.normal;

	auto const res_x = desc.resolution.x;
	auto const res_y = desc.resolution.y;

	auto index = 0;
	for (auto y = 0; y < res_y; ++y) {
		for (auto x = 0; x < res_x; ++x, ++index) {
			auto pos = grille.index_vers_unit(dls::math::vec2i(x, y));

			switch (ordre) {
				case NORMAUX_DIFF_CENTRE:
				{
					auto normal = calcul_normal(grille, pos);
					normaux.valeur(index, normal);
					break;
				}
				case NORMAUX_VOISINS_4:
				{
					auto normal = calcul_normal4(grille, pos);
					normaux.valeur(index, normal);
					break;
				}
				case NORMAUX_VOISINS_8:
				{
					auto normal = calcul_normal8(grille, pos);
					normaux.valeur(index, normal);
					break;
				}
			}
		}
	}
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
		entrees(2);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_terrain_vent.jo";
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

		/* entrée 0 est celle pour la grille, nous la copions pour sauvegarder
		 * son état, car le terrain sera modifié peut-être modifié par la
		 * deuxième entrée */
		entree(0)->requiers_corps(contexte, donnees_aval);

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto grille_entree = wlk::grille_dense_2d<float>(desc);
		copie_donnees_calque(terrain->hauteur, grille_entree);

		auto grille_poids = static_cast<wlk::grille_dense_2d<float> *>(nullptr);

		if (entree(1)->connectee()) {
			entree(1)->requiers_corps(contexte, donnees_aval);

			grille_poids = &terrain->hauteur;
		}

		auto temp = wlk::grille_dense_2d<float>(desc);

		auto direction = evalue_decimal("direction", contexte.temps_courant);
		auto force = evalue_decimal("force1", contexte.temps_courant);
		auto force2 = evalue_decimal("force2", contexte.temps_courant);
		auto repetitions = evalue_entier("répétitions", contexte.temps_courant);

		force *= 1.35f;
		force2 *= 2.0f;

		auto dir = direction * constantes<float>::TAU;

		for (auto r = 0; r < repetitions; ++r) {
			copie_donnees_calque(grille_entree, temp);

			auto index = 0;
			for (auto y = 0; y < desc.resolution.y; ++y) {
				for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
					auto pos_monde = grille_entree.index_vers_unit(dls::math::vec2i(x, y));

					auto h = grille_entree.valeur(index);

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

					if (grille_poids != nullptr) {
						auto poids = grille_poids->valeur(index);
						hauteur = dls::math::entrepolation_lineaire(hauteur, h, dls::math::restreint(poids, 0.0f, 1.0f));
					}

					grille_entree.valeur(index, hauteur);
				}
			}
		}

		/* copie les données */
		copie_donnees_calque(grille_entree, terrain->hauteur);

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
		entrees(2);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_terrain_erosion.jo";
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

		/* entrée 0 est celle pour la grille, nous la copions pour sauvegarder
		 * son état, car le terrain sera modifié peut-être modifié par la
		 * deuxième entrée */
		entree(0)->requiers_corps(contexte, donnees_aval);

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto grille_entree = wlk::grille_dense_2d<float>(desc);
		copie_donnees_calque(terrain->hauteur, grille_entree);

		auto grille_poids = static_cast<wlk::grille_dense_2d<float> *>(nullptr);

		if (entree(1)->connectee()) {
			entree(1)->requiers_corps(contexte, donnees_aval);

			grille_poids = &terrain->hauteur;
		}

		auto temp = wlk::grille_dense_2d<float>(desc);

		auto repetitions = evalue_entier("répétitions", contexte.temps_courant);
		auto inverse = evalue_bool("inverse");
		auto superficielle = evalue_bool("superficielle");
		auto rugueux = evalue_bool("rugueux");
		auto pente = evalue_bool("pente");

		auto const res_x = desc.resolution.x;
		auto const res_y = desc.resolution.y;

		auto const s = 1.0f / static_cast<float>(res_x);
		auto const t = 1.0f / static_cast<float>(res_y);

		for (auto r = 0; r < repetitions; ++r) {
			copie_donnees_calque(grille_entree, temp);

			auto index = 0;
			for (auto y = 0; y < desc.resolution.y; ++y) {
				for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
					auto pos_monde = grille_entree.index_vers_unit(dls::math::vec2i(x, y));

					auto uv = pos_monde;

					auto centre      = wlk::echantillonne_lineaire(temp, uv.x, uv.y);
					auto gauche      = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y);
					auto droit       = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y);
					auto haut        = wlk::echantillonne_lineaire(temp, uv.x, uv.y + t);
					auto bas         = wlk::echantillonne_lineaire(temp, uv.x, uv.y - t);
					auto haut_gauche = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y + t);
					auto haut_droit  = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y + t);
					auto bas_gauche  = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y - t);
					auto bas_droit   = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y - t);

					auto a = dls::math::vec4f(gauche, droit, haut, bas);
					auto b = dls::math::vec4f(haut_gauche, haut_droit, bas_gauche, bas_droit);

					float count = 1.0f;
					float sum = centre;
					float result;

					if (inverse) {
						for (auto i = 0u; i < 4; ++i) {
							if (a[i] > centre) {
								count += 1.0f;
								sum += a[i];
							}
						}

						if (!rugueux) {
							for (auto i = 0u; i < 4; ++i) {
								if (b[i] > centre) {
									count += 1.0f;
									sum += b[i];
								}
							}
						}
					}
					else {
						for (auto i = 0u; i < 4; ++i) {
							if (a[i] < centre) {
								count += 1.0f;
								sum += a[i];
							}
						}

						if (!rugueux) {
							for (auto i = 0u; i < 4; ++i) {
								if (b[i] < centre) {
									count += 1.0f;
									sum += b[i];
								}
							}
						}
					}

					if (pente) {
						auto normal = normalise(dls::math::vec3f(
													gauche - droit,
													s+t,
													bas - haut));

						float factor = normal.y; // normal . up

						if (superficielle) {
							factor = 1.0f - factor;
						}
						else {
							factor = factor - 0.05f * count;
						}

						result = dls::math::entrepolation_lineaire(sum/count, centre, factor);
					}
					else {
						result = sum/count;
					}

					if (grille_poids != nullptr) {
						auto poids = grille_poids->valeur(index);
						result = dls::math::entrepolation_lineaire(result, centre, dls::math::restreint(poids, 0.0f, 1.0f));
					}

					grille_entree.valeur(index, result);
				}
			}
		}

		/* copie les données */
		copie_donnees_calque(grille_entree, terrain->hauteur);

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
		return "entreface/operatrice_terrain_inclinaison.jo";
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

		auto facteur = evalue_decimal("facteur", contexte.temps_courant);
		auto decalage = evalue_decimal("décalage", contexte.temps_courant);
		auto inverse = evalue_bool("inverse");

//		facteur = std::pow(facteur * 2.0f, 10.0f);
//		decalage = (decalage - 0.5f) * 10.0f;

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
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_terrain_creation.jo";
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
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("terrain")) {
			this->ajoute_avertissement("Il n'y a pas de terrain en aval.");
			return EXECUTION_ECHOUEE;
		}

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto dico_type = dls::cree_dico(
					dls::paire(dls::chaine("cellule"), bruit::type::CELLULE),
					dls::paire(dls::chaine("fourier"), bruit::type::FOURIER),
					dls::paire(dls::chaine("ondelette"), bruit::type::ONDELETTE),
					dls::paire(dls::chaine("perlin"), bruit::type::PERLIN),
					dls::paire(dls::chaine("simplex"), bruit::type::SIMPLEX),
					dls::paire(dls::chaine("voronoi_f1"), bruit::type::VORONOI_F1),
					dls::paire(dls::chaine("voronoi_f2"), bruit::type::VORONOI_F2),
					dls::paire(dls::chaine("voronoi_f3"), bruit::type::VORONOI_F3),
					dls::paire(dls::chaine("voronoi_f4"), bruit::type::VORONOI_F4),
					dls::paire(dls::chaine("voronoi_f1f2"), bruit::type::VORONOI_F1F2),
					dls::paire(dls::chaine("voronoi_cr"), bruit::type::VORONOI_CR));

		auto chn_type = evalue_enum("type");
		auto plg_type = dico_type.trouve(chn_type);
		auto type = bruit::type{};

		if (plg_type.est_finie()) {
			ajoute_avertissement("type inconnu");
			type = bruit::type::SIMPLEX;
		}
		else {
			type = plg_type.front().second;
		}

		auto graine = 0;
		auto params = bruit::parametres();
		params.taille_bruit = evalue_vecteur("fréquence", contexte.temps_courant);
		params.origine_bruit = evalue_vecteur("décalage");
		params.echelle_valeur = evalue_decimal("échelle_valeur", contexte.temps_courant);
		params.decalage_valeur = evalue_decimal("décalage_valeur", contexte.temps_courant);
		params.type_bruit = type;
		params.temps_anim = evalue_decimal("temps", contexte.temps_courant);

		bruit::construit(type, params, graine);

		auto params_turb = bruit::param_turbulence();
		params_turb.dur = evalue_bool("dur");
		params_turb.octaves = evalue_decimal("octaves", contexte.temps_courant);
		params_turb.amplitude = evalue_decimal("amplitude", contexte.temps_courant);
		params_turb.gain = evalue_decimal("persistence", contexte.temps_courant);
		params_turb.lacunarite = evalue_decimal("lacunarité", contexte.temps_courant);

		auto index = 0;
		for (auto y = 0; y < desc.resolution.y; ++y) {
			for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
				auto p = terrain->hauteur.index_vers_unit(dls::math::vec2i(x, y));
				auto valeur = bruit::evalue_turb(params, params_turb, dls::math::vec3f(p, 0.0f));

				terrain->hauteur.valeur(index, valeur);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpEvalueTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Évalue Terrain";
	static constexpr auto AIDE = "";

	OpEvalueTerrain(Graphe &graphe_parent, Noeud &noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_terrain_evaluation.jo";
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

		auto res_x = evalue_entier("résolution_x");
		auto res_y = evalue_entier("résolution_y");
		auto taille_x = evalue_decimal("taille_x");
		auto taille_y = evalue_decimal("taille_y");

		auto desc = desc_depuis_hauteur_largeur(res_x, res_y);

		terrain.hauteur = wlk::grille_dense_2d<float>(desc);
		terrain.normal  = wlk::grille_dense_2d<dls::math::vec3f>(desc);

		desc = terrain.hauteur.desc();

		entree(0)->requiers_corps(contexte, &da);

		auto type_normaux = NORMAUX_DIFF_CENTRE;
		auto chn_normaux = evalue_enum("type_normaux");

		if (chn_normaux == "voisins4") {
			type_normaux = NORMAUX_VOISINS_4;
		}
		else if (chn_normaux == "voisins8") {
			type_normaux = NORMAUX_VOISINS_8;
		}

		calcul_normaux(terrain, type_normaux);

		auto attr_N = m_corps.ajoute_attribut("N", type_attribut::R32, 3, portee_attr::POINT);

		auto index = 0;
		for (auto y = 0; y < desc.resolution.y; ++y) {
			for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
				auto pos = terrain.hauteur.index_vers_unit(dls::math::vec2i(x, y));

				auto h = terrain.hauteur.valeur(index);
				auto const &normal = terrain.normal.valeur(index);

				auto idx_pnt = m_corps.ajoute_point((pos.x - 0.5f) * taille_x, h, (pos.y - 0.5f) * taille_y);
				assigne(attr_N->r32(idx_pnt), normal);

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
	usine.enregistre_type(cree_desc<OpEvalueTerrain>());
	usine.enregistre_type(cree_desc<OpInclineTerrain>());
	usine.enregistre_type(cree_desc<OpVentTerrain>());
}

#pragma clang diagnostic pop
