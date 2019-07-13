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

#include "biblinternes/langage/analyseuse.hh"
#include "biblinternes/structures/tableau.hh"

#include "biblinternes/structures/chaine.hh"

namespace arachne {

struct DonneesMorceaux {
	using type = int;
	static constexpr type INCONNU = -1;

	int identifiant;
	unsigned int valeur;
};

/**
 * Classe de base pour définir des analyseurs syntactique.
 */
class analyseuse : public lng::analyseuse<DonneesMorceaux> {
public:
	explicit analyseuse(dls::tableau<DonneesMorceaux> &identifiants)
		: lng::analyseuse<DonneesMorceaux>(identifiants)
	{}

	~analyseuse() override = default;

	/**
	 * Lance l'analyse syntactique du vecteur d'identifiants spécifié.
	 *
	 * Si aucun assembleur n'est installé lors de l'appel de cette méthode,
	 * une exception est lancée.
	 */
	void lance_analyse(std::ostream &os) override;
};

} /* namespace arachne */
