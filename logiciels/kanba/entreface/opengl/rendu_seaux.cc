/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "rendu_seaux.hh"

#include <numeric>

#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/kanba.h"

/* ------------------------------------------------------------------------- */
/** \name Génération du tampon de rendu.
 * \{ */

static std::unique_ptr<TamponRendu> cree_tampon(dls::math::vec4f const &couleur,
                                                float taille_ligne)
{
    auto sources = crée_sources_glsl_depuis_fichier("nuanceurs/simple.vert",
                                                    "nuanceurs/simple.frag");
    if (!sources.has_value()) {
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(sources.value());

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);
    parametres_dessin.taille_ligne(taille_ligne);

    tampon->parametres_dessin(parametres_dessin);

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("possede_couleur_sommet", true);
    programme->desactive();

    return tampon;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name RenduSeaux
 * \{ */

RenduSeaux::RenduSeaux(KNB::Kanba &kanba) : m_kanba(kanba)
{
}

void RenduSeaux::initialise()
{
    auto camera = m_kanba.donne_caméra();
    auto const cannevas = m_kanba.donne_canevas();
    auto seaux = cannevas.donne_seaux();

    if (seaux.est_vide()) {
        m_tampon.reset(nullptr);
        return;
    }

    if (m_id_cannevas == cannevas.donne_id()) {
        return;
    }

    m_id_cannevas = cannevas.donne_id();

    m_tampon.reset(nullptr);
    m_tampon = cree_tampon(dls::math::vec4f(1.0f, 1.0f, 0.0f, 1.0f), 1.0f);

    /* 4 sommets par seau, 2 sommets par ligne. */
    auto const nombre_de_points = seaux.taille() * 4;
    dls::tableau<unsigned int> index(nombre_de_points * 2);
    dls::tableau<dls::math::vec3f> sommets(nombre_de_points);

    auto const largeur_inverse_caméra = 1.0f / static_cast<float>(camera.donne_largeur());
    auto const hauteur_inverse_caméra = 1.0f / static_cast<float>(camera.donne_hauteur());

    unsigned int décalage_sommet = 0;
    unsigned int décalage_index = 0;
    for (auto const &seau : seaux) {
        auto seau_x = seau.donne_x();
        auto seau_y = seau.donne_y();
        auto seau_largeur = seau.donne_largeur();
        auto seau_hauteur = seau.donne_hauteur();

        /* Décale de 1 pixel afin de ne pas dessiner les arêtes des carreaux les unes sur les
         * autres. */
        auto px0 = static_cast<float>(seau_x + 1) * largeur_inverse_caméra;
        auto px1 = static_cast<float>(seau_x + 1 + seau_largeur - 2) * largeur_inverse_caméra;
        auto py0 = static_cast<float>(seau_y + 1) * hauteur_inverse_caméra;
        auto py1 = static_cast<float>(seau_y + 1 + seau_hauteur - 2) * hauteur_inverse_caméra;

        px0 = px0 * 2.0f - 1.0f;
        px1 = px1 * 2.0f - 1.0f;
        /* Le 0 des seaux est en haut de l'écran, celuis d'OpenGL en bas. */
        py0 = (1.0f - py0) * 2.0f - 1.0f;
        py1 = (1.0f - py1) * 2.0f - 1.0f;

        auto p0 = dls::math::vec3f(px0, py0, 0.0f);
        auto p1 = dls::math::vec3f(px1, py0, 0.0f);
        auto p2 = dls::math::vec3f(px1, py1, 0.0f);
        auto p3 = dls::math::vec3f(px0, py1, 0.0f);

        index[décalage_index++] = décalage_sommet;
        index[décalage_index++] = décalage_sommet + 1;

        index[décalage_index++] = décalage_sommet + 1;
        index[décalage_index++] = décalage_sommet + 2;

        index[décalage_index++] = décalage_sommet + 2;
        index[décalage_index++] = décalage_sommet + 3;

        index[décalage_index++] = décalage_sommet + 3;
        index[décalage_index++] = décalage_sommet;

        sommets[décalage_sommet++] = p0;
        sommets[décalage_sommet++] = p1;
        sommets[décalage_sommet++] = p2;
        sommets[décalage_sommet++] = p3;
    }

    remplis_tampon_principal(m_tampon.get(), "sommets", sommets, index);

    ajourne_tampon_couleur();
}

void RenduSeaux::ajourne_tampon_couleur()
{
    if (!m_tampon) {
        return;
    }

    auto const cannevas = m_kanba.donne_canevas();
    auto seaux = cannevas.donne_seaux();

    if (seaux.est_vide()) {
        return;
    }

    auto const nombre_de_points = seaux.taille() * 4;
    dls::tableau<dls::math::vec4f> couleurs(nombre_de_points);
    unsigned int décalage_sommet = 0;

    dls::math::vec4f couleur_non_peint(1.0f, 1.0f, 0.0f, 1.0f);
    dls::math::vec4f couleur_peint(1.0f, 0.0f, 0.0f, 1.0f);
    dls::math::vec4f couleur_possede_texels(0.0f, 1.0f, 0.0f, 1.0f);
    dls::math::vec4f couleur_touché_par_pinceau(0.0f, 0.0f, 1.0f, 1.0f);

    for (auto const &seau : seaux) {
        auto couleur = seau.donne_texels().taille() != 0 ? couleur_possede_texels :
                                                           couleur_non_peint;
        couleur = seau.donne_fut_peint() ? couleur_peint : couleur;
        couleur = seau.donne_fut_touché_par_pinceau() ? couleur_touché_par_pinceau : couleur;

        couleurs[décalage_sommet++] = couleur;
        couleurs[décalage_sommet++] = couleur;
        couleurs[décalage_sommet++] = couleur;
        couleurs[décalage_sommet++] = couleur;
    }

    remplis_tampon_extra(m_tampon.get(), "couleur_sommet", couleurs);
}

void RenduSeaux::dessine(ContexteRendu const &contexte)
{
    if (!m_tampon) {
        return;
    }

    m_tampon->dessine(contexte);
}

/** \} */
