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

#include "../evenement.h"
#include "../fluide.h"
#include "../poseidon.h"

/* ************************************************************************** */

class CommandeChangementTemps final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto poseidon = std::any_cast<Poseidon *>(pointeur);
		auto fluide = poseidon->fluide;

		if (donnees.metadonnee == "va_image_debut") {
			fluide->temps_precedent = fluide->temps_courant;
			fluide->temps_courant = fluide->temps_debut;

			fluide->ajourne_pour_nouveau_temps();
		}
		else if (donnees.metadonnee == "joue_en_arriere") {
			//poseidon->animation = true;
			return EXECUTION_COMMANDE_REUSSIE;
		}
		else if (donnees.metadonnee == "pas_en_arriere") {
			fluide->temps_precedent = fluide->temps_courant;
			fluide->temps_courant = fluide->temps_courant - 1;

			if (fluide->temps_courant < fluide->temps_debut) {
				fluide->temps_courant = fluide->temps_fin;
			}

			fluide->ajourne_pour_nouveau_temps();
		}
		else if (donnees.metadonnee == "arrete_animation") {
			/* Évite d'envoyer des évènements inutiles. */
			if (poseidon->animation == false) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

			poseidon->animation = false;
		}
		else if (donnees.metadonnee == "pas_en_avant") {
			fluide->temps_precedent = fluide->temps_courant;
			fluide->temps_courant = fluide->temps_courant + 1;

			if (fluide->temps_courant > fluide->temps_fin) {
				fluide->temps_courant = fluide->temps_debut;
			}

			fluide->ajourne_pour_nouveau_temps();
		}
		else if (donnees.metadonnee == "joue_en_avant") {
			/* Évite d'envoyer des évènements inutiles. */
			if (poseidon->animation == true) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

			poseidon->animation = true;
		}
		else if (donnees.metadonnee == "va_image_fin") {
			fluide->temps_precedent = fluide->temps_courant;
			fluide->temps_courant = fluide->temps_fin;

			fluide->ajourne_pour_nouveau_temps();
		}

		poseidon->notifie_observatrices(type_evenement::temps | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_temps(UsineCommande &usine)
{
	usine.enregistre_type("changement_temps",
						   description_commande<CommandeChangementTemps>(
							   "scene", 0, 0, 0, false));
}
