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

#include <fstream>

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/objets/import_objet.h"

#include "../evenement.h"
#include "../koudou.h"
#include "../maillage.h"
#include "../nuanceur.h"

#include "adaptrice_creation_maillage.h"

/* ************************************************************************** */

class CommandeOuvrirFichier : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto koudou = std::any_cast<kdo::Koudou *>(pointeur);
		auto const chemin_projet = koudou->requiers_dialogue(FICHIER_OUVERTURE);

		if (chemin_projet.est_vide()) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto maillage = new kdo::Maillage();

		AdaptriceChargementMaillage adaptrice;
		adaptrice.maillage = maillage;

		objets::charge_fichier_OBJ(&adaptrice, chemin_projet);

		maillage->nuanceur(kdo::NuanceurDiffus::defaut());

		koudou->parametres_rendu.scene.ajoute_maillage(maillage);

		koudou->notifie_observatrices(type_evenement::objet | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_fichier(UsineCommande &usine)
{
	usine.enregistre_type("ouvrir_fichier",
						   description_commande<CommandeOuvrirFichier>(
							   "", 0, 0, 0, false));
}
