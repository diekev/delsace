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

#include "monde.hh"

#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/outils/gna.hh"

#include "corps/limites_corps.hh"
#include "corps/iter_volume.hh"

#include "coeur/objet.h"

#include "fluide.hh"

namespace psn {

template <typename T>
static auto fill_grid(Grille<T> *flags, T valeur)
{
	std::cerr << __func__ << '\n';

	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

		flags->valeur(idx) = valeur;
	}
}

void fill_grid(Grille<int> &flags, int type)
{
	auto res = flags.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);
		auto val = flags.valeur(idx);

		if ((val & TypeObstacle) == 0 && (val & TypeInflow) == 0 && (val & TypeOutflow) == 0 && (val & TypeOpen) == 0) {
			val = (val & ~(TypeVide | TypeFluid)) | type;
			flags.valeur(idx, val);
		}
	}
}

static auto ajourne_murs_domaine(Grille<int> &drapeaux)
{
	auto res = drapeaux.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;

	auto iter = IteratricePosition(limites);

	auto const boundaryWidth = 0;
	auto const w = boundaryWidth;

	int types[6] = {
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
	};

	while (!iter.fini()) {
		auto pos = iter.suivante();

		auto i = pos.x;
		auto j = pos.y;
		auto k = pos.z;

		if (i <= w) {
			drapeaux.valeur(pos, types[0]);
		}

		if (i >= res.x - 1 - w) {
			drapeaux.valeur(pos, types[1]);
		}

		if (j <= w) {
			drapeaux.valeur(pos, types[2]);
		}

		if (j >= res.y - 1 - w) {
			drapeaux.valeur(pos, types[3]);
		}

		if (k <= w) {
			drapeaux.valeur(pos, types[4]);
		}

		if (k >= res.z - 1 - w) {
			drapeaux.valeur(pos, types[5]);
		}
	}
}

#undef UTILISE_BRUIT

void ajourne_sources(Poseidon &poseidon, int temps)
{
	auto corps = Corps();

	auto densite = poseidon.densite;
	auto res = densite->resolution();

#ifdef UTILISE_BRUIT
	poseidon.bruit.genere_donnees();

	auto echelle_bruit = 1.0f;
	auto sigma = 0.5f;
	auto facteur_bruit = dls::math::restreint(1.0f - 0.5f / sigma + sigma, 0.0f, 1.0f);
#endif

	for (auto const &params : poseidon.monde.sources) {
		auto objet = params.objet;

		if (temps < params.debut || temps > params.fin) {
			continue;
		}

#ifdef UTILISE_BRUIT
		auto facteur_densite = echelle_bruit * facteur_bruit * params.densite;
#endif

		/* copie par convénience */
		objet->corps.accede_lecture([&](Corps const &corps_objet)
		{
			corps_objet.copie_vers(&corps);
		});

		/* À FAIRE : considère la surface des maillages. */
		auto lim = calcule_limites_mondiales_corps(corps);
		auto min_idx = densite->monde_vers_unit(lim.min);
		auto max_idx = densite->monde_vers_unit(lim.max);

		auto limites = limites3i{};
		limites.min.x = static_cast<int>(static_cast<float>(res.x) * min_idx.x);
		limites.min.y = static_cast<int>(static_cast<float>(res.y) * min_idx.y);
		limites.min.z = static_cast<int>(static_cast<float>(res.z) * min_idx.z);
		limites.max.x = static_cast<int>(static_cast<float>(res.x) * max_idx.x);
		limites.max.y = static_cast<int>(static_cast<float>(res.y) * max_idx.y);
		limites.max.z = static_cast<int>(static_cast<float>(res.z) * max_idx.z);
		auto iter = IteratricePosition(limites);

		auto gna_part = GNA{};
		auto dx2 = densite->taille_voxel() * 0.5f;

		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

#ifdef UTILISE_BRUIT
			auto pos_monde = dls::math::discret_vers_continu<float>(pos);
			auto densite_cible = facteur_densite * poseidon.bruit.evalue(&pos_monde[0]);
#else
			auto densite_cible = params.densite;
#endif

			auto densite_finale = 0.0f;

			switch (params.fusion) {
				case mode_fusion::SUPERPOSITION:
				{
					auto u = densite->valeur(idx);
					auto v = densite_cible;

					densite_finale = dls::math::entrepolation_lineaire(u, v, params.facteur);
					break;
				}
				case mode_fusion::ADDITION:
				{
					auto u = densite->valeur(idx);
					auto v = densite_cible;

					densite_finale = dls::math::entrepolation_lineaire(u, u + v, params.facteur);
					break;
				}
				case mode_fusion::SOUSTRACTION:
				{
					auto u = densite->valeur(idx);
					auto v = densite_cible;

					densite_finale = dls::math::entrepolation_lineaire(u, u - v, params.facteur);
					break;
				}
				case mode_fusion::MINIMUM:
				{
					auto u = densite->valeur(idx);
					auto v = densite_cible;

					densite_finale = dls::math::entrepolation_lineaire(u, std::min(u, v), params.facteur);
					break;
				}
				case mode_fusion::MAXIMUM:
				{
					auto u = densite->valeur(idx);
					auto v = densite_cible;

					densite_finale = dls::math::entrepolation_lineaire(u, std::max(u, v), params.facteur);
					break;
				}
				case mode_fusion::MULTIPLICATION:
				{
					auto u = densite->valeur(idx);
					auto v = densite_cible;

					densite_finale = dls::math::entrepolation_lineaire(u, u * v, params.facteur);
					break;
				}
			}

			//densite->valeur(idx) = densite_finale;

			auto centre_voxel = densite->index_vers_monde(pos);

			for (auto i = 0; i < 8; ++i) {
				auto particule = memoire::loge<Particule>("part_psn");
				particule->densite = densite_finale / 8.0f;
				particule->pos = centre_voxel;
				particule->pos.x += gna_part.uniforme(-dx2, dx2);
				particule->pos.y += gna_part.uniforme(-dx2, dx2);
				particule->pos.z += gna_part.uniforme(-dx2, dx2);

				poseidon.particules.pousse(particule);
			}
		}
	}
}

void ajourne_obstables(Poseidon &poseidon)
{
	auto corps = Corps();

	auto densite = poseidon.densite;
	auto drapeaux = poseidon.drapeaux;
	auto res = densite->resolution();

	for (auto objet : poseidon.monde.obstacles) {
		/* copie par convénience */
		objet->corps.accede_lecture([&](Corps const &corps_objet)
		{
			corps_objet.copie_vers(&corps);
		});

		/* À FAIRE : considère la surface des maillages. */
		auto lim = calcule_limites_mondiales_corps(corps);
		auto min_idx = densite->monde_vers_unit(lim.min);
		auto max_idx = densite->monde_vers_unit(lim.max);

		auto limites = limites3i{};
		limites.min.x = static_cast<int>(static_cast<float>(res.x) * min_idx.x);
		limites.min.y = static_cast<int>(static_cast<float>(res.y) * min_idx.y);
		limites.min.z = static_cast<int>(static_cast<float>(res.z) * min_idx.z);
		limites.max.x = static_cast<int>(static_cast<float>(res.x) * max_idx.x);
		limites.max.y = static_cast<int>(static_cast<float>(res.y) * max_idx.y);
		limites.max.z = static_cast<int>(static_cast<float>(res.z) * max_idx.z);
		auto iter = IteratricePosition(limites);

		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

			/* À FAIRE : decouplage */
			if (!poseidon.decouple) {
				densite->valeur(idx) = 0.0f;
			}

			drapeaux->valeur(idx) = TypeObstacle;
		}
	}

	ajourne_murs_domaine(*poseidon.drapeaux);
}

Poseidon::~Poseidon()
{
	supprime_particules();
}

void Poseidon::supprime_particules()
{
	for (auto p : particules) {
		memoire::deloge("part_psn", p);
	}

	particules.efface();
}

}  /* namespace psn */
