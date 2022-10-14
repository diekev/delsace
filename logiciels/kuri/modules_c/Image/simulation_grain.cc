/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "simulation_grain.hh"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"

#include "biblinternes/structures/tableau.hh"
#include "image.h"
#include <algorithm>
#include <cmath>

namespace image {

/**
 * Simulation de grain tiré de :
 * https://hal.archives-ouvertes.fr/hal-01358392/file/Film_grain_rendering_preprint.pdf
 */

static auto const MAX_NIVEAU_GRIS = 255;

struct ParamatresGrain {
    /* Valeurs venant des paramètres. */
    float rayon = 0.0f;
    float sigma = 0.0f;
    float sigma_filtre = 0.0f;

    /* Valeurs précalculées selon celles du dessus. */
    float rayon_carre = 0.0f;
    float rayon_max = 0.0f;
    float mu = 0.0f;
    float delta = 0.0f;
    float delta_inv = 0.0f;
};

static ParamatresGrain calcule_parametres(const float rayon,
                                          const float sigma,
                                          const float sigma_filtre)
{
    ParamatresGrain resultat{};
    resultat.rayon = rayon;
    resultat.sigma_filtre = sigma_filtre;

    auto const normal_quantile = 3.0902f;  // standard normal quantile for alpha=0.999
    resultat.rayon_carre = rayon * rayon;
    resultat.rayon_max = rayon;

    if (sigma > 0.0f) {
        resultat.sigma = std::sqrt(std::log1p((sigma / rayon) * (sigma / rayon)));
        auto const sigma2 = sigma * sigma;
        resultat.mu = std::log(rayon) - sigma2 / 2.0f;
        auto const log_normal_quantile = std::exp(resultat.mu + sigma * normal_quantile);
        resultat.rayon_max = log_normal_quantile;
    }

    resultat.delta = 1.0f / std::ceil(1.0f / resultat.rayon_max);
    resultat.delta_inv = 1.0f / resultat.delta;

    return resultat;
}

static void init_lambdas(const ParamatresGrain &params, float lambdas[MAX_NIVEAU_GRIS + 1])
{
    auto const poids_lambda = (constantes<float>::PI * (params.rayon_max * params.rayon_max +
                                                        params.sigma * params.sigma));
    for (auto i = 0; i <= MAX_NIVEAU_GRIS; ++i) {
        auto const u = static_cast<float>(i) / static_cast<float>(MAX_NIVEAU_GRIS);
        auto const ag = 1.0f / std::ceil(1.0f / params.rayon_max);
        auto const lambda_tmp = -((ag * ag) / poids_lambda) * std::log(1.0f - u);
        lambdas[i] = lambda_tmp;
    }
}

struct DonneesCanal {
    int largeur = 0;
    int hauteur = 0;
    const float *donnees_entree = nullptr;
    float *donnees_sortie = nullptr;

    ParamatresGrain params;
};

static inline long calcule_index(DonneesCanal &image, int i, int j)
{
    return j * image.largeur + i;
}

static inline float valeur_entree(DonneesCanal &image, int i, int j)
{
    auto index = calcule_index(image, i, j);
    return image.donnees_entree[index];
}

static unsigned int poisson(const float u, const float lambda)
{
    /* Inverse transform sampling */
    auto prod = std::exp(-lambda);

    auto somme = prod;
    auto x = 0u;

    while ((u > somme) && (static_cast<float>(x) < std::floor(10000.0f * lambda))) {
        x = x + 1u;
        prod = prod * lambda / static_cast<float>(x);
        somme = somme + prod;
    }

    return x;
}

static float simule_grain_pour_coordonnees(DonneesCanal &canal,
                                           GNA &gna_local,
                                           const float *lambdas,
                                           const float gaussien_x,
                                           const float gaussien_y,
                                           int k)
{
    auto const &params = canal.params;
    auto const rayon_max = params.rayon_max;
    auto const delta = params.delta_inv;
    auto const delta_inv = params.delta_inv;
    auto const sigma = params.sigma;
    auto const rayon_carre = params.rayon_carre;
    auto const mu = params.mu;

    // obtiens la liste de cellule couvrant les balles (ig, jg)
    auto const min_x = std::floor((gaussien_x - rayon_max) * delta_inv);
    auto const max_x = std::floor((gaussien_x + rayon_max) * delta_inv);
    auto const min_y = std::floor((gaussien_y - rayon_max) * delta_inv);
    auto const max_y = std::floor((gaussien_y + rayon_max) * delta_inv);

    auto resultat = 0.0f;

    for (int jd = static_cast<int>(min_y); jd <= static_cast<int>(max_y); ++jd) {
        for (int id = static_cast<int>(min_x); id <= static_cast<int>(max_x); ++id) {
            /* coins de la cellule en coordonnées pixel */
            auto coin_x = delta * static_cast<float>(id);
            auto coin_y = delta * static_cast<float>(jd);

            // échantillone image
            auto const u = std::max(
                0.0f, std::min(1.0f, valeur_entree(canal, int(coin_x), int(coin_y))));
            auto const index_u = static_cast<long>(u * MAX_NIVEAU_GRIS);
            auto const lambda = lambdas[index_u];
            auto const Q = poisson(gna_local.uniforme(0.0f, 1.0f), lambda);

            for (unsigned l = 1; l <= Q; ++l) {
                // prend un centre aléatoire d'une distribution uniforme dans un
                // carré ([id, id+1), [jd, jd+1))
                auto const grain_x = coin_x + gna_local.uniforme(0.0f, 1.0f) * delta;
                auto const grain_y = coin_y + gna_local.uniforme(0.0f, 1.0f) * delta;
                auto const dx = grain_x - gaussien_x;
                auto const dy = grain_y - gaussien_y;

                float rayon_courant;
                float rayon_courant2;
                if (sigma > 0.0f) {
                    rayon_courant = std::min(std::exp(mu + sigma * gna_local.uniforme(0.0f, 1.0f)),
                                             rayon_max);
                    rayon_courant2 = rayon_courant * rayon_courant;
                }
                else if (sigma == 0.0f) {
                    rayon_courant2 = rayon_carre;
                }

                if ((dy * dy + dx * dx) < rayon_courant2) {
                    resultat += 1.0f;
                    return resultat;
                }
            }
        }
    }

    return resultat;
}

static void simule_grain_image(DonneesCanal &image,
                               const unsigned int graine,
                               const int iterations)
{
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    auto gna = GNA(graine);

    /* précalcul des lambdas */
    float lambdas[MAX_NIVEAU_GRIS + 1];
    init_lambdas(image.params, lambdas);

    /* précalcul des gaussiens */

    auto const &params = image.params;
    auto const sigma_filtre = params.sigma_filtre;

    dls::tableau<float> liste_gaussien_x(iterations);
    dls::tableau<float> liste_gaussien_y(iterations);

    for (auto i = 0; i < iterations; ++i) {
        liste_gaussien_x[i] = gna.normale(0.0f, sigma_filtre);
        liste_gaussien_y[i] = gna.normale(0.0f, sigma_filtre);
    }

    boucle_parallele(tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
        auto gna_local = GNA(static_cast<unsigned long>(graine + plage.begin()));

        for (int j = plage.begin(); j < plage.end(); ++j) {
            for (int i = 0; i < res_x; ++i) {
                auto grain = 0.0f;

                for (auto k = 0; k < iterations; ++k) {
                    // décalage aléatoire d'une distribution gaussienne centrée de variance sigma^2
                    auto gaussien_x = static_cast<float>(i) + sigma_filtre * liste_gaussien_x[k];
                    auto gaussien_y = static_cast<float>(j) + sigma_filtre * liste_gaussien_y[k];

                    grain += simule_grain_pour_coordonnees(
                        image, gna_local, lambdas, gaussien_x, gaussien_y, k);
                }

                auto index = calcule_index(image, i, j);
                image.donnees_sortie[index] = grain / static_cast<float>(iterations);
            }
        }
    });
}

void simule_grain(const ParametresSimulationGrain &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie)
{
    auto const nombre_de_calques = entree.nombre_de_calques(&entree);

    ParamatresGrain parametres_grain_pour_calque[3] = {
        calcule_parametres(params.rayon_r, params.sigma_r, params.sigma_filtre_r),
        calcule_parametres(params.rayon_v, params.sigma_v, params.sigma_filtre_v),
        calcule_parametres(params.rayon_b, params.sigma_b, params.sigma_filtre_b),
    };

    dls::tableau<DonneesCanal> canaux;

    DescriptionImage desc;
    entree.decris_image(&entree, &desc);

    /* Crée les calques de sorties. */
    for (auto i = 0; i < nombre_de_calques; i++) {
        auto const calque_entree = entree.calque_pour_index(&entree, i);

        char *ptr_nom;
        long taille_nom;
        entree.nom_calque(&entree, calque_entree, &ptr_nom, &taille_nom);

        auto calque_sortie = sortie.cree_calque(&sortie, ptr_nom, taille_nom);

        auto const nombre_de_canaux = std::min(3, entree.nombre_de_canaux(&entree, calque_entree));

        for (auto j = 0; j < nombre_de_canaux; j++) {
            auto const canal_entree = entree.canal_pour_index(&entree, calque_entree, j);
            entree.nom_canal(&entree, canal_entree, &ptr_nom, &taille_nom);

            auto const donnees_canal_entree = entree.donnees_canal_pour_lecture(&entree,
                                                                                canal_entree);

            auto canal_sortie = sortie.ajoute_canal(&sortie, calque_sortie, ptr_nom, taille_nom);

            auto donnees_canal_sortie = sortie.donnees_canal_pour_ecriture(&sortie, canal_sortie);

            DonneesCanal donnees;
            donnees.hauteur = desc.hauteur;
            donnees.largeur = desc.largeur;
            donnees.donnees_entree = donnees_canal_entree;
            donnees.donnees_sortie = donnees_canal_sortie;
            donnees.params = parametres_grain_pour_calque[j];

            canaux.ajoute(donnees);
        }
    }

    for (auto &canal : canaux) {
        simule_grain_image(canal, params.graine, params.iterations);
    }
}

}  // namespace image
