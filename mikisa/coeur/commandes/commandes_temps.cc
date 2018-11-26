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

#include "commandes_temps.h"

#include "bibliotheques/commandes/commande.h"

#include "../composite.h"
#include "../evenement.h"
#include "../mikisa.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeChangementTemps final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);

		if (donnees.metadonnee == "va_image_debut") {
			mikisa->temps_courant = mikisa->temps_debut;

			mikisa->ajourne_pour_nouveau_temps();
		}
		else if (donnees.metadonnee == "joue_en_arriere") {
			//poseidon->animation = true;
			return EXECUTION_COMMANDE_REUSSIE;
		}
		else if (donnees.metadonnee == "pas_en_arriere") {
			mikisa->temps_courant = mikisa->temps_courant - 1;

			if (mikisa->temps_courant < mikisa->temps_debut) {
				mikisa->temps_courant = mikisa->temps_fin;
			}

			mikisa->ajourne_pour_nouveau_temps();
		}
		else if (donnees.metadonnee == "arrete_animation") {
			/* Évite d'envoyer des évènements inutiles. */
			if (mikisa->animation == false) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

			mikisa->animation = false;
		}
		else if (donnees.metadonnee == "pas_en_avant") {
			mikisa->temps_courant = mikisa->temps_courant + 1;

			if (mikisa->temps_courant > mikisa->temps_fin) {
				mikisa->temps_courant = mikisa->temps_debut;
			}

			mikisa->ajourne_pour_nouveau_temps();
		}
		else if (donnees.metadonnee == "joue_en_avant") {
			/* Évite d'envoyer des évènements inutiles. */
			if (mikisa->animation == true) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

			mikisa->animation = true;
		}
		else if (donnees.metadonnee == "va_image_fin") {
			mikisa->temps_courant = mikisa->temps_fin;

			mikisa->ajourne_pour_nouveau_temps();
		}

		mikisa->notifie_auditeurs(type_evenement::temps | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_temps(UsineCommande *usine)
{
	usine->enregistre_type("changement_temps",
						   description_commande<CommandeChangementTemps>(
							   "scene", 0, 0, 0, false));
}

#pragma clang diagnostic pop
