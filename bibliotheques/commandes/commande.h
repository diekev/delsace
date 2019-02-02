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

#include <any>
#include <stack>
#include <unordered_map>

enum {
	EXECUTION_COMMANDE_REUSSIE,
	EXECUTION_COMMANDE_ECHOUEE,
	EXECUTION_COMMANDE_MODALE,
};

struct DonneesCommande {
	int souris = 0;
	int modificateur = 0;
	int cle = 0;
	bool double_clique = false;
	float x = 0;
	float y = 0;
	std::string metadonnee = "";

	DonneesCommande() = default;
};

class Commande {
public:
	virtual ~Commande() = default;

	virtual bool evalue_predicat(std::any const &pointeur, std::string const &metadonnee);

	virtual int execute(std::any const &pointeur, DonneesCommande const &donnees) = 0;

	virtual void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees);

	virtual void termine_execution_modale(std::any const &pointeur, DonneesCommande const &donnees);
};

struct DescriptionCommande {
	typedef Commande *(*fonction_usine)();

	std::string categorie{};
	std::string metadonnee{};
	int souris = 0;
	int modificateur = 0;
	int cle = 0;
	bool double_clique = false;
	bool pad[3];

	fonction_usine construction_commande = nullptr;
};

template <typename T>
inline auto description_commande(
		std::string const &categorie,
		int souris,
		int modificateur,
		int cle,
		bool double_clique,
		std::string const &metadonnee = "")
{
	DescriptionCommande description;
	description.cle = cle;
	description.souris = souris;
	description.modificateur = modificateur;
	description.categorie = categorie;
	description.double_clique = double_clique;
	description.construction_commande = []() -> Commande* { return new T(); };
	description.metadonnee = metadonnee;

#if 0
	std::string identifiant;
	identifiant.reserve(categorie.size() + 13);

	identifiant += categorie;

	char tampon[4];

	*tampon = *reinterpret_cast<char *>(&description.souris);

	identifiant.push_back(tampon[0]);
	identifiant.push_back(tampon[1]);
	identifiant.push_back(tampon[2]);
	identifiant.push_back(tampon[3]);

	*tampon = *reinterpret_cast<char *>(&description.modificateur);

	identifiant.push_back(tampon[0]);
	identifiant.push_back(tampon[1]);
	identifiant.push_back(tampon[2]);
	identifiant.push_back(tampon[3]);

	*tampon = *reinterpret_cast<char *>(&description.cle);

	identifiant.push_back(tampon[0]);
	identifiant.push_back(tampon[1]);
	identifiant.push_back(tampon[2]);
	identifiant.push_back(tampon[3]);

	identifiant.push_back('\0' + static_cast<char>(double_clique));

	std::cerr << "Tampon commande " << identifiant << '\n';
#endif

	return description;
}

class UsineCommande {
	std::unordered_map<std::string, DescriptionCommande> m_tableau{};

public:
	void enregistre_type(std::string const &nom, DescriptionCommande const &description);

	Commande *operator()(std::string const &nom);

	Commande *trouve_commande(std::string const &categorie, DonneesCommande &donnees_commande);
};
