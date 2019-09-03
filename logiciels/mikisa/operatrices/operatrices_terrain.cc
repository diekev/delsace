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
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/noeud.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/operatrice_graphe_detail.hh"
#include "coeur/usine_operatrice.h"

#include "lcc/lcc.hh"

#include "wolika/echantillonnage.hh"
#include "wolika/outils.hh"

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
 * VOIR AUSSI
 *
 * Logiciels :
 * - A.N.T. de Blender
 * - Terragen
 * - Vue
 * - WorldMachine
 *
 * Publications :
 * - Interactive Example-Based Terrain Authoring with Conditional Generative Adversarial Networks
 *   (https://hal.archives-ouvertes.fr/hal-01583706/file/tog.pdf)
 * - Terrain Generation Using Procedural Models Based on Hydrology
 *   (https://arches.liris.cnrs.fr/publications/articles/SIGGRAPH2013_PCG_Terrain.pdf)
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

	OpVentTerrain(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

/**
 * Structure et algorithme issus du greffon A.N.T. Landscape de Blender. Le
 * model sous-jacent est expliqué ici :
 * https://blog.michelanders.nl/search/label/erosion
 */
struct erodeuse {
	using type_grille = wlk::grille_dense_2d<float>;
	wlk::desc_grille_2d desc{};
	type_grille roche{};
	type_grille *eau = nullptr;
	type_grille *sediment = nullptr;
	type_grille *scour = nullptr;
	type_grille *flowrate = nullptr;
	type_grille *sedimentpct = nullptr;
	type_grille *capacity = nullptr;
	type_grille *avalanced = nullptr;
	type_grille *rainmap = nullptr;
	float maxrss = 0.0f;

	/* pour normaliser les données lors des exports (par exemple via des
	 * attributs sur les points) */
	float max_eau = 1.0f;
	float max_flowrate = 1.0f;
	float max_scour = 1.0f;
	float max_sediment = 1.0f;
	float min_scour = 1.0f;

	erodeuse(int taille = 10)
		: desc(wlk::desc_depuis_hauteur_largeur(taille, taille))
	{
		roche = type_grille(desc);
		desc = roche.desc();
	}

	COPIE_CONSTRUCT(erodeuse);

	~erodeuse()
	{
		memoire::deloge("grille_erodeuse", eau);
		memoire::deloge("grille_erodeuse", sediment);
		memoire::deloge("grille_erodeuse", scour);
		memoire::deloge("grille_erodeuse", flowrate);
		memoire::deloge("grille_erodeuse", sedimentpct);
		memoire::deloge("grille_erodeuse", avalanced);
		memoire::deloge("grille_erodeuse", capacity);
	}

	void initialise_eau_et_sediment()
	{
		if (eau == nullptr) {
			eau = memoire::loge<type_grille>("grille_erodeuse", desc);
		}

		if (sediment == nullptr) {
			sediment = memoire::loge<type_grille>("grille_erodeuse", desc);
		}

		if (scour == nullptr) {
			scour = memoire::loge<type_grille>("grille_erodeuse", desc);
		}

		if (flowrate == nullptr) {
			flowrate = memoire::loge<type_grille>("grille_erodeuse", desc);
		}

		if (sedimentpct == nullptr) {
			sedimentpct = memoire::loge<type_grille>("grille_erodeuse", desc);
		}

		if (avalanced == nullptr) {
			avalanced = memoire::loge<type_grille>("grille_erodeuse", desc);
		}

		if (capacity == nullptr) {
			capacity = memoire::loge<type_grille>("grille_erodeuse", desc);
		}
	}

	void peak(float valeur = 1.0f)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;

		//self.center[int(nx/2),int(ny/2)] += value
		this->roche.valeur(dls::math::vec2i(nx/2, ny/2)) += valeur;
	}

	void shelf(float valeur = 1.0f)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;
		//self.center[:nx/2] += value

		for (auto y = 0; y < ny; ++y) {
			for (auto x = nx / 2; x < nx; ++x) {
				this->roche.valeur(dls::math::vec2i(x, y)) += valeur;
			}
		}
	}

	void mesa(float valeur = 1.0f)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;
		//self.center[nx/4:3*nx/4,ny/4:3*ny/4] += value

		for (auto y = ny / 4; y < 3 * ny / 4; ++y) {
			for (auto x = nx / 4; x < 3 * nx / 4; ++x) {
				this->roche.valeur(dls::math::vec2i(x, y)) += valeur;
			}
		}
	}

	void random(float valeur = 1.0f)
	{
		auto gna = GNA();

		for (auto i = 0; i < this->roche.nombre_elements(); ++i) {
			this->roche.valeur(i) += gna.uniforme(0.0f, 1.0f) * valeur;
		}
	}

	void zeroedge(type_grille *quantity = nullptr)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;
		auto grille = (quantity == nullptr) ? &this->roche : quantity;

		for (auto y = 0; y < ny; ++y) {
			grille->valeur(dls::math::vec2i(0, y)) = 0.0f;
			grille->valeur(dls::math::vec2i(nx - 1, y)) = 0.0f;
		}

		for (auto x = 0; x < nx; ++x) {
			grille->valeur(dls::math::vec2i(x, 0)) = 0.0f;
			grille->valeur(dls::math::vec2i(x, ny - 1)) = 0.0f;
		}
	}

	void pluie(float amount = 1.0f, float variance = 0.0f)
	{
		auto gna = GNA();

		for (auto i = 0; i < this->eau->nombre_elements(); ++i) {
			auto valeur = (1.0f - gna.uniforme(0.0f, 1.0f) * variance);

			if (this->rainmap != nullptr) {
				valeur *= rainmap->valeur(i);
			}
			else {
				valeur *= amount;
			}

			this->eau->valeur(i) += valeur;
		}
	}

	void diffuse(float Kd, int IterDiffuse)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;

		auto temp = type_grille(desc);

		Kd /= static_cast<float>(IterDiffuse);

		for (auto y = 1; y < ny - 1; ++y) {
			for (auto x = 1; x < nx - 1; ++x) {
				auto index = this->roche.calcul_index(dls::math::vec2i(x, y));

				auto c = this->roche.valeur(index);
				auto up = this->roche.valeur(index - nx);
				auto down = this->roche.valeur(index + nx);
				auto left = this->roche.valeur(index - 1);
				auto right = this->roche.valeur(index + 1);

				temp.valeur(index) = c + Kd * (up + down + left + right - 4.0f * c);
			}
		}

		//self.maxrss = max(getmemsize(), self.maxrss);

		this->roche = temp;
	}

	void avalanche(float delta, int iterava, float prob)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;

		auto temp = type_grille(desc);

		auto gna = GNA();

		for (auto y = 1; y < ny - 1; ++y) {
			for (auto x = 1; x < nx - 1; ++x) {
				auto index = this->roche.calcul_index(dls::math::vec2i(x, y));

				auto c = this->roche.valeur(index);
				auto up = this->roche.valeur(index - nx);
				auto down = this->roche.valeur(index + nx);
				auto left = this->roche.valeur(index - 1);
				auto right = this->roche.valeur(index + 1);

				auto sa = 0.0f;
				// incoming
				if (up - c > delta) {
					sa += (up - c - delta) * 0.5f;
				}
				if (down - c > delta) {
					sa += (down - c - delta) * 0.5f;
				}
				if (left - c > delta) {
					sa += (left - c - delta) * 0.5f;
				}
				if (right - c > delta) {
					sa += (right - c - delta) * 0.5f;
				}

				// outgoing
				if (up - c < -delta) {
					sa += (up - c + delta) * 0.5f;
				}
				if (down - c < -delta) {
					sa += (down - c + delta) * 0.5f;
				}
				if (left - c < -delta) {
					sa += (left - c + delta) * 0.5f;
				}
				if (right - c < -delta) {
					sa += (right - c + delta) * 0.5f;
				}

				if (gna.uniforme(0.0f, 1.0f) >= prob) {
					sa = 0.0f;
				}

				this->avalanced->valeur(index) += sa / static_cast<float>(iterava);
				temp.valeur(index) = c + sa / static_cast<float>(iterava);
			}
		}

		//self.maxrss = max(getmemsize(), self.maxrss);

		this->roche = temp;
	}

   // px, py and radius are all fractions
	void spring(float amount, float px, float py, float radius)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;
		auto rx = std::max(static_cast<int>(static_cast<float>(nx) * radius), 1);
		auto ry = std::max(static_cast<int>(static_cast<float>(ny) * radius), 1);

		auto sx = static_cast<int>(static_cast<float>(nx) * px);
		auto sy = static_cast<int>(static_cast<float>(ny) * py);

		for (auto y = sy - ry; y <= sy + ry; ++y) {
			for (auto x = sx - rx; x <= sx + rx; ++x) {
				auto index = this->eau->calcul_index(dls::math::vec2i(x, y));
				auto e = this->eau->valeur(index);
				e += amount;
				this->eau->valeur(index, e);
			}
		}
	}

	void riviere(float Kc, float Ks, float Kdep, float Ka, float Kev)
	{
		auto &rock = this->roche;
		auto sc = type_grille(desc);
		auto height = type_grille(desc);

		for (auto i = 0; i < this->roche.nombre_elements(); ++i) {
			auto e = this->eau->valeur(i);

			height.valeur(i) = this->roche.valeur(i) + e;

			if (e > 0.0f) {
				sc.valeur(i) = this->sediment->valeur(i) / e;
			}
		}

		// !! this gives a runtime warning for division by zero

		auto sdw = type_grille(desc);
		auto svdw = type_grille(desc);
		auto sds = type_grille(desc);
		auto angle = type_grille(desc);
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;

		// peut-être faut-il faire 4 boucles, pour chaque voisin ?
		for (auto y = 1; y < ny - 1; ++y) {
			for (auto x = 1; x < nx - 1; ++x) {
				auto index = rock.calcul_index(dls::math::vec2i(x, y));

				long voisins[4] = {
					index - 1, index + 1, index - nx, index + nx
				};

				for (auto i = 0; i < 4; ++i) {
					auto dw = height.valeur(voisins[i]) - height.valeur(index);
					auto influx = dw > 0.0f;

					if (influx) {
						dw = std::min(this->eau->valeur(voisins[i]), dw);
					}
					else {
						dw = std::max(-this->eau->valeur(index), dw) / 4.0f;
					}

					sdw.valeur(index) += dw;

					if (influx) {
						sds.valeur(index) += dw * sc.valeur(voisins[i]);
					}
					else {
						sds.valeur(index) += dw * sc.valeur(index);
					}

					svdw.valeur(index) += std::abs(dw);

					angle.valeur(index) += std::atan(std::abs(rock.valeur(voisins[i]) - rock.valeur(index)));
				}
			}
		}

		for (auto y = 1; y < ny - 1; ++y) {
			for (auto x = 1; x < nx - 1; ++x) {
				auto index = rock.calcul_index(dls::math::vec2i(x, y));

				auto wcc = this->eau->valeur(index);
				auto scc = this->sediment->valeur(index);
				//auto rcc = rock.valeur(index);

				this->eau->valeur(index) = wcc * (1.0f - Kev) + sdw.valeur(index);
				this->sediment->valeur(index) = scc + sds.valeur(index);

				if (wcc > 0.0f) {
					sc.valeur(index) = scc / wcc;
				}
				else {
					sc.valeur(index) = 2.0f * Kc;
				}

				auto fKc = Kc * svdw.valeur(index);
				auto ds = ((fKc > sc.valeur(index)) ? (fKc - sc.valeur(index)) * Ks : (fKc - sc.valeur(index)) * Kdep) * wcc;

				this->flowrate->valeur(index) = svdw.valeur(index);
				this->scour->valeur(index) = ds;
				this->sedimentpct->valeur(index) = sc.valeur(index);
				this->capacity->valeur(index) = fKc;
				this->sediment->valeur(index) = scc + ds + sds.valeur(index);
			}
		}
	}

	void flow(float Kc, float Ks, float Kz, float Ka)
	{
		auto nx = desc.resolution.x;
		auto ny = desc.resolution.y;

		for (auto y = 1; y < ny - 1; ++y) {
			for (auto x = 1; x < nx - 1; ++x) {
				auto index = this->roche.calcul_index(dls::math::vec2i(x, y));
				auto rcc = this->roche.valeur(index);
				auto ds = this->scour->valeur(index);

				auto valeur = rcc + ds * Kz;

				// there isn't really a bottom to the rock but negative values look ugly
				if (valeur < 0.0f) {
					valeur = rcc;
				}

				this->roche.valeur(index, rcc + ds * Kz);
			}
		}
	}

	void generation_riviere(
	  float quantite_pluie,
		float variance_pluie,
			  float Kc,
			  float Ks,
			  float Kdep,
			  float Ka,
			  float Kev)
	{
		this->initialise_eau_et_sediment();
		this->pluie(quantite_pluie, variance_pluie);
		this->zeroedge(this->eau);
		this->zeroedge(this->sediment);
		this->riviere(Kc, Ks, Kdep, Ka, Kev);
		this->max_eau = wlk::extrait_max(*this->eau);
	}

	void erosion_fluviale(
					float Kc,
					float Ks,
					float Kdep,
					float Ka)
	{
		this->flow(Kc, Ks, Kdep, Ka);
		this->max_flowrate = wlk::extrait_max(*this->flowrate);
		this->max_sediment = wlk::extrait_max(*this->sediment);
		wlk::extrait_min_max(*this->scour, this->min_scour, this->max_scour);
	}

//	def analyze(self):
//		self.neighborgrid()
//		# just looking at up and left to avoid needless double calculations
//		slopes=np.concatenate((np.abs(self.left - self.center),np.abs(self.up - self.center)))
//		return '\n'.join(["%-15s: %.3f"%t for t in [
//				('height average', np.average(self.center)),
//				('height median', np.median(self.center)),
//				('height max', np.max(self.center)),
//				('height min', np.min(self.center)),
//				('height std', np.std(self.center)),
//				('slope average', np.average(slopes)),
//				('slope median', np.median(slopes)),
//				('slope max', np.max(slopes)),
//				('slope min', np.min(slopes)),
//				('slope std', np.std(slopes))
//				]]
//			)

};

class OpErosionTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Érosion Terrain";
	static constexpr auto AIDE = "";

	OpErosionTerrain(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	OpInclineTerrain(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

class OpCreeTerrain final : public OperatriceCorps {
	compileuse_lng m_compileuse{};
	gestionnaire_propriete m_gest_props{};
	gestionnaire_propriete m_gest_attrs{};

public:
	static constexpr auto NOM = "Crée Terrain";
	static constexpr auto AIDE = "";

	OpCreeTerrain(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(0);

		noeud.peut_avoir_graphe = true;
		noeud.graphe.type = type_graphe::DETAIL;
		noeud.graphe.donnees.efface();
		noeud.graphe.donnees.pousse(static_cast<int>(DETAIL_TERRAIN));
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

		auto chef = contexte.chef;

		if (!compile_graphe(contexte)) {
			ajoute_avertissement("Ne peut pas compiler le graphe, voir si les noeuds n'ont pas d'erreurs.");
			return EXECUTION_ECHOUEE;
		}

		if (noeud.graphe.noeuds().taille() == 0) {
			return EXECUTION_REUSSIE;
		}

		auto terrain = extrait_terrain(donnees_aval);
		auto desc = terrain->hauteur.desc();

		auto ctx_exec = lcc::ctx_exec{};

		boucle_parallele(tbb::blocked_range<int>(0, desc.resolution.y),
						 [&](tbb::blocked_range<int> const &plage)
		{
			if (chef->interrompu()) {
				return;
			}

			/* fais une copie locale pour éviter les problèmes de concurrence critique */
			auto donnees = m_compileuse.donnees();
			auto ctx_local = lcc::ctx_local{};

			for (auto y = plage.begin(); y < plage.end(); ++y) {
				for (auto x = 0; x < desc.resolution.x; ++x) {
					auto index = terrain->hauteur.calcul_index(dls::math::vec2i(x, y));
					auto p = terrain->hauteur.index_vers_unit(dls::math::vec2i(x, y));

					auto pos = dls::math::vec3f(p, 0.0f);
					remplis_donnees(donnees, m_gest_props, "P", pos);

					lcc::execute_pile(
								ctx_exec,
								ctx_local,
								donnees,
								m_compileuse.instructions(),
								static_cast<int>(index));

					auto idx_sortie = m_gest_props.pointeur_donnees("hauteur");

					if (idx_sortie != -1) {
						auto h = donnees.charge_decimal(idx_sortie);
						terrain->hauteur.valeur(index, h);
					}
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			delta *= 1.0f / static_cast<float>(desc.resolution.y);

			chef->indique_progression_parallele(delta * 100.0f);
		});

		return EXECUTION_REUSSIE;
	}

	bool compile_graphe(const ContexteEvaluation &contexte)
	{
		m_compileuse = compileuse_lng();
		m_gest_props = gestionnaire_propriete();
		m_gest_attrs = gestionnaire_propriete();

		if (noeud.graphe.besoin_ajournement) {
			tri_topologique(noeud.graphe);
			noeud.graphe.besoin_ajournement = false;
		}

		auto donnees_aval = DonneesAval{};
		donnees_aval.table.insere({"compileuse", &m_compileuse});
		donnees_aval.table.insere({"gest_props", &m_gest_props});
		donnees_aval.table.insere({"gest_attrs", &m_gest_attrs});

		auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(lcc::type_var::VEC3));
		m_gest_props.ajoute_propriete("P", lcc::type_var::VEC3, idx);

		for (auto &noeud_graphe : noeud.graphe.noeuds()) {
			for (auto &sortie : noeud_graphe->sorties) {
				sortie->decalage_pile = 0;
			}

			auto operatrice = extrait_opimage(noeud_graphe->donnees);
			operatrice->reinitialise_avertisements();

			auto resultat = operatrice->execute(contexte, &donnees_aval);

			if (resultat == EXECUTION_ECHOUEE) {
				return false;
			}
		}

		return true;
	}
};

/* ************************************************************************** */

class OpEvalueTerrain final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Évalue Terrain";
	static constexpr auto AIDE = "";

	OpEvalueTerrain(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

		auto desc = wlk::desc_depuis_hauteur_largeur(res_x, res_y);

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
