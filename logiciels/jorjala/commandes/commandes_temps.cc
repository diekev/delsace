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

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "commande_jorjala.hh"

//#include "coeur/composite.h"
//#include "coeur/evenement.h"
//#include "coeur/tache.h"

#include "coeur/jorjala.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static auto compte_tick_ms()
{
	auto const maintenant = std::chrono::system_clock::now();
	auto const duree = maintenant.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duree).count();
}

static void anime_image(JJL::Jorjala &jorjala)
{
    auto const IMAGE_PAR_SECONDES = jorjala.cadence();
	auto const TICKS_PAR_IMAGE = static_cast<int>(1000.0 / IMAGE_PAR_SECONDES);

	auto ticks_courant = compte_tick_ms();

    while (jorjala.animation_en_cours()) {
        jorjala.ajourne_pour_nouveau_temps("thread animation");

		ticks_courant += TICKS_PAR_IMAGE;
		auto temps_sommeil = (ticks_courant - compte_tick_ms());

		if (temps_sommeil > 0) {
			usleep(static_cast<unsigned>(temps_sommeil * 1000));
		}

		/* notifie depuis le thread principal. */
        //jorjala.notifiant_thread->signal_proces(type_evenement::temps | type_evenement::modifie);

        auto value = jorjala.temps_courant();
		++value;

        if (value > jorjala.temps_fin()) {
            value = jorjala.temps_début();
		}

        jorjala.temps_courant(value);
	}
}

class CommandeChangementTemps final : public CommandeJorjala {
public:
	int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
		if (donnees.metadonnee == "va_image_debut") {
            jorjala.temps_courant(jorjala.temps_début());

            jorjala.ajourne_pour_nouveau_temps("controle image début");
		}
		else if (donnees.metadonnee == "joue_en_arriere") {
			//poseidon->animation = true;
			return EXECUTION_COMMANDE_REUSSIE;
		}
		else if (donnees.metadonnee == "pas_en_arriere") {
            jorjala.temps_courant(jorjala.temps_courant() - 1);

            if (jorjala.temps_courant() < jorjala.temps_début()) {
                jorjala.temps_courant(jorjala.temps_fin());
			}

            jorjala.ajourne_pour_nouveau_temps("controle pas en arrière");
		}
		else if (donnees.metadonnee == "arrete_animation") {
			/* Évite d'envoyer des évènements inutiles. */
            if (jorjala.animation_en_cours() == false) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

//			jorjala.animation = false;
//			jorjala.thread_animation->join();
//          memoire::deloge("thread_animation", jorjala.thread_animation);
		}
		else if (donnees.metadonnee == "pas_en_avant") {
            jorjala.temps_courant(jorjala.temps_courant() + 1);

            if (jorjala.temps_courant() > jorjala.temps_fin()) {
                jorjala.temps_courant(jorjala.temps_fin());
            }

            jorjala.ajourne_pour_nouveau_temps("controle pas en avant");
		}
		else if (donnees.metadonnee == "joue_en_avant") {
			/* Évite d'envoyer des évènements inutiles. */
            if (jorjala.animation_en_cours() == true) {
				return EXECUTION_COMMANDE_REUSSIE;
			}

            // jorjala.thread_animation = memoire::loge<std::thread>("thread_animation", anime_image, jorjala);
            // jorjala.animation = true;
			return EXECUTION_COMMANDE_REUSSIE;
		}
		else if (donnees.metadonnee == "va_image_fin") {
            jorjala.temps_courant(jorjala.temps_fin());

            jorjala.ajourne_pour_nouveau_temps("controle va image fin");
		}

        jorjala.notifie_observatrices(JJL::TypeEvenement::TEMPS | JJL::TypeEvenement::MODIFIÉ);

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
