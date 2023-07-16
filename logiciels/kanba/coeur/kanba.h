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

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

#include "interface_graphique.hh"

class BaseEditrice;
class FenetrePrincipale;
class RepondantCommande;
class UsineCommande;

namespace vision {

class Camera3D;

} /* namespace vision */

namespace KNB {

enum class type_evenement : int;

struct Brosse;
class CannevasPeinture;
class GestionnaireFenetre;
class Maillage;

enum class TypeCurseur : int32_t {
    NORMAL = 0,
    ATTENTE_BLOQUÉ = 1,
    TÂCHE_ARRIÈRE_PLAN_EN_COURS = 2,
    MAIN_OUVERTE = 3,
    MAIN_FERMÉE = 4,
};

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

struct Kanba {
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

  private:
    InterfaceGraphique m_interface_graphique{};

    GestionnaireFenetre *m_gestionnaire_fenêtre = nullptr;

  public:
    Kanba();
    ~Kanba();

    Kanba(Kanba const &) = default;
    Kanba &operator=(Kanba const &) = default;

    void enregistre_commandes();

    dls::chaine requiers_dialogue(int type);

    void installe_gestionnaire_fenêtre(GestionnaireFenetre *gestionnaire);

    void installe_maillage(Maillage *m);

    void notifie_observatrices(type_evenement type);

    const InterfaceGraphique &donne_interface_graphique() const
    {
        return m_interface_graphique;
    }

    /* Interface pour l'interface graphique. À FAIRE. */

    void restaure_curseur_application();
    void change_curseur_application(KNB::TypeCurseur curseur);

    /* Interface pour l'historique. À FAIRE. */

    void prépare_pour_changement(dls::chaine const &identifiant);
    void soumets_changement();
    void annule_changement();

    /* Interface pour les logs. */

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

}  // namespace KNB
