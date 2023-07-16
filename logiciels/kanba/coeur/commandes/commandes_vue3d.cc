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

#include "biblinternes/objets/creation.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/liste.hh"
#include "biblinternes/vision/camera.h"

#include "adaptrice_creation_maillage.h"
#include "commande_kanba.hh"

#include "../brosse.h"
#include "../cannevas_peinture.hh"
#include "../evenement.h"
#include "../kanba.h"
#include "../maillage.h"
#include "../melange.h"

/* ************************************************************************** */

class CommandeZoomCamera : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_SI_INTERFACE_VOULUE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        auto const delta = donnees.x;

        auto camera = kanba.camera;

        if (delta >= 0) {
            auto distance = camera->distance() + camera->vitesse_zoom();
            camera->distance(distance);
        }
        else {
            const float temp = camera->distance() - camera->vitesse_zoom();
            auto distance = std::max(0.0f, temp);
            camera->distance(distance);
        }

        camera->ajuste_vitesse();
        camera->besoin_ajournement(true);

        auto cannevas = kanba.cannevas;
        cannevas->invalide_pour_changement_caméra();

        kanba.notifie_observatrices(static_cast<KNB::type_evenement>(-1));

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
        auto camera = kanba.camera;

        const float dx = (donnees.x - m_vieil_x);
        const float dy = (donnees.y - m_vieil_y);

        camera->tete(camera->tete() + dy * camera->vitesse_chute());
        camera->inclinaison(camera->inclinaison() + dx * camera->vitesse_chute());
        camera->besoin_ajournement(true);

        auto cannevas = kanba.cannevas;
        cannevas->invalide_pour_changement_caméra();

        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;

        kanba.notifie_observatrices(static_cast<KNB::type_evenement>(-1));
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
        auto camera = kanba.camera;

        const float dx = (donnees.x - m_vieil_x);
        const float dy = (donnees.y - m_vieil_y);

        auto cible = (dy * camera->haut() - dx * camera->droite()) * camera->vitesse_laterale();
        camera->cible(camera->cible() + cible);
        camera->besoin_ajournement(true);

        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;

        auto cannevas = kanba.cannevas;
        cannevas->invalide_pour_changement_caméra();

        kanba.notifie_observatrices(static_cast<KNB::type_evenement>(-1));
    }
};

/* ************************************************************************** */

class CommandePeinture3D : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        if (kanba.maillage == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto maillage = kanba.maillage;
        auto calque = maillage->calque_actif();

        if (calque == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto brosse = kanba.brosse;
        auto cannevas = kanba.cannevas;
        cannevas->ajourne_pour_peinture();

        auto pos_brosse = dls::math::point2f(donnees.x, donnees.y);
        auto tampon = static_cast<dls::math::vec4f *>(calque->tampon);

        auto const &rayon_inverse = 1.0f / static_cast<float>(brosse->rayon);

#undef DEBUG_TOUCHES_SEAUX
#ifdef DEBUG_TOUCHES_SEAUX
        int seaux_non_vides = 0;
        int seaux_touchés = 0;
#endif

        for (auto const &seau : cannevas->seaux()) {
            if (seau.texels.est_vide()) {
                continue;
            }

#ifdef DEBUG_TOUCHES_SEAUX
            seaux_non_vides += 1;
#endif

            // std::cerr << "Il y a " << seau.texels.taille() << " texels dans le seau !\n";

#ifdef DEBUG_TOUCHES_SEAUX
            bool texel_modifié = false;
#endif
            for (auto const &texel : seau.texels) {
                auto dist = longueur(texel.pos - pos_brosse);

                if (dist > static_cast<float>(brosse->rayon)) {
                    continue;
                }

#ifdef DEBUG_TOUCHES_SEAUX
                texel_modifié = true;
#endif

                auto opacite = dist * rayon_inverse;
                opacite = 1.0f - opacite * opacite;

                auto poly = maillage->polygone(texel.index);
                auto tampon_poly = tampon + (poly->x + poly->y * maillage->largeur_texture());
                auto index = texel.v + texel.u * maillage->largeur_texture();

                tampon_poly[index] = melange(tampon_poly[index],
                                             brosse->couleur,
                                             opacite * brosse->opacite,
                                             brosse->mode_fusion);
            }

#ifdef DEBUG_TOUCHES_SEAUX
            seaux_touchés += texel_modifié;
#endif
        }

#ifdef DEBUG_TOUCHES_SEAUX
        std::cerr << "Seaux touchés " << seaux_touchés << " / " << seaux_non_vides
                  << " seaux non vides\n";
#endif

        fusionne_calques(maillage->canaux_texture());

        maillage->marque_texture_surrannee(true);
        kanba.notifie_observatrices(KNB::type_evenement::dessin | KNB::type_evenement::fini);

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
        auto adaptrice = AdaptriceCreationMaillage();
        adaptrice.maillage = new KNB::Maillage;

        objets::cree_boite(&adaptrice, 1.0f, 1.0f, 1.0f);

        kanba.installe_maillage(adaptrice.maillage);
        kanba.notifie_observatrices(KNB::type_evenement::calque | KNB::type_evenement::ajoute);

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
        auto adaptrice = AdaptriceCreationMaillage();
        adaptrice.maillage = new KNB::Maillage;

        objets::cree_sphere_uv(&adaptrice, 1.0f, 48, 24);

        kanba.installe_maillage(adaptrice.maillage);
        kanba.notifie_observatrices(KNB::type_evenement::calque | KNB::type_evenement::ajoute);

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
