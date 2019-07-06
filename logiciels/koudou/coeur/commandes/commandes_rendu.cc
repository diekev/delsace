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

#include "commandes_rendu.h"

#include "biblinternes/commandes/commande.h"

#include "../koudou.h"
#include "../moteur_rendu.h"

/* ************************************************************************** */

class CommandeDemarreRendu : public Commande {
public:
	CommandeDemarreRendu() = default;

	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto koudou = std::any_cast<Koudou *>(pointeur);
		auto tache_rendu = new(tbb::task::allocate_root()) TacheRendu(*koudou);
		tbb::task::enqueue(*tache_rendu);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeArreteRendu : public Commande {
public:
	CommandeArreteRendu() = default;

	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto koudou = std::any_cast<Koudou *>(pointeur);
		koudou->moteur_rendu->arrete();

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_rendu(UsineCommande &usine)
{
	usine.enregistre_type("demarre_rendu",
						   description_commande<CommandeDemarreRendu>(
							   "rendu", 0, 0, 0, false));

	usine.enregistre_type("arrete_rendu",
						   description_commande<CommandeArreteRendu>(
							   "rendu", 0, 0, 0, false));
}
