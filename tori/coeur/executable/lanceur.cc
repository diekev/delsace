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

#include "tori/gabarit.hh"
#include "tori/objet.hh"

int main()
{
	auto texte = "{%pour x dans evenements%}<a href='{{lien}}'>{%si variable%}{{variable}} {{x}}{% sinon %}texte{% finsi %}</a>\n{% finpour %}";
	// tests : avec et sans espace avant }}, %} et après {{, {%

	std::cerr << "texte : \n" << texte << '\n';

	auto objet = tori::ObjetDictionnaire{};

	auto tableau = tori::ObjetTableau::construit(127l, 36.0, "chaine test");
	objet.insere("evenements", tableau);

	auto chaine1 = tori::construit_objet("http://aaa");
	objet.insere("lien", chaine1);

	chaine1 = tori::construit_objet("eh ouais...");
	objet.insere("variable", chaine1);

	auto tampon = tori::calcul_gabarit(texte, objet);

	if (tampon.empty()) {

	}

	std::cerr << "-----------------------------------------------\n";
	std::cerr << "Résultat :\n";
	std::cerr << tampon;
	std::cerr << '\n';
	std::cerr << "-----------------------------------------------\n";

	return 0;
}
