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

#include "cliente.hh"
#include "cliente_http.hh"

int main()
{
	const auto argc = 2;
	const char *argv[] = { "client", "https://kanbun.info/shibu02/roushi00.html" };

	if (argc != 2) {
		std::cerr << "usage : client hôte\n";
		return 1;
	}

	auto uri = 	reseau::uri(argv[1]);
	auto cliente = reseau::cliente<reseau::cliente_http>{};

	if (cliente.connecte_vers(uri) == false) {
		return 1;
	}

	auto donnees = reseau::cliente_http::donnees_requete{ reseau::methode_http::GET, uri };

	cliente.envoie_requete(donnees);
	cliente.recois_reponse();

	return 0;
}
