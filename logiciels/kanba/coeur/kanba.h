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

#pragma once

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/observation.hh"

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

class BaseEditrice;
class Brosse;
class CannevasPeinture;
class FenetrePrincipale;
class Maillage;
class RepondantCommande;
class UsineCommande;

namespace vision {

class Camera3D;

} /* namespace vision */

enum {
    FICHIER_OUVERTURE,
};

struct EntréeLog {
    enum Type {
        GÉNÉRALE,
        IMAGE,
        RENDU,
        MAILLAGE,
        EMPAQUETAGE,
    };

    Type type{};
    dls::chaine texte{};
};

struct Kanba : public Sujette {
    dls::math::matrice_dyn<dls::math::vec4f> tampon;

    /* Interface utilisateur. */
    FenetrePrincipale *fenetre_principale = nullptr;
    BaseEditrice *widget_actif = nullptr;

    UsineCommande usine_commande;

    RepondantCommande *repondant_commande;

    Brosse *brosse;
    vision::Camera3D *camera;
    Maillage *maillage;
    CannevasPeinture *cannevas = nullptr;

    /* Définis si les seaux du cannevas doivent être dessinés sur la vue 3D. */
    bool dessine_seaux = false;

    dls::tableau<EntréeLog> entrées_log{};

    Kanba();
    ~Kanba();

    Kanba(Kanba const &) = default;
    Kanba &operator=(Kanba const &) = default;

    void enregistre_commandes();

    dls::chaine requiers_dialogue(int type);

    template <typename... Args>
    void ajoute_log(EntréeLog::Type type, Args... args)
    {
        std::stringstream ss;
        ((ss << args), ...);

        ajoute_log_impl(type, ss.str());
    }

    void ajoute_log(EntréeLog::Type type, dls::chaine const &texte);

  private:
    void ajoute_log_impl(EntréeLog::Type type, dls::chaine const &texte);
};
