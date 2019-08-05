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
static auto fill_grid(grille_dense_3d<T> *flags, T valeur)
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

void fill_grid(grille_dense_3d<int> &flags, int type)
{
	auto res = flags.desc().resolution;
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

static auto ajourne_murs_domaine(grille_dense_3d<int> &drapeaux)
{
	auto res = drapeaux.desc().resolution;
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

static auto compose_valeur(
		ParametresSource const &params,
		float densite_courante,
		float densite_cible)
{
	auto u = densite_courante;
	auto v = densite_cible;

	switch (params.fusion) {
		case mode_fusion::SUPERPOSITION:
		{
			return dls::math::entrepolation_lineaire(u, v, params.facteur);
		}
		case mode_fusion::ADDITION:
		{
			return dls::math::entrepolation_lineaire(u, u + v, params.facteur);
		}
		case mode_fusion::SOUSTRACTION:
		{
			return dls::math::entrepolation_lineaire(u, u - v, params.facteur);
		}
		case mode_fusion::MINIMUM:
		{
			return dls::math::entrepolation_lineaire(u, std::min(u, v), params.facteur);
		}
		case mode_fusion::MAXIMUM:
		{
			return dls::math::entrepolation_lineaire(u, std::max(u, v), params.facteur);
		}
		case mode_fusion::MULTIPLICATION:
		{
			return dls::math::entrepolation_lineaire(u, u * v, params.facteur);
		}
	}

	return densite_courante;
}

#undef UTILISE_BRUIT

void ajourne_sources(Poseidon &poseidon, int temps)
{
	auto corps = Corps();

	auto densite = poseidon.densite;
	auto &grille_particule = poseidon.grille_particule;
	auto res = densite->desc().resolution;

#ifdef UTILISE_BRUIT
	poseidon.bruit = bruit_vaguelette::construit(5);
	poseidon.bruit.temps_anim = poseidon.temps_total * poseidon.dt;
	poseidon.bruit.echelle_pos = dls::math::vec3f(45.0f);
	poseidon.bruit.dx = static_cast<float>(densite->desc().taille_voxel);
	poseidon.bruit.taille_grille_inv = 1.0f / static_cast<float>(poseidon.bruit.dx);

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
		objet->donnees.accede_lecture([&](DonneesObjet const *donnees)
		{
			auto const &corps_objet = static_cast<DonneesCorps const *>(donnees)->corps;
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
		auto dx2 = static_cast<float>(densite->desc().taille_voxel * 0.5);

		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

#ifdef UTILISE_BRUIT
			auto pos_monde = dls::math::discret_vers_continu<float>(pos);
			auto densite_cible = facteur_densite * poseidon.bruit.evalue(pos_monde);
#else
			auto densite_cible = params.densite;
#endif

			if (!poseidon.solveur_flip) {
				densite->valeur(idx) = compose_valeur(params, densite->valeur(idx), densite_cible);
				continue;
			}

			auto densite_courante = 0.0f;
			auto nombre_a_genere = 0;

			auto &cellule_part = grille_particule.cellule(idx);

			if (cellule_part.taille() == 0) {
				nombre_a_genere = 8;
			}
			else {
				/* calcul la densité, compose avec les particules courantes */
				for (auto p : cellule_part) {
					densite_courante += p->densite;
				}

				/* s'il y a plus que 8 particules, nous aurons un nombre négatif
				 * indiquant que nous ne ferons que modifier la densité
				 * existante */
				nombre_a_genere = 8 - static_cast<int>(cellule_part.taille());
			}

			auto densite_finale = compose_valeur(params, densite_courante, densite_cible);

			auto centre_voxel = densite->index_vers_monde(pos);

			if (nombre_a_genere > 0) {
				for (auto i = 0; i < nombre_a_genere; ++i) {
					auto particule = memoire::loge<Particule>("part_psn");
					particule->densite = densite_finale / static_cast<float>(nombre_a_genere);
					particule->pos = centre_voxel;
					particule->pos.x += gna_part.uniforme(-dx2, dx2);
					particule->pos.y += gna_part.uniforme(-dx2, dx2);
					particule->pos.z += gna_part.uniforme(-dx2, dx2);

					poseidon.particules.pousse(particule);
					cellule_part.pousse(particule);
				}
			}
			else {
				for (auto p : cellule_part) {
					p->densite = densite_finale / static_cast<float>(cellule_part.taille());
				}
			}
		}
	}
}

void ajourne_obstables(Poseidon &poseidon)
{
	auto corps = Corps();

	auto densite = poseidon.densite;
	auto drapeaux = poseidon.drapeaux;
	auto res = densite->desc().resolution;

	for (auto objet : poseidon.monde.obstacles) {
		/* copie par convénience */
		objet->donnees.accede_lecture([&](DonneesObjet const *donnees)
		{
			auto const &corps_objet = static_cast<DonneesCorps const *>(donnees)->corps;
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
