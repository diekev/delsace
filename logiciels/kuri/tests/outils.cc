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

#include "outils.h"

#include <iostream>

#include "compilation/analyseuse_grammaire.h"
#include "compilation/assembleuse_arbre.h"
#include "compilation/contexte_generation_code.h"
#include "compilation/decoupeuse.h"
#include "compilation/modules.hh"

std::pair<bool, bool> retourne_erreur_lancee(
		const char *texte,
		const bool imprime_message,
		const erreur::type_erreur type,
		const bool genere_code)
{
	auto contexte = ContexteGenerationCode{};
	/* Ne nomme pas le module, car c'est le module racine. */
	auto module = contexte.cree_module("", "");
	module->tampon = lng::tampon_source(texte);

	auto erreur_lancee = false;
	auto type_correcte = false;

	try {
		decoupeuse_texte decoupeuse(module);
		decoupeuse.genere_morceaux();

		auto assembleuse = assembleuse_arbre(contexte);
		contexte.assembleuse = &assembleuse;
		auto analyseuse = analyseuse_grammaire(contexte, module, "");

		std::ostream os(nullptr);
		analyseuse.lance_analyse(os);

		if (genere_code) {
			assembleuse.genere_code_C(contexte, os, "");
		}
	}
	catch (const erreur::frappe &e) {
		if (imprime_message) {
			std::cerr << e.message() << static_cast<int>( e.type()) << ' ' << static_cast<int>(type) << '\n';
		}

		erreur_lancee = true;
		type_correcte = type == e.type();
	}

	return { erreur_lancee, type_correcte };
}
