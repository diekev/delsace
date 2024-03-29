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

#include "coeur/delegue_hbe.hh"
#include "coeur/objet.h"

#include "wolika/iteration.hh"

#include "fluide.hh"

namespace psn {

void fill_grid(wlk::grille_dense_3d<int> &flags, int type)
{
    auto res = flags.desc().resolution;
    auto limites = limites3i{};
    limites.min = dls::math::vec3i(0);
    limites.max = res;
    auto iter = wlk::IteratricePosition(limites);

    while (!iter.fini()) {
        auto pos = iter.suivante();
        auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);
        auto val = flags.valeur(idx);

        if ((val & TypeObstacle) == 0 && (val & TypeInflow) == 0 && (val & TypeOutflow) == 0 &&
            (val & TypeOpen) == 0) {
            val = (val & ~(TypeVide | TypeFluid)) | type;
            flags.valeur(idx, val);
        }
    }
}

static auto ajourne_murs_domaine(wlk::grille_dense_3d<int> &drapeaux)
{
    auto res = drapeaux.desc().resolution;
    auto limites = limites3i{};
    limites.min = dls::math::vec3i(0);
    limites.max = res;

    auto iter = wlk::IteratricePosition(limites);

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

static auto compose_valeur(ParametresSource const &params,
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
    auto temperature = poseidon.temperature;
    auto fioul = poseidon.fioul;
    auto &grille_particule = poseidon.grille_particule;
    auto res = densite->desc().resolution;

#ifdef UTILISE_BRUIT
    poseidon.bruit = bruit::ondelette::construit(5);
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
        objet->donnees.accede_lecture([&](DonneesObjet const *donnees) {
            auto const &corps_objet = extrait_corps(donnees);
            corps_objet.copie_vers(&corps);
        });

        auto delegue = DeleguePrim(corps);
        auto arbre = construit_arbre_hbe(delegue, 12);

        auto limites = limites3i{};
        limites.min = dls::math::vec3i(1);
        limites.max = densite->desc().resolution - dls::math::vec3i(1);
        auto iter = wlk::IteratricePosition(limites);

        auto gna_part = GNA{};
        auto dx2 = static_cast<float>(densite->desc().taille_voxel * 0.5);

        while (!iter.fini()) {
            auto pos = iter.suivante();
            auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

            auto pos_mnd = densite->index_vers_monde(pos);
            auto pos_mnd_d = dls::math::point3d(static_cast<double>(pos_mnd.x),
                                                static_cast<double>(pos_mnd.y),
                                                static_cast<double>(pos_mnd.z));

            auto dist_max = densite->desc().taille_voxel;
            dist_max *= dist_max;

            auto dpp = cherche_point_plus_proche(arbre, delegue, pos_mnd_d, dist_max);

            if (dpp.distance_carree < -0.5) {
                continue;
            }

            if (dpp.distance_carree >= dist_max) {
                continue;
            }

#ifdef UTILISE_BRUIT
            auto pos_monde = dls::math::discret_vers_continu<float>(pos);
            auto densite_cible = facteur_densite * poseidon.bruit.evalue(pos_monde);
#else
            auto densite_cible = params.densite;
#endif

            if (!poseidon.solveur_flip) {
                densite->valeur(idx) = compose_valeur(params, densite->valeur(idx), densite_cible);

                if (temperature != nullptr) {
                    temperature->valeur(idx) = params.temperature;
                }

                if (fioul != nullptr) {
                    fioul->valeur(idx) = std::max(params.fioul, fioul->valeur(idx));
                }

                continue;
            }

            auto densite_courante = 0.0f;
            auto nombre_a_genere = 0;

            auto &cellule_part = grille_particule.cellule(idx);

            if (cellule_part.taille() == 0) {
                nombre_a_genere = 8;
            }
            else {
                auto dens_parts = poseidon.parts.champs_scalaire("densité");

                /* calcul la densité, compose avec les particules courantes */
                for (auto p : cellule_part) {
                    densite_courante += dens_parts[p];
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
                    auto index = poseidon.parts.ajoute_particule();

                    /* ajoute particule peut modifier les pointeurs */
                    auto dens_parts = poseidon.parts.champs_scalaire("densité");
                    auto pos_parts = poseidon.parts.champs_vectoriel("position");

                    auto d = densite_finale / static_cast<float>(nombre_a_genere);

                    if (d < 0.0f) {
                        d = 0.0f;
                    }

                    dens_parts[index] = d;

                    auto p = centre_voxel;
                    p.x += gna_part.uniforme(-dx2, dx2);
                    p.y += gna_part.uniforme(-dx2, dx2);
                    p.z += gna_part.uniforme(-dx2, dx2);
                    pos_parts[index] = p;
                }
            }
            else {
                auto dens_parts = poseidon.parts.champs_scalaire("densité");

                for (auto p : cellule_part) {
                    dens_parts[p] = densite_finale / static_cast<float>(cellule_part.taille());
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
        objet->donnees.accede_lecture([&](DonneesObjet const *donnees) {
            auto const &corps_objet = extrait_corps(donnees);
            corps_objet.copie_vers(&corps);
        });

        /* À FAIRE : considère la surface des maillages. */
        auto lim = calcule_limites_mondiales_corps(corps);
        auto min_idx = densite->monde_vers_unit(lim.min);
        auto max_idx = densite->monde_vers_unit(lim.max);

        auto limites = limites3i{};
        limites.min = dls::math::converti_type<int>(dls::math::converti_type<float>(res) *
                                                    min_idx);
        limites.max = dls::math::converti_type<int>(dls::math::converti_type<float>(res) *
                                                    max_idx);
        auto iter = wlk::IteratricePosition(limites);

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
    parts.efface();
}

float calcul_vel_max(wlk::GrilleMAC const &vel)
{
    auto vel_max = 0.0f;

    for (auto i = 0; i < vel.nombre_elements(); ++i) {
        vel_max = std::max(vel_max, longueur(vel.valeur(i)));
    }

    return vel_max;
}

void calcul_dt(Poseidon &poseidon, float vel_max)
{
    /* Méthode de Mantaflow, limitant Dt selon le temps de l'image, où Dt se
     * rapproche de 30 image/seconde (d'où les epsilon de 10^-4). */
    auto dt = poseidon.dt;
    auto cfl = poseidon.cfl;
    auto dt_min = poseidon.dt_min;
    auto dt_max = poseidon.dt_max;
    auto temps_par_frame = poseidon.temps_par_frame;
    auto duree_frame = poseidon.duree_frame;
    auto vel_max_dt = vel_max * poseidon.dt;

    if (!poseidon.verrouille_dt) {
        dt = std::max(std::min(dt * (cfl / (vel_max_dt + 1e-05f)), dt_max), dt_min);

        if ((temps_par_frame + dt * 1.05f) > duree_frame) {
            /* à 5% d'une durée d'image ? ajoute epsilon pour prévenir les
             * erreurs d'arrondissement... */
            dt = (duree_frame - temps_par_frame) + 1e-04f;
        }
        else if ((temps_par_frame + dt + dt_min) > duree_frame ||
                 (temps_par_frame + (dt * 1.25f)) > duree_frame) {
            /* évite les petits pas ainsi que ceux avec beaucoup de variance,
             * donc divisons par 2 pour en faire des moyens au besoin */
            dt = (duree_frame - temps_par_frame + 1e-04f) * 0.5f;
            poseidon.verrouille_dt = true;
        }
    }

    poseidon.dt = dt;

#if 0
	/* Méthode de Bridson, limitant Dt selon la vélocité max pour éviter qu'il
	 * n'y ait un déplacement de plus de 5 cellules lors des advections.
	 * "Fluid Simulation Course Notes", SIGGRAPH 2007.
	 * https://www.cs.ubc.ca/~rbridson/fluidsimulation/fluids_notes.pdf
	 *
	 * Ancien code gardé pour référence.
	 */
	auto const factor = poseidon.cfl;
	auto const dh = 5.0f / static_cast<float>(poseidon.resolution);
	auto const vel_max_ = std::max(1e-16f, std::max(dh, vel_max));

	/* dt <= (5dh / max_u) */
	poseidon.dt = std::min(poseidon.dt, factor * dh / std::sqrt(vel_max_));
#endif
}

} /* namespace psn */
