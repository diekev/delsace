/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "cannevas_peinture.hh"

#include "biblinternes/vision/camera.h"

#include "brosse.h"
#include "kanba.h"
#include "maillage.h"

/* ------------------------------------------------------------------------- */
/** \name Cannevas Peinture
 * \{ */

CannevasPeinture::CannevasPeinture(Kanba &kanba) : m_kanba(kanba)
{
}

void CannevasPeinture::ajourne_pour_peinture()
{
    construit_seaux();
    remplis_seaux_avec_texels_maillage();
}

void CannevasPeinture::invalide_pour_changement_caméra()
{
    m_besoin_remplissage_seaux = true;
}

void CannevasPeinture::invalide_pour_changement_brosse()
{
    /* À FAIRE : ne dépend pas de la brosse. */
    invalide_pour_changement_taille_écran();
}

void CannevasPeinture::invalide_pour_changement_taille_écran()
{
    m_besoin_reconstruction_seaux = true;
    m_besoin_remplissage_seaux = true;
}

void CannevasPeinture::construit_seaux()
{
    if (!m_besoin_reconstruction_seaux) {
        return;
    }

    auto camera = m_kanba.camera;
    auto brosse = m_kanba.brosse;

    auto const &diametre_brosse = brosse->rayon * 2;

    auto seaux_x = camera->largeur() / diametre_brosse + 1;
    auto seaux_y = camera->hauteur() / diametre_brosse + 1;

    seaux_x = restreint(seaux_x, MIN_SEAUX, MAX_SEAUX);
    seaux_y = restreint(seaux_y, MIN_SEAUX, MAX_SEAUX);

    //		std::cerr << "Il y a " << seaux_x << "x" << seaux_y << " seaux en tout\n";
    //		std::cerr << "Taille écran " << camera->largeur() << "x" << camera->hauteur() <<
    //"\n"; 		std::cerr << "Taille seaux " << seaux_x * diametre_brosse << "x" << seaux_y
    //* diametre_brosse << "\n";

    m_seaux.redimensionne(seaux_x * seaux_y);
    m_seaux_x = seaux_x;
    m_seaux_y = seaux_y;
    for (auto &seau : m_seaux) {
        seau = Seau();
    }

    m_besoin_reconstruction_seaux = false;
}

void CannevasPeinture::remplis_seaux_avec_texels_maillage()
{
    if (!m_besoin_remplissage_seaux) {
        return;
    }

    auto camera = m_kanba.camera;
    auto maillage = m_kanba.maillage;

    if (!maillage) {
        return;
    }

    POUR (m_seaux) {
        it.texels.efface();
    }

    auto nombre_polys = maillage->nombre_polygones();

    auto const &dir = dls::math::vec3f(-camera->dir().x, -camera->dir().y, -camera->dir().z);

    for (auto i = 0; i < nombre_polys; ++i) {
        auto poly = maillage->polygone(i);
        auto const &angle = produit_scalaire(poly->nor, dir);

        // std::cerr << "Angle : " << angle << '\n';

        if (angle <= 0.0f || angle >= 1.0f) {
            // std::cerr << "Le polygone " << poly->index << " ne fait pas face à l'écran !\n";
            continue;
        }

        // std::cerr << "Le polygone " << poly->index << " fait face à l'écran !\n";

        // projette texel sur l'écran
        auto const &v0 = poly->s[0]->pos;

#if 1
        auto const &v1 = poly->s[1]->pos;
        auto const &v3 = ((poly->s[3] != nullptr) ? poly->s[3]->pos : poly->s[2]->pos);

        auto const &e1 = v1 - v0;
        auto const &e2 = v3 - v0;

        auto const &du = e1 / static_cast<float>(poly->res_u);
        auto const &dv = e2 / static_cast<float>(poly->res_v);
#else
        auto const &du = poly->du;
        auto const &dv = poly->dv;
#endif

        for (unsigned j = 0; j < poly->res_u; ++j) {
            for (unsigned k = 0; k < poly->res_v; ++k) {
                auto const &pos3d = v0 + static_cast<float>(j) * du + static_cast<float>(k) * dv;

                // calcul position 2D du texel
                auto const &pos2d = camera->pos_ecran(dls::math::point3f(pos3d));

                // cherche seau
                auto seau = cherche_seau(
                    pos2d, m_seaux_x, m_seaux_y, camera->largeur(), camera->hauteur());

                TexelProjete texel;
                texel.pos = pos2d;
                texel.index = i;
                texel.u = j;
                texel.v = k;

                seau->texels.ajoute(texel);
            }
        }
    }

    m_besoin_remplissage_seaux = false;
}

Seau *CannevasPeinture::cherche_seau(
    const dls::math::point2f &pos, int seaux_x, int seaux_y, int largeur, int hauteur)
{
    auto x = pos.x / static_cast<float>(largeur);
    auto y = pos.y / static_cast<float>(hauteur);

    auto sx = static_cast<float>(seaux_x) * x;
    auto sy = static_cast<float>(seaux_y) * y;

    auto index = static_cast<long>(sx + sy * static_cast<float>(seaux_y));

    index = restreint(index, 0l, m_seaux.taille() - 1);

    return &m_seaux[index];
}

/** \} */
