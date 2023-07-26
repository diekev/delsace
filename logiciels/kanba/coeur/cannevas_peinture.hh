/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/liste.hh"
#include "biblinternes/structures/tableau.hh"

#include "outils.hh"

namespace KNB {

struct Kanba;

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
  private:
    /* La position du texel sur l'écran. */
    dls::math::point2f m_pos{};

    /* L'index du polygone possédant le texel. */
    int64_t m_index{};

    /* La position u du texel. */
    uint32_t m_u{};

    /* La position v du texel. */
    uint32_t m_v{};

  public:
    TexelProjete() = default;

    TexelProjete(dls::math::point2f pos, int64_t index, uint32_t u, uint32_t v)
        : m_pos(pos), m_index(index), m_u(u), m_v(v)
    {
    }

    DEFINIS_ACCESSEUR_MEMBRE(dls::math::point2f, pos);
    DEFINIS_ACCESSEUR_MEMBRE(int64_t, index);
    DEFINIS_ACCESSEUR_MEMBRE(uint32_t, u);
    DEFINIS_ACCESSEUR_MEMBRE(uint32_t, v);
};

struct Seau {
  private:
    dls::liste<TexelProjete> m_texels = dls::liste<TexelProjete>{};

    /* Position du seau sur le cannevas. */
    int m_x = 0;
    int m_y = 0;

    /* Taille du seau. */
    int m_largeur = 0;
    int m_hauteur = 0;

  public:
    Seau() = default;

    Seau(int x, int y, int largeur, int hauteur)
        : m_x(x), m_y(y), m_largeur(largeur), m_hauteur(hauteur)
    {
    }

    void réinitialise()
    {
        m_texels.efface();
    }

    void ajoute_texel(TexelProjete texel)
    {
        m_texels.ajoute(texel);
    }

    DEFINIS_ACCESSEUR_MEMBRE(int, x);
    DEFINIS_ACCESSEUR_MEMBRE(int, y);
    DEFINIS_ACCESSEUR_MEMBRE(int, largeur);
    DEFINIS_ACCESSEUR_MEMBRE(int, hauteur);
    DEFINIS_ACCESSEUR_MEMBRE_TABLEAU(dls::liste<TexelProjete>, texels);
};

class CannevasPeinture {
    Kanba &m_kanba;

    /* Identifiant pour le dessin. */
    int m_id = 0;

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

    int id() const
    {
        return m_id;
    }

  private:
    void construit_seaux();

    void remplis_seaux_avec_texels_maillage();

    Seau *cherche_seau(
        dls::math::point2f const &pos, int seaux_x, int seaux_y, int largeur, int hauteur);
};

/** \} */

}  // namespace KNB
