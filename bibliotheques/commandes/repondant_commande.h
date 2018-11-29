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

#pragma once

#include <danjo/repondant_bouton.h>

#include <any>
#include <string>

class Commande;
class DonneesCommande;
class UsineCommande;

class RepondantCommande : public danjo::RepondantBouton {
	UsineCommande *m_usine_commande;
	std::any m_pointeur = nullptr;

	Commande *m_commande_modale = nullptr;

public:
	RepondantCommande(UsineCommande *usine_commande, std::any const &pointeur);

	bool appele_commande(const std::string &categorie, const DonneesCommande &donnees_commande);

	void ajourne_commande_modale(const DonneesCommande &donnees_commande);

	void acheve_commande_modale(const DonneesCommande &donnees_commande);

	bool evalue_predicat(const std::string &identifiant, const std::string &metadonnee) override;

	void repond_clique(const std::string &identifiant, const std::string &metadonnee) override;
};
