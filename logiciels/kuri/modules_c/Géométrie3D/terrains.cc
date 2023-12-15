/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "terrains.hh"
#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/math/outils.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/collision.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"

#include "wolika/echantillonnage.hh"
#include "wolika/filtre_2d.hh"
#include "wolika/grille_dense.hh"
#include "wolika/outils.hh"

#include "acceleration.hh"
#include "ipa.h"
#include "outils.hh"

namespace geo {

/**
 * Opératrices à considérer :
 * - graphe détail terrain
 * - terrain depuis image satellite
 *
 * VOIR AUSSI
 *
 * Logiciels :
 * - A.N.T. de Blender
 * - Terragen
 * - Vue
 * - WorldMachine
 *
 * https://nickmcd.me/2020/11/23/particle-based-wind-erosion/
 *
 * Publications :
 * - Interactive Example-Based Terrain Authoring with Conditional Generative Adversarial Networks
 *   (https://hal.archives-ouvertes.fr/hal-01583706/file/tog.pdf)
 * - Terrain Generation Using Procedural Models Based on Hydrology
 *   (https://arches.liris.cnrs.fr/publications/articles/SIGGRAPH2013_PCG_Terrain.pdf)
 */

static auto calcul_normal(dls::math::vec3f const &pos, dls::math::vec3f const &pos_voisin)
{
    auto vec = pos_voisin - pos;
    auto perp = produit_croix(vec, dls::math::vec3f(0.0f, 1.0f, 0.0f));
    return normalise(produit_croix(vec, perp));
}

static auto echantillonne_position(wlk::grille_dense_2d<float> const &grille,
                                   dls::math::vec2f const &p)
{
    auto h = wlk::echantillonne_lineaire(grille, p.x, p.y);
    return dls::math::vec3f(p.x, h, p.y);
}

static auto calcul_normal4(wlk::grille_dense_2d<float> const &grille, dls::math::vec2f const &p)
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

static auto calcul_normal8(wlk::grille_dense_2d<float> const &grille, dls::math::vec2f const &p)
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

static auto calcul_normal(wlk::grille_dense_2d<float> const &grille, dls::math::vec2f const &p)
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

#if 0
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
#endif

/* ************************************************************************** */

static wlk::desc_grille_2d descripteur_terrain(AdaptriceTerrain &terrain)
{
    wlk::desc_grille_2d résultat;
    terrain.accede_resolution(&terrain, &résultat.resolution.x, &résultat.resolution.y);

#if 0
    résultat.etendue.max.x = static_cast<float>(résultat.resolution.x);
    résultat.etendue.max.y = static_cast<float>(résultat.resolution.y);
    résultat.type_donnees = wlk::type_grille::R32;
    résultat.taille_pixel = 1.0;
#else

    dls::math::vec3f position;
    terrain.accede_position(&terrain, &position.x, &position.y, &position.z);

    dls::math::vec2f taille;
    terrain.accede_taille(&terrain, &taille.x, &taille.y);

    résultat.etendue.min.x = position.x - (taille.x * 0.5f);
    résultat.etendue.min.y = position.y - (taille.y * 0.5f);
    résultat.etendue.max.x = position.x + (taille.x * 0.5f);
    résultat.etendue.max.y = position.y + (taille.y * 0.5f);
    résultat.taille_pixel = static_cast<double>(taille.x) / résultat.resolution.x;
#endif
    return résultat;
}

static void copie_donnees_calque(AdaptriceTerrain &terrain, wlk::grille_dense_2d<float> &grille)
{
    auto const taille_donnees = grille.taille_octet();

    float *pointeur_donnees;
    terrain.accede_pointeur_donnees(&terrain, &pointeur_donnees);

    memcpy(grille.donnees(), pointeur_donnees, taille_donnees);
}

static void copie_donnees_calque(wlk::grille_dense_2d<float> const &grille,
                                 AdaptriceTerrain &terrain)
{
    auto const taille_donnees = grille.taille_octet();

    float *pointeur_donnees;
    terrain.accede_pointeur_donnees(&terrain, &pointeur_donnees);

    memcpy(pointeur_donnees, grille.donnees(), taille_donnees);
}

static void copie_donnees_calque(wlk::grille_dense_2d<float> const &de,
                                 wlk::grille_dense_2d<float> &vers)
{
    auto const taille_donnees = de.taille_octet();
    memcpy(vers.donnees(), de.donnees(), taille_donnees);
}

static dls::math::vec3f vers_vec3f(float *ptr)
{
    return dls::math::vec3f(ptr[0], ptr[1], ptr[2]);
}

static wlk::grille_dense_2d<float> grille_depuis_terrain(AdaptriceTerrain &terrain)
{
    auto desc = descripteur_terrain(terrain);

    auto résultat = wlk::grille_dense_2d<float>(desc);
    copie_donnees_calque(terrain, résultat);
    return résultat;
}

void simule_erosion_vent(ParametresErosionVent &params,
                         AdaptriceTerrain &terrain,
                         AdaptriceTerrain *terrain_pour_facteur)
{
    auto desc = descripteur_terrain(terrain);

    auto grille_entree = wlk::grille_dense_2d<float>(desc);
    copie_donnees_calque(terrain, grille_entree);

    auto grille_poids = static_cast<wlk::grille_dense_2d<float> *>(nullptr);

    if (terrain_pour_facteur) {
        // À FAIRE
        // grille_poids = &terrain->hauteur;
    }

    auto const direction = params.direction;
    /* À FAIRE : trouve d'où viennent ces constantes. */
    auto const erosion_amont = params.erosion_amont * 1.35f;
    auto const erosion_avale = params.erosion_avale * 2.0f;
    auto const repetitions = params.repetitions;

    auto const dir = direction * constantes<float>::TAU;

    auto const inv_x = 1.0f / static_cast<float>(desc.resolution.x);

    /* Direction où va le vent. */
    auto const direction_avale = dls::math::vec2f(std::cos(dir), std::sin(dir)) * inv_x;

    /* Direction d'où vient le vent. */
    auto const direction_amont = -direction_avale;
    auto temp = wlk::grille_dense_2d<float>(desc);

    for (auto r = 0; r < repetitions; ++r) {
        copie_donnees_calque(grille_entree, temp);

        boucle_parallele(
            tbb::blocked_range<int>(0, desc.resolution.y),
            [&](tbb::blocked_range<int> const &plage) {
                for (auto y = plage.begin(); y < plage.end(); ++y) {
                    for (auto x = 0; x < desc.resolution.x; ++x) {
                        auto index = grille_entree.calcul_index(dls::math::vec2i{x, y});
                        auto const pos_monde = grille_entree.index_vers_unit(
                            dls::math::vec2i(x, y));

                        auto const hauteur = grille_entree.valeur(index);

                        /* Position où va le vent. */
                        auto const position_avale = pos_monde + direction_avale;

                        auto hauteur_avale = wlk::echantillonne_lineaire(
                            temp, position_avale.x, position_avale.y);
                        hauteur_avale = std::min(hauteur_avale, hauteur);

                        /* Position d'où vient le vent. */
                        auto const position_amont = pos_monde + direction_amont;

                        auto const hauteur_amont = wlk::echantillonne_lineaire(
                            temp, position_amont.x, position_amont.y);

                        /* Le normal définit la pente et donc la quantité d'érosion. */
                        auto const normal = calcul_normal(temp, pos_monde);
                        // produit_scalaire(normal, dls::math::vec3f(0.0f, 1.0f, 0.0f);
                        auto const facteur = normal.z;

                        /* Nous enlevons ce qui est emporté par le vent. */
                        auto nouvelle_hauteur = dls::math::entrepolation_lineaire(
                            hauteur, hauteur_avale, facteur * erosion_avale);

                        /* Nous ajoutons ce qui est amené par le vent. */
                        nouvelle_hauteur += dls::math::entrepolation_lineaire(
                            hauteur, hauteur_amont, (1.0f - facteur) * erosion_amont);

                        /* Division par deux pour ne pas ajouter plus de matière. */
                        nouvelle_hauteur *= 0.5f;

                        if (grille_poids != nullptr) {
                            auto poids = grille_poids->valeur(index);
                            nouvelle_hauteur = dls::math::entrepolation_lineaire(
                                nouvelle_hauteur,
                                hauteur,
                                dls::math::restreint(poids, 0.0f, 1.0f));
                        }

                        grille_entree.valeur(index, nouvelle_hauteur);
                    }
                }
            });
    }

    /* copie les données */
    copie_donnees_calque(grille_entree, terrain);
}

/* ************************************************************************** */

void incline_terrain(ParametresInclinaisonTerrain const &params, AdaptriceTerrain &terrain)
{
    auto desc = descripteur_terrain(terrain);
    auto temp = grille_depuis_terrain(terrain);

    auto const facteur = params.facteur;
    auto const decalage = params.decalage;
    auto const inverse = params.inverse;

    //		facteur = std::pow(facteur * 2.0f, 10.0f);
    //		decalage = (decalage - 0.5f) * 10.0f;

    float *pointeur_donnees;
    terrain.accede_pointeur_donnees(&terrain, &pointeur_donnees);

    auto index = 0;
    for (auto y = 0; y < desc.resolution.y; ++y) {
        for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
            auto pos_monde = temp.index_vers_unit(dls::math::vec2i(x, y));

            auto normal = calcul_normal(temp, pos_monde);
            // produit_scalaire(normal, dls::math::vec3f(0.0f, 1.0f, 0.0f);
            auto résultat = normal.z;
            résultat = résultat * facteur + decalage;

            if (inverse) {
                résultat = (1.0f - résultat);
            }

            pointeur_donnees[index] *= résultat;
        }
    }
}

/* ************************************************************************** */

static auto type_filtre_wolika(TypeFiltreTerrain type)
{
    switch (type) {
        case BOITE:
        {
            return wlk::type_filtre::BOITE;
        }
        case TRIANGULAIRE:
        {
            return wlk::type_filtre::TRIANGULAIRE;
        }
        case QUADRATIC:
        {
            return wlk::type_filtre::QUADRATIC;
        }
        case CUBIQUE:
        {
            return wlk::type_filtre::CUBIC;
        }
        case GAUSSIEN:
        {
            return wlk::type_filtre::GAUSSIEN;
        }
        case MITCHELL:
        {
            return wlk::type_filtre::MITCHELL;
        }
        case CATROM:
        {
            return wlk::type_filtre::CATROM;
        }
    }
    return wlk::type_filtre::BOITE;
}

void filtrage_terrain(ParametresFiltrageTerrain const &params, AdaptriceTerrain &terrain)
{
    auto const desc = descripteur_terrain(terrain);
    auto temp = grille_depuis_terrain(terrain);

    wlk::filtre_grille(temp, type_filtre_wolika(params.type), params.rayon);

    copie_donnees_calque(temp, terrain);
}

/* ************************************************************************** */

#if 1
/**
 * Structure et algorithme issus du greffon A.N.T. Landscape de Blender. Le
 * model sous-jacent est expliqué ici :
 * https://blog.michelanders.nl/search/label/erosion
 */
struct erodeuse {
    using type_grille = wlk::grille_dense_2d<float>;
    type_grille &roche;
    wlk::desc_grille_2d desc{};
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

    erodeuse(type_grille &grille_entree) : roche(grille_entree), desc(roche.desc())
    {
    }

    EMPECHE_COPIE(erodeuse);

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
            remplis(eau, 0.0f);
        }

        if (sediment == nullptr) {
            sediment = memoire::loge<type_grille>("grille_erodeuse", desc);
            remplis(sediment, 0.0f);
        }

        if (scour == nullptr) {
            scour = memoire::loge<type_grille>("grille_erodeuse", desc);
            remplis(scour, 0.0f);
        }

        if (flowrate == nullptr) {
            flowrate = memoire::loge<type_grille>("grille_erodeuse", desc);
            remplis(flowrate, 0.0f);
        }

        if (sedimentpct == nullptr) {
            sedimentpct = memoire::loge<type_grille>("grille_erodeuse", desc);
            remplis(sedimentpct, 0.0f);
        }

        if (avalanced == nullptr) {
            avalanced = memoire::loge<type_grille>("grille_erodeuse", desc);
            remplis(avalanced, 0.0f);
        }

        if (capacity == nullptr) {
            capacity = memoire::loge<type_grille>("grille_erodeuse", desc);
            remplis(capacity, 0.0f);
        }
    }

    void peak(float valeur = 1.0f)
    {
        auto nx = desc.resolution.x;
        auto ny = desc.resolution.y;

        // self.center[int(nx/2),int(ny/2)] += value
        this->roche.valeur(dls::math::vec2i(nx / 2, ny / 2)) += valeur;
    }

    void shelf(float valeur = 1.0f)
    {
        auto nx = desc.resolution.x;
        auto ny = desc.resolution.y;
        // self.center[:nx/2] += value

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
        // self.center[nx/4:3*nx/4,ny/4:3*ny/4] += value

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

    void remplis(type_grille *grille, float valeur)
    {
        for (auto i = 0; i < this->roche.nombre_elements(); ++i) {
            grille->valeur(i) = valeur;
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

        // self.maxrss = max(getmemsize(), self.maxrss);

        // this->roche = temp;
        for (auto i = 0; i < this->roche.nombre_elements(); ++i) {
            this->roche.valeur(i) = temp.valeur(i);
        }
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

        // self.maxrss = max(getmemsize(), self.maxrss);

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

                int64_t voisins[4] = {index - 1, index + 1, index - nx, index + nx};

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

                    angle.valeur(index) += std::atan(
                        std::abs(rock.valeur(voisins[i]) - rock.valeur(index)));
                }
            }
        }

        for (auto y = 1; y < ny - 1; ++y) {
            for (auto x = 1; x < nx - 1; ++x) {
                auto index = rock.calcul_index(dls::math::vec2i(x, y));

                auto wcc = this->eau->valeur(index);
                auto scc = this->sediment->valeur(index);
                // auto rcc = rock.valeur(index);

                this->eau->valeur(index) = wcc * (1.0f - Kev) + sdw.valeur(index);
                this->sediment->valeur(index) = scc + sds.valeur(index);

                if (wcc > 0.0f) {
                    sc.valeur(index) = scc / wcc;
                }
                else {
                    sc.valeur(index) = 2.0f * Kc;
                }

                auto fKc = Kc * svdw.valeur(index);
                auto ds = ((fKc > sc.valeur(index)) ? (fKc - sc.valeur(index)) * Ks :
                                                      (fKc - sc.valeur(index)) * Kdep) *
                          wcc;

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

    void generation_riviere(float quantite_pluie,
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

    void erosion_fluviale(float Kc, float Ks, float Kdep, float Ka)
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

void erosion_simple(ParametresErosionSimple const &params,
                    AdaptriceTerrain &terrain,
                    AdaptriceTerrain *grille_poids)
{
    auto grille_entree = grille_depuis_terrain(terrain);
    auto desc = grille_entree.desc();
    auto temp = wlk::grille_dense_2d<float>(desc);
    auto inverse = params.inverse;
    auto superficielle = params.superficielle;
    auto rugueux = params.rugueux;
    auto pente = params.pente;
    auto const iterations = params.iterations;

    auto const res_x = desc.resolution.x;
    auto const res_y = desc.resolution.y;

    auto const s = 1.0f / static_cast<float>(res_x);
    auto const t = 1.0f / static_cast<float>(res_y);

    for (auto r = 0; r < iterations; ++r) {
        copie_donnees_calque(grille_entree, temp);

        boucle_parallele(
            tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
                for (auto y = plage.begin(); y < plage.end(); ++y) {
                    for (auto x = 0; x < desc.resolution.x; ++x) {
                        auto index = grille_entree.calcul_index(dls::math::vec2i{x, y});
                        auto pos_monde = grille_entree.index_vers_unit(dls::math::vec2i(x, y));

                        auto uv = pos_monde;

                        auto centre = wlk::echantillonne_lineaire(temp, uv.x, uv.y);
                        auto gauche = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y);
                        auto droit = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y);
                        auto haut = wlk::echantillonne_lineaire(temp, uv.x, uv.y + t);
                        auto bas = wlk::echantillonne_lineaire(temp, uv.x, uv.y - t);
                        auto haut_gauche = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y + t);
                        auto haut_droit = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y + t);
                        auto bas_gauche = wlk::echantillonne_lineaire(temp, uv.x - s, uv.y - t);
                        auto bas_droit = wlk::echantillonne_lineaire(temp, uv.x + s, uv.y - t);

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
                            auto normal = normalise(
                                dls::math::vec3f(gauche - droit, s + t, bas - haut));

                            float factor = normal.z;  // normal . up

                            if (superficielle) {
                                factor = 1.0f - factor;
                            }
                            else {
                                factor = factor - 0.05f * count;
                            }

                            result = dls::math::entrepolation_lineaire(
                                sum / count, centre, factor);
                        }
                        else {
                            result = sum / count;
                        }

                        if (grille_poids != nullptr) {
                            // À FAIRE
                            //                    auto poids = grille_poids->valeur(index);
                            //                    result = dls::math::entrepolation_lineaire(
                            //                        result, centre, dls::math::restreint(poids,
                            //                        0.0f, 1.0f));
                        }

                        grille_entree.valeur(index, result);
                    }
                }
            });
    }

    copie_donnees_calque(grille_entree, terrain);
}

void erosion_complexe(ParametresErosionComplexe &params, AdaptriceTerrain &terrain)
{
    auto IterRiver = params.iterations_rivieres;
    auto IterAvalanche = params.iterations_avalanche;
    auto IterDiffuse = params.iterations_diffusion;
    // auto Ef = evalue_decimal("pluie_plaines");
    auto Kd = params.diffusion_thermale;
    auto Kt = params.angle_talus;
    auto Kr = params.quantite_pluie;
    auto Kv = params.variance_pluie;
    auto Ks = params.permea_sol;
    auto Kdep = params.taux_sedimentation;
    auto Kz = params.taux_fluvial;
    auto Kc = params.cap_trans;
    auto Ka = params.dep_pente;
    auto Kev = params.evaporation;
    // auto Pd = evalue_decimal("quant_diff");
    auto Pa = params.quantite_avale;
    // auto Pw = evalue_decimal("quant_riv");

    // À FAIRE : mappe de pluie, soit par image, soit par peinture sur le terrain
    // À FAIRE : termine ceci
    // NOTE : le greffon de Blender avait également des options pour montrer des stats sur les
    // itérations et les maillages

    auto grille_entree = grille_depuis_terrain(terrain);
    auto modele = erodeuse(grille_entree);

    for (auto i = 0; i < params.iterations; ++i) {
        for (auto j = 0; j < IterRiver; ++j) {
            modele.generation_riviere(Kr, Kv, Kc, Ks, Kdep, Ka, Kev);
        }

        if (Kd > 0.0f) {
            for (auto j = 0; j < IterDiffuse; ++j) {
                modele.diffuse(Kd / 5.0f, IterDiffuse);
            }
        }

        if (Kt < 90.0f && Pa > 0.0f) {
            for (auto j = 0; j < IterAvalanche; ++j) {
                auto Kt_grad = dls::math::degre_vers_radian(Kt);
                // si dx et dy sont = 1, tan(Kt) est la hauteur pour un
                // angle donné, sinon ce sera tan(Kt)/dx
                modele.avalanche(std::tan(Kt_grad), IterAvalanche, Pa);
            }
        }

        if (Kz > 0.0f) {
            modele.erosion_fluviale(Kc, Ks, Kz * 50, Ka);
        }
    }

    copie_donnees_calque(grille_entree, terrain);
}
#endif

struct DelegueTraverse {
    Maillage const &maillage;

    mutable dls::phys::esectd entresection{};

    bool utilise_touche_la_plus_eloignee = false;

    dls::phys::esectd intersecte_element(int index, dls::phys::rayond const &rayon) const
    {
        auto nombre_de_sommets = maillage.nombreDeSommetsPolygone(index);
        kuri::tableau<int> temp_access_index_sommet;
        temp_access_index_sommet.redimensionne(nombre_de_sommets);

        maillage.indexPointsSommetsPolygone(index, temp_access_index_sommet.donnees());

        auto cos = kuri::tableau<dls::math::vec3f>();
        cos.redimensionne(nombre_de_sommets);
        for (int64_t j = 0; j < nombre_de_sommets; j++) {
            cos[j] = maillage.pointPourIndex(temp_access_index_sommet[j]);
        }

        auto v0 = dls::math::converti_type_point<double>(cos[0]);
        auto v1 = dls::math::converti_type_point<double>(cos[1]);

        auto touche = false;
        auto distance = 0.0;

        for (int64_t j = 2; j < nombre_de_sommets; j++) {
            auto v2 = dls::math::converti_type_point<double>(cos[j]);

            if (entresecte_triangle(v0, v1, v2, rayon, distance)) {
                touche = true;
                break;
            }

            v1 = v2;
        }

        auto résultat = dls::phys::esectd();
        résultat.touche = touche;
        résultat.distance = distance;
        ajourne_entresection(résultat);
        return résultat;
    }

    void ajourne_entresection(dls::phys::esectd const &esect) const
    {
        if (!esect.touche) {
            return;
        }

        if (!entresection.touche) {
            entresection = esect;
            return;
        }

        if (utilise_touche_la_plus_eloignee) {
            entresection.distance = std::max(entresection.distance, esect.distance);
        }
        else {
            entresection.distance = std::min(entresection.distance, esect.distance);
        }
    }
};

void projette_geometrie_sur_terrain(ParametresProjectionTerrain const &params,
                                    AdaptriceTerrain &terrain,
                                    Maillage const &geometrie)
{
    auto hbe = cree_hierarchie_boite_englobante(geometrie);

    if (!hbe) {
        return;
    }

    auto grille = grille_depuis_terrain(terrain);
    auto desc = grille.desc();

    DelegueTraverse delegue{geometrie};
    delegue.utilise_touche_la_plus_eloignee = params.utilise_touche_la_plus_eloignee;

    auto index = 0l;
    for (auto y = 0; y < desc.resolution.y; ++y) {
        for (auto x = 0; x < desc.resolution.x; ++x, ++index) {
            auto const pos_monde = grille.index_vers_monde(dls::math::vec2i(x, y));
            auto const point_monde = dls::math::converti_type_point<double>(pos_monde);

            auto const z = static_cast<double>(grille.valeur(index));

            dls::phys::rayond rayon;
            rayon.origine = dls::math::point3d(point_monde.x, point_monde.y, z);
            rayon.direction = dls::math::vec3d(0.0, 0.0, 1.0);

            delegue.entresection = {};

            traverse(hbe, delegue, rayon);

            if (!delegue.entresection.touche) {
                continue;
            }

            auto distance = delegue.entresection.distance;

            if (distance > params.distance_max) {
                distance = params.distance_max;
            }

            auto valeur = std::max(distance, z);

            grille.valeur(index, static_cast<float>(valeur));
        }
    }

    copie_donnees_calque(grille, terrain);
}

}  // namespace geo
