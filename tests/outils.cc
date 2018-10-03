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
#include <llvm/IR/Module.h>

#include "analyseuse_grammaire.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"
#include "erreur.h"

bool retourne_erreur_lancee(
		const char *texte,
		const bool imprime_message,
		const bool genere_code)
{
	auto tampon = TamponSource(texte);

	try {
		decoupeuse_texte decoupeuse(tampon);
		decoupeuse.genere_morceaux();

		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(tampon, &assembleuse);
		analyseuse.lance_analyse(decoupeuse.morceaux());

		if (genere_code) {
			auto contexte = ContexteGenerationCode();
			auto module = llvm::Module("test", contexte.contexte);
			contexte.module = &module;

			assembleuse.genere_code_llvm(contexte);
		}
	}
	catch (const erreur::frappe &e) {
		if (imprime_message) {
			std::cerr << e.message() << '\n';
		}

		return true;
	}
	catch (const char *e) {
		if (imprime_message) {
			std::cerr << e << '\n';
		}

		return true;
	}
	catch (...) {
		return true;
	}

	return false;
}
