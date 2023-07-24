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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "commandes_vue3d.h"

#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/objets/creation.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/liste.hh"

#include "adaptrice_creation_maillage.h"
#include "commande_kanba.hh"

#include "../kanba.h"

/* ************************************************************************** */

class CommandeZoomCamera : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_SI_INTERFACE_VOULUE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        auto const delta = donnees.x;

        auto camera = kanba.donne_caméra();
        camera.applique_zoom(delta);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeTourneCamera : public CommandeKanba {
    float m_vieil_x = 0.0f;
    float m_vieil_y = 0.0f;

    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_SI_INTERFACE_VOULUE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        INUTILISE(kanba);
        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;
        return EXECUTION_COMMANDE_MODALE;
    }

    void ajourne_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        const float dx = (donnees.x - m_vieil_x);
        const float dy = (donnees.y - m_vieil_y);

        auto camera = kanba.donne_caméra();
        camera.applique_tourne(dx, dy);

        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;
    }
};

/* ************************************************************************** */

class CommandePanCamera : public CommandeKanba {
    float m_vieil_x = 0.0f;
    float m_vieil_y = 0.0f;

    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_SI_INTERFACE_VOULUE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        INUTILISE(kanba);
        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;
        return EXECUTION_COMMANDE_MODALE;
    }

    void ajourne_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {

        const float dx = (donnees.x - m_vieil_x);
        const float dy = (donnees.y - m_vieil_y);

        auto camera = kanba.donne_caméra();
        camera.applique_pan(dx, dy);

        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;
    }
};

/* ************************************************************************** */

static bool seau_peut_être_peint(KNB::Seau seau,
                                 dls::math::point2f const pos_pinceau,
                                 float const rayon_carré)
{
    if (seau.donne_texels().est_vide()) {
        return false;
    }

    const auto x_min = float(seau.donne_x());
    const auto x_max = float(seau.donne_x() + seau.donne_largeur());
    const auto y_min = float(seau.donne_y());
    const auto y_max = float(seau.donne_y() + seau.donne_hauteur());

    const dls::math::point2f coins_seau[4] = {
        dls::math::point2f(x_min, y_min),
        dls::math::point2f(x_min, y_max),
        dls::math::point2f(x_max, y_min),
        dls::math::point2f(x_max, y_max),
    };

    for (auto coin : coins_seau) {
        auto dist = dls::math::longueur_carree(coin - pos_pinceau);

        if (dist < rayon_carré) {
            return true;
        }
    }

    return false;
}

class CommandePeinture3D : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        auto maillage = kanba.donne_maillage();
        if (maillage == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto canaux = maillage.donne_canaux_texture();
        auto calque = canaux.donne_calque_actif();

        if (calque == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        if (calque.est_verrouillé()) {
            // À FAIRE : message d'erreur dans la barre d'état.
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto pinceau = kanba.donne_pinceau();
        auto cannevas = kanba.donne_canevas();
        cannevas.ajourne_pour_peinture();

        auto pos_pinceau = dls::math::point2f(donnees.x, donnees.y);
        auto tampon = calque.donne_tampon_couleur().données_crues();

        auto const &rayon_inverse = 1.0f / static_cast<float>(pinceau.donne_rayon());
        auto const rayon_carré = float(pinceau.donne_rayon() * pinceau.donne_rayon());
        auto const largeur = canaux.donne_largeur();

        if (kanba.donne_dessine_seaux()) {
            cannevas.réinitialise_drapeaux_peinture_seaux();
        }

        bool polygones_modifiés[6] = {false, false, false, false, false, false};

        auto index_seau = 0;
        for (auto const &seau : cannevas.donne_seaux()) {
            auto texels = seau.donne_texels();

            if (!seau_peut_être_peint(seau, pos_pinceau, rayon_carré)) {
                index_seau += 1;
                continue;
            }

            bool texel_modifié = false;

            for (auto const &texel : texels) {
                auto pos_texel = texel.donne_pos();
                auto pos2f_texel = dls::math::point2f(pos_texel.donne_x(), pos_texel.donne_y());
                auto dist = dls::math::longueur(pos2f_texel - pos_pinceau);

                if (dist > static_cast<float>(pinceau.donne_rayon())) {
                    continue;
                }

                texel_modifié = true;

                polygones_modifiés[texel.donne_index()] = true;

                auto opacite = dist * rayon_inverse;
                opacite = 1.0f - opacite * opacite;

                auto poly = maillage.donne_quadrilatère(texel.donne_index());
                auto tampon_poly = tampon + (poly.donne_x() + poly.donne_y() * largeur);
                auto index = uint32_t(texel.donne_v()) + uint32_t(texel.donne_u()) * largeur;

                //                tampon_poly[index] = KNB::mélange(tampon_poly[index],
                //                                                  pinceau.donne_couleur(),
                //                                                  opacite *
                //                                                  pinceau.donne_opacité(),
                //                                                  pinceau.donne_mode_de_peinture());

                tampon_poly[index] = pinceau.donne_couleur();
            }

#ifdef DEBUG_TOUCHES_SEAUX
            seaux_touchés += texel_modifié;
#endif

            if (kanba.donne_dessine_seaux()) {
                if (texel_modifié) {
                    cannevas.marque_seau_comme_peint(index_seau);
                }
                else {
                    cannevas.marque_seau_comme_touché_par_pinceau(index_seau);
                }
            }

            index_seau += 1;
        }

        for (int i = 0; i < 6; i++) {
            if (!polygones_modifiés[i]) {
                continue;
            }

            std::cerr << "Quad " << i << " modifié !\n";
        }

        maillage.marque_chose_à_recalculer(KNB::ChoseÀRecalculer::CANAL_FUSIONNÉ);
        kanba.notifie_observatrices(KNB::TypeÉvènement::DESSIN | KNB::TypeÉvènement::FINI);

        return EXECUTION_COMMANDE_MODALE;
    }

    void ajourne_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        execute_kanba(kanba, donnees);
    }
};

/* ************************************************************************** */

class CommandeAjouteCube : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        auto maillage = KNB::crée_un_maillage_vide();

        auto adaptrice = AdaptriceCreationMaillage();
        adaptrice.maillage = &maillage;

        objets::cree_boite(&adaptrice, 1.0f, 1.0f, 1.0f);

        kanba.installe_maillage(maillage);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeAjouteSphere : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        auto maillage = KNB::crée_un_maillage_vide();

        auto adaptrice = AdaptriceCreationMaillage();
        adaptrice.maillage = &maillage;

        objets::cree_sphere_uv(&adaptrice, 1.0f, 48, 24);

        kanba.installe_maillage(maillage);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_vue3d(UsineCommande &usine)
{
    usine.enregistre_type(
        "commande_zoom_camera",
        description_commande<CommandeZoomCamera>("vue_3d", Qt::MiddleButton, 0, 0, true));

    usine.enregistre_type(
        "commande_tourne_camera",
        description_commande<CommandeTourneCamera>("vue_3d", Qt::MiddleButton, 0, 0, false));

    usine.enregistre_type("commande_pan_camera",
                          description_commande<CommandePanCamera>(
                              "vue_3d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

    usine.enregistre_type(
        "commande_peinture_3D",
        description_commande<CommandePeinture3D>("vue_3d", Qt::LeftButton, 0, 0, false));

    usine.enregistre_type("ajouter_cube",
                          description_commande<CommandeAjouteCube>("vue_3d", 0, 0, 0, false));

    usine.enregistre_type("ajouter_sphere",
                          description_commande<CommandeAjouteSphere>("vue_3d", 0, 0, 0, false));
}
