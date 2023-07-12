/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/liste.hh"
#include "biblinternes/structures/tableau.hh"

class Kanba;

/* ------------------------------------------------------------------------- */
/** \name Cannevas Peinture
 * \{ */

static constexpr auto MIN_SEAUX = 4;
static constexpr auto MAX_SEAUX = 256;

template <typename T>
auto restreint(T a, T min, T max)
{
    if (a < min) {
        return min;
    }

    if (a > max) {
        return max;
    }

    return a;
}

struct TexelProjete {
    /* La position du texel sur l'écran. */
    dls::math::point2f pos{};

    /* L'index du polygone possédant le texel. */
    long index{};

    /* La position u du texel. */
    unsigned int u{};

    /* La position v du texel. */
    unsigned int v{};
};

struct Seau {
    dls::liste<TexelProjete> texels = dls::liste<TexelProjete>{};
    dls::math::vec2f min = dls::math::vec2f(0.0);
    dls::math::vec2f max = dls::math::vec2f(0.0);
};

class CannevasPeinture {
    Kanba &m_kanba;

    dls::tableau<Seau> m_seaux{};
    int m_seaux_x = 0;
    int m_seaux_y = 0;

    bool m_besoin_reconstruction_seaux = true;
    bool m_besoin_remplissage_seaux = true;

  public:
    CannevasPeinture(Kanba &kanba);

    CannevasPeinture(CannevasPeinture const &) = delete;
    CannevasPeinture &operator=(CannevasPeinture const &) = delete;

    void ajourne_pour_peinture();

    void invalide_pour_changement_caméra();

    void invalide_pour_changement_brosse();

    void invalide_pour_changement_taille_écran();

    dls::tableau<Seau> &seaux()
    {
        return m_seaux;
    }

  private:
    void construit_seaux();

    void remplis_seaux_avec_texels_maillage();

    Seau *cherche_seau(
        dls::math::point2f const &pos, int seaux_x, int seaux_y, int largeur, int hauteur);
};

/** \} */
