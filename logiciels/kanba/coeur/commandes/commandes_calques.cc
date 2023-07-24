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

#include "commandes_calques.h"

#include "commande_kanba.hh"

#include "../kanba.h"

/* ************************************************************************** */

class CommandeAjouterCalque : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        auto maillage = kanba.donne_maillage();

        if (maillage == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto canaux = maillage.donne_canaux_texture();
        canaux.ajoute_un_calque(KNB::TypeCanal::DIFFUSION);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeSupprimerCalque : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const & /*donnees*/) override
    {
        auto maillage = kanba.donne_maillage();

        if (maillage == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto canaux = maillage.donne_canaux_texture();
        auto calque = maillage.donne_calque_actif();

        if (calque == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto calque_actif = KNB::Calque(nullptr);
        maillage.définis_calque_actif(calque_actif);

        canaux.supprime_calque(calque);

        maillage.marque_chose_à_recalculer(KNB::ChoseÀRecalculer::CANAL_FUSIONNÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_calques(UsineCommande &usine)
{
    usine.enregistre_type("ajouter_calque",
                          description_commande<CommandeAjouterCalque>("", 0, 0, 0, false));

    usine.enregistre_type("supprimer_calque",
                          description_commande<CommandeSupprimerCalque>("", 0, 0, 0, false));
}
