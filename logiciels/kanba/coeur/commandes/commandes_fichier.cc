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

#include "commandes_fichier.h"

#include "biblinternes/objets/import_objet.h"

#include "adaptrice_creation_maillage.h"
#include "commande_kanba.hh"

#include "../evenement.h"
#include "../kanba.h"
#include "../maillage.h"
#include "../sauvegarde.hh"

/* ************************************************************************** */

class CommandeOuvrirFichier : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        auto const chemin_projet = kanba.requiers_dialogue(KNB::FICHIER_OUVERTURE);

        if (chemin_projet.est_vide()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        KNB::Maillage *maillage = new KNB::Maillage();

        AdaptriceCreationMaillage adaptrice;
        adaptrice.maillage = maillage;

        objets::charge_fichier_OBJ(&adaptrice, chemin_projet);

        kanba.installe_maillage(maillage);
        kanba.notifie_observatrices(static_cast<KNB::TypeÉvènement>(-1));

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeOuvrirProjet : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        auto const chemin_projet =
            "/home/kevin/test.cnvs";  // kanba->requiers_dialogue(FICHIER_OUVERTURE);

        //		if (chemin_projet.est_vide()) {
        //			return;
        //		}

        if (!KNB::lis_projet(kanba, chemin_projet)) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        kanba.notifie_observatrices(KNB::TypeÉvènement::PROJET | KNB::TypeÉvènement::CHARGÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeSauvegarderProjet : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        if (kanba.maillage == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto const chemin_projet =
            "/home/kevin/test.cnvs";  // kanba->requiers_dialogue(FICHIER_OUVERTURE);

        //		if (chemin_projet.est_vide()) {
        //			return;
        //		}

        KNB::écris_projet(kanba, chemin_projet);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_fichier(UsineCommande &usine)
{
    usine.enregistre_type("ouvrir_fichier",
                          description_commande<CommandeOuvrirFichier>("", 0, 0, 0, false));

    usine.enregistre_type("ouvrir_projet",
                          description_commande<CommandeOuvrirProjet>("", 0, 0, 0, false));

    usine.enregistre_type("sauvegarder_projet",
                          description_commande<CommandeSauvegarderProjet>("", 0, 0, 0, false));
}
