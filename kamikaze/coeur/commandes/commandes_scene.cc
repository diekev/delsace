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

#include "commandes_scene.h"

#include "bibliotheques/commandes/commande.h"

#include "kamikaze_main.h"

/* ************************************************************************** */

class CommandeChangementTemps final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		auto const &contexte = main->contexte;
		auto scene = contexte.scene;

		if (donnees.metadonnee == "va_image_debut") {
			scene->currentFrame(scene->startFrame());
			scene->updateForNewFrame(contexte);
		}
		else if (donnees.metadonnee == "joue_en_arriere") {
			contexte.eval_ctx->animation = true;
			contexte.eval_ctx->time_direction = TIME_DIR_BACKWARD;
			scene->notify_listeners(type_evenement::temps | type_evenement::modifie);
		}
		else if (donnees.metadonnee == "pas_en_arriere") {
			contexte.eval_ctx->time_direction = TIME_DIR_BACKWARD;
			scene->currentFrame(scene->currentFrame() - 1);
			scene->updateForNewFrame(contexte);
		}
		else if (donnees.metadonnee == "arrete_animation") {
			contexte.eval_ctx->animation = false;
			scene->notify_listeners(type_evenement::temps | type_evenement::modifie);
		}
		else if (donnees.metadonnee == "pas_en_avant") {
			contexte.eval_ctx->time_direction = TIME_DIR_FORWARD;
			scene->currentFrame(scene->currentFrame() + 1);
			scene->updateForNewFrame(contexte);
		}
		else if (donnees.metadonnee == "joue_en_avant") {
			contexte.eval_ctx->animation = true;
			contexte.eval_ctx->time_direction = TIME_DIR_FORWARD;
			scene->notify_listeners(type_evenement::temps | type_evenement::modifie);
		}
		else if (donnees.metadonnee == "va_image_fin") {
			scene->currentFrame(scene->endFrame());
			scene->updateForNewFrame(contexte);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_scene(UsineCommande &usine)
{
	usine.enregistre_type("changement_temps",
						   description_commande<CommandeChangementTemps>(
							   "scene", 0, 0, 0, false));
}
