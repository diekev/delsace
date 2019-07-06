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

#include "biblinternes/commandes/commande.h"

#include "../evenement.h"
#include "../kanba.h"
#include "../maillage.h"

/* ************************************************************************** */

class CommandeAjouterCalque : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);
		auto maillage = kanba->maillage;

		if (maillage == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto &canaux = maillage->canaux_texture();

		ajoute_calque(canaux, TypeCanal::DIFFUSION);

		kanba->notifie_observatrices(type_evenement::calque | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSupprimerCalque : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);
		auto maillage = kanba->maillage;

		if (maillage == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto &canaux = maillage->canaux_texture();
		auto calque = maillage->calque_actif();

		if (calque == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		maillage->calque_actif(nullptr);

		supprime_calque(canaux, calque);
		fusionne_calques(canaux);

		maillage->marque_texture_surrannee(true);

		kanba->notifie_observatrices(type_evenement::calque | type_evenement::supprime);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_calques(UsineCommande &usine)
{
	usine.enregistre_type("ajouter_calque",
						   description_commande<CommandeAjouterCalque>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("supprimer_calque",
						   description_commande<CommandeSupprimerCalque>(
							   "", 0, 0, 0, false));
}
