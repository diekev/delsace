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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "plage.hh"

#include <iostream>

static auto imprime_plage(dls::plage_continue<const int> const &plage)
{
	auto plg = plage;
	while(!plg.est_finie()) {
		std::cerr << plg.front() << '\n';
		plg.effronte();
	}
}

static auto trouve_binaire(dls::plage_continue<const int> const &plage, int v)
{
	std::cerr << "------------------------------------------------\n";
	std::cerr << __func__ << '\n';
	std::cerr << "Recherche : " << v << '\n';
	auto plg = plage;
	std::cerr << "Plage :\n";
	imprime_plage(plage);

	while (!plg.est_finie()) {
		/* divise la plage en deux */
		auto m = plg.deuxieme_moitie();
		std::cerr << "Deuxième moitié :\n";
		imprime_plage(m);

		if (m.front() == v) {
			plg = m;
			break;
		}
		else {
			if (m.front() < v) {
				m.effronte();
				plg = m;
			}
			else {
				std::cerr << "Première moitié :\n";
				plg = plg.premiere_moitie();
				imprime_plage(plg);
			}
		}
	}

	return plg;
}

int main()
{
	const int valeurs[6] = { 0, 1, 2, 3, 4, 5 };

	auto plage = dls::plage_continue(&valeurs[0], &valeurs[6]);

	std::cerr << "------------------------------------------------\n";
	std::cerr << "plage complète\n";
	imprime_plage(plage);

	std::cerr << "------------------------------------------------\n";
	std::cerr << "première moitié\n";
	imprime_plage(plage.premiere_moitie());

	std::cerr << "------------------------------------------------\n";
	std::cerr << "deuxième moitié\n";
	imprime_plage(plage.deuxieme_moitie());

	std::cerr << "------------------------------------------------\n";
	std::cerr << "recherche binaire\n";

	for (int i = 0; i < 10; ++i) {
		auto plg = trouve_binaire(plage, i);

		if (plg.est_finie()) {
			std::cerr << "Impossible de trouver '" << i << "'\n";
		}
		else {
			std::cerr << "Trouver '" << plg.front() << "' pour '" << i << "'\n";
		}
	}

	return 0;
}
