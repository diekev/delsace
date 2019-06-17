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

#include <iostream>

#include "analyseuse_langage.h"
#include "decoupeur.h"
#include "erreur.h"

const char *texte =
		R"(chronomètre "chrono"
		entrée = exprime "Entrez votre nom : "
		imprime entrée
		a = 5
		imprime a
		imprime "Temps exécution :", temps "chrono"
		fonction foo()
		{
			a = 5
			b = a + 2
			retourne b
		})";

int main()
{
	try {
		langage::Decoupeur decoupeur(texte);
		decoupeur.decoupe();

		langage::AnalyseuseLangage analyseuse;
		analyseuse.lance_analyse(decoupeur.morceaux());
	}
	catch (langage::ErreurFrappe &e) {
		std::cerr << e.quoi() << '\n';
	}
	catch (langage::ErreurSyntactique &e) {
		std::cerr << e.quoi() << '\n';
	}
}
