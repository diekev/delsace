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

#include "repondant_commande.h"

#include <iostream>

#include "commande.h"

RepondantCommande::RepondantCommande(UsineCommande &usine_commande, std::any const &pointeur)
    : m_usine_commande(&usine_commande)
	, m_pointeur(pointeur)
{}

bool RepondantCommande::appele_commande(dls::chaine const &categorie, DonneesCommande const &donnees_commande)
{
#if 0
	std::cerr << "Appele commande pour catégorie : " << categorie << " :\n";
	std::cerr << "\tclé : " << donnees_commande.cle << '\n';
	std::cerr << "\tmodificateur : " << donnees_commande.modificateur << '\n';
	std::cerr << "\tsouris : " << donnees_commande.souris << '\n';
	std::cerr << "\tx : " << donnees_commande.x << '\n';
	std::cerr << "\ty : " << donnees_commande.y << '\n';
#endif
    if (m_commande_modale) {
        return false;
    }

    auto donnees = donnees_commande;
    auto commande = m_usine_commande->trouve_commande(categorie, donnees);

	if (commande == nullptr) {
		return false;
    }

	auto resultat = commande->execute(m_pointeur, donnees);

	if (resultat == EXECUTION_COMMANDE_MODALE) {
		m_commande_modale = commande;
	}
	else {
		m_commande_modale = nullptr;
		delete commande;
	}

	return true;
}

bool RepondantCommande::ajourne_commande_modale(DonneesCommande const &donnees_commande)
{
	if (!m_commande_modale) {
        return false;
	}

	m_commande_modale->ajourne_execution_modale(m_pointeur, donnees_commande);
    return true;
}

bool RepondantCommande::acheve_commande_modale(DonneesCommande const &donnees_commande)
{
	if (!m_commande_modale) {
        return false;
	}

	m_commande_modale->termine_execution_modale(m_pointeur, donnees_commande);

	delete m_commande_modale;
	m_commande_modale = nullptr;
    return true;
}

void RepondantCommande::repond_clique(dls::chaine const &identifiant, dls::chaine const &metadonnee)
{
    auto commande = (*m_usine_commande)(identifiant);

	DonneesCommande donnees;
	donnees.metadonnee = metadonnee;
    donnees.identifiant = identifiant;

	commande->execute(m_pointeur, donnees);

	delete commande;
}

bool RepondantCommande::evalue_predicat(dls::chaine const &identifiant, dls::chaine const &metadonnee)
{
    auto commande = (*m_usine_commande)(identifiant);
    if (!commande) {
        return false;
    }
	auto ok = commande->evalue_predicat(m_pointeur, metadonnee);
	delete commande;
	return ok;
}
