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

#include "commande.h"

#include <cassert>

#include "../outils/definitions.h"

bool Commande::evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee)
{
	INUTILISE(pointeur);
	INUTILISE(metadonnee);
	return true;
}

void Commande::ajourne_execution_modale(
		std::any const &pointeur,
		DonneesCommande const &donnees)
{
	INUTILISE(pointeur);
	INUTILISE(donnees);
}

void Commande::termine_execution_modale(
		std::any const &pointeur,
		DonneesCommande const &donnees)
{
	INUTILISE(pointeur);
	INUTILISE(donnees);
}

void UsineCommande::enregistre_type(dls::chaine const &nom, DescriptionCommande const &description)
{
	auto const iter = m_tableau.trouve(nom);
	assert(iter == m_tableau.fin());
    description.identifiant = nom;
	m_tableau[nom] = description;
}

Commande *UsineCommande::operator()(dls::chaine const &nom)
{
	auto const iter = m_tableau.trouve(nom);
    if (iter == m_tableau.fin()) {
        return nullptr;
    }

	DescriptionCommande const &desc = iter->second;

	return desc.construction_commande();
}

Commande *UsineCommande::trouve_commande(dls::chaine const &categorie, DonneesCommande &donnees_commande)
{
	for (auto const &donnees : m_tableau) {
		DescriptionCommande const &desc = donnees.second;

		if (desc.categorie != categorie) {
			continue;
		}

		if (desc.souris != donnees_commande.souris) {
			continue;
		}

		if (desc.double_clique != donnees_commande.double_clique) {
			continue;
		}

		if (desc.modificateur != donnees_commande.modificateur) {
			continue;
		}

		if (desc.cle != donnees_commande.cle) {
			continue;
		}

		if (donnees_commande.metadonnee == "") {
			donnees_commande.metadonnee = desc.metadonnee;
		}

        if (donnees_commande.identifiant == "") {
            donnees_commande.identifiant = desc.identifiant;
        }

		return desc.construction_commande();
	}

	return nullptr;
}

long UsineCommande::taille() const
{
	return m_tableau.taille();
}
