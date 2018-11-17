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

#include <string>
#include <unordered_map>

#include "morceaux.h"
#include "tampon_source.h"

struct ContexteGenerationCode;

struct DonneesArgument {
	size_t index = 0;
	size_t donnees_type{-1ul};
	bool est_variadic = false;
	bool est_variable = false;
	char pad[6];
};

struct DonneesFonction {
	std::unordered_map<std::string_view, DonneesArgument> args{};
	size_t donnees_type{-1ul};
	std::vector<std::string_view> nom_args{};
	bool est_externe = false;
	bool est_variadique = false;
	char pad[6];
};

struct DonneesModule {
	TamponSource tampon{""};
	std::vector<DonneesMorceaux> morceaux{};
	std::vector<std::string_view> modules_importes{};
	std::vector<std::string_view> fonctions_exportees{};
	std::unordered_map<std::string_view, DonneesFonction> fonctions{};
	size_t id = 0ul;
	std::string nom{""};
	double temps_chargement = 0.0;
	double temps_analyse = 0.0;
	double temps_tampon = 0.0;
	double temps_decoupage = 0.0;

	DonneesModule() = default;

	/**
	 * Retourne vrai si le module importe un module du nom spécifié.
	 */
	bool importe_module(const std::string_view &nom_module) const;

	/**
	 * Retourne vrai si le module possède une fonction du nom spécifié.
	 */
	bool possede_fonction(const std::string_view &nom_fonction) const;

	/**
	 * Ajoute les données de la fonction dont le nom est spécifié en paramètres
	 * à la table de fonctions de ce contexte.
	 */
	void ajoute_donnees_fonctions(const std::string_view &nom_fonction, const DonneesFonction &donnees);

	/**
	 * Retourne les données de la fonction dont le nom est spécifié en
	 * paramètre. Si aucune fonction ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	[[nodiscard]] const DonneesFonction &donnees_fonction(const std::string_view &nom_fonction) const  noexcept;

	/**
	 * Retourne vrai si le nom spécifié en paramètre est celui d'une fonction
	 * ayant déjà été ajouté à la liste de fonctions de ce module.
	 */
	[[nodiscard]] bool fonction_existe(const std::string_view &nom_fonction) const noexcept;

	/**
	 * Retourne la mémoire utilisée en octet par les données de ce module. La
	 * quantité de mémoire utilisée n'est pas mise en cache est est recalculée
	 * à chaque appel à cette méthode.
	 */
	[[nodiscard]] size_t memoire_utilisee() const noexcept;

private:
	/* Utilisées comme retour dans donnees_fonction(nom). */
	DonneesFonction m_donnees_invalides{};
};

/**
 * Charge le module dont le nom est spécifié.
 *
 * Le nom doit être celui d'un fichier s'appelant '<nom>.kuri' et se trouvant
 * dans le dossier du module racine.
 *
 * Les fonctions contenues dans le module auront leurs noms préfixés par le nom
 * du module, sauf pour le module racine.
 *
 * Le std::ostream est un flux de sortie où sera imprimé le nom du module ouvert
 * pour tenir compte de la progression de la compilation. Si un nom de module ne
 * pointe pas vers un fichier Kuri, ou si le fichier ne peut être ouvert, une
 * exception est lancée.
 *
 * Les DonneesMorceaux doivent être celles du nom du module et sont utilisées
 * pour les erreurs lancées.
 *
 * Le paramètre est_racine ne doit être vrai que pour le module racine.
 */
void charge_module(
		std::ostream &os,
		const std::string &nom,
		ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau,
		bool est_racine = false);
