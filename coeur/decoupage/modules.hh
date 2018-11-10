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

#include "morceaux.h"
#include "tampon_source.h"

struct ContexteGenerationCode;

struct DonneesModule {
	TamponSource tampon{""};
	std::vector<DonneesMorceaux> morceaux{};
	std::vector<std::string_view> modules_importes{};
	std::vector<std::string_view> fonctions_exportees{};
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
	bool importe_module(const std::string_view &nom) const;

	/**
	 * Retourne vrai si le module possède une fonction du nom spécifié.
	 */
	bool possede_fonction(const std::string_view &nom) const;
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
