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

#include <cstring>
#include <iostream>

#include "decoupage/decoupeuse.h"
#include "decoupage/erreur.h"

int main()
{
//	const char *str = ".3 boucle 0...20 { imprime(index != modèle de voiture); }";

	const char *str = ""
	"# Ceci est un commentaire \n"
	"soit str='a';\n"
	"associe nombre {\n"
	" 0...10: imprime(10);\n"
	" 11...20: imprime(20);\n"
	" sinon:imprime(inconnu);\n"
	"decoupeuse_texte decoupeuse(str, str + len);\n"
	"}";

	const size_t len = std::strlen(str);

	try {
		decoupeuse_texte decoupeuse(str, str + len);
		decoupeuse.genere_morceaux();

		for (auto &morceaux : decoupeuse) {
			std::cout << "{ \"" << morceaux.chaine << "\", " << chaine_identifiant(morceaux.identifiant) << " },\n";
		}

		//assembleuse_arbre assembleuse;
		//analyseuse_grammaire analyseuse(assembleuse);
		//analyseuse.lance_analyse(decoupeuse_texte.begin(), decoupeuse_texte.end());
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message();
	}
}
