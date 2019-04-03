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

#include <unistd.h>
#include <chrono>

#include "bibliotheques/commandes/commande.h"

#include "bibloc/logeuse_memoire.hh"

#include "../composite.h"
#include "../evenement.h"
#include "../mikisa.h"
#include "../tache.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static auto compte_tick_ms()
{
	auto const maintenant = std::chrono::system_clock::now();
	auto const duree = maintenant.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duree).count();
}

static void anime_image(Mikisa *mikisa)
{
	auto const IMAGE_PAR_SECONDES = mikisa->cadence;
	auto const TICKS_PAR_IMAGE = static_cast<int>(1000.0 / IMAGE_PAR_SECONDES);

	auto ticks_courant = compte_tick_ms();

	while (mikisa->animation) {
		mikisa->ajourne_pour_nouveau_temps("thread animation");

		ticks_courant += TICKS_PAR_IMAGE;
		auto temps_sommeil = (ticks_courant - compte_tick_ms());

		if (temps_sommeil > 0) {
			usleep(static_cast<unsigned>(temps_sommeil * 1000));
		}

		/* notifie depuis le thread principal. */
		mikisa->notifiant_thread->signal_proces(type_evenement::temps | type_evenement::modifie);

		auto value = mikisa->temps_courant;
		++value;

		if (value > mikisa->temps_fin) {
			value = mikisa->temps_debut;
		}

		mikisa->temps_courant = value;
	}
}

class CommandeChangementTemps final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		if (donnees.metadonnee == "va_image_debut") {
			mikisa->temps_courant = mikisa->temps_debut;

			mikisa->ajourne_pour_nouveau_temps("controle image début");
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

			mikisa->ajourne_pour_nouveau_temps("controle pas en arrière");
		}
		else if (donnees.metadonnee == "arrete_animation") {
			/* Évite d'envoyer des évènements inutiles. */
			if (mikisa->animation == false) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

			mikisa->animation = false;
			mikisa->thread_animation->join();
			memoire::deloge("thread_animation", mikisa->thread_animation);
		}
		else if (donnees.metadonnee == "pas_en_avant") {
			mikisa->temps_courant = mikisa->temps_courant + 1;

			if (mikisa->temps_courant > mikisa->temps_fin) {
				mikisa->temps_courant = mikisa->temps_debut;
			}

			mikisa->ajourne_pour_nouveau_temps("controle pas en avant");
		}
		else if (donnees.metadonnee == "joue_en_avant") {
			/* Évite d'envoyer des évènements inutiles. */
			if (mikisa->animation == true) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

			mikisa->thread_animation = memoire::loge<std::thread>("thread_animation", anime_image, mikisa);
			mikisa->animation = true;
			return EXECUTION_COMMANDE_REUSSIE;
		}
		else if (donnees.metadonnee == "va_image_fin") {
			mikisa->temps_courant = mikisa->temps_fin;

			mikisa->ajourne_pour_nouveau_temps("controle va image fin");
		}

		mikisa->notifie_observatrices(type_evenement::temps | type_evenement::modifie);

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

#pragma clang diagnostic pop
