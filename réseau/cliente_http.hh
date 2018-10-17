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

#include "outils_http.hh"
#include "uri.hh"

#include <iostream>
#include <sstream>

namespace reseau {

struct cliente_http {
	struct donnees_requete {
		methode_http methode;
		reseau::uri uri;
	};

	static void construit_requete(const donnees_requete &donnees, std::string &chaine)
	{
		chaine.clear();

		std::stringstream ss;

		switch (donnees.methode) {
			case methode_http::GET:
				ss << "GET ";
				break;
			case methode_http::HEAD:
				ss << "HEAD ";
				break;
			case methode_http::POST:
				ss << "POST ";
				break;
			case methode_http::PUT:
				ss << "PUT ";
				break;
			case methode_http::DELETE:
				ss << "DELETE ";
				break;
			case methode_http::CONNECT:
				ss << "CONNECT ";
				break;
			case methode_http::OPTIONS:
				ss << "OPTIONS ";
				break;
			case methode_http::TRACE:
				ss << "TRACE ";
				break;
			case methode_http::PATCH:
				ss << "PATCH ";
				break;
		}

		if (!donnees.uri.chemin().empty()) {
			ss << donnees.uri.chemin();
			ss << donnees.uri.requete();
			ss << donnees.uri.fragment();
		}
		else {
			ss << "/";
		}

		ss << " HTTP/1.1" << NOUVELLE_LIGNE;

		ss << "Host: " << donnees.uri.hote() << NOUVELLE_LIGNE;
		ss << "Accept-Encoding: identity, gzip, deflate, br" << NOUVELLE_LIGNE;
		ss << "Connection: close" << NOUVELLE_LIGNE;
		ss << "User-Agent: delsace" << NOUVELLE_LIGNE;
		ss << "Cache-Control: max-age=0" << NOUVELLE_LIGNE;
		ss << "Keep-Alive: 300" << NOUVELLE_LIGNE;
		ss << "Upgrade-Insecure-Requests: 1" << NOUVELLE_LIGNE;
		ss << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/ *;q=0.8" << NOUVELLE_LIGNE;
		ss << "Accept-Charset: utf-8, iso-8859-1;q=0.5" << NOUVELLE_LIGNE;
		ss << "Accept-Language: fr-FR,fr;q=0.9,en-US;q=0.8,en;q=0.7,ja;q=0.6" << NOUVELLE_LIGNE;
		ss << NOUVELLE_LIGNE;

		chaine = ss.str();

		std::cerr << "Requête : \n";
		std::cerr << chaine;
	}

	static void construit_reponse(const std::string &chaine)
	{
		std::cerr << "Réponse : \n";
		std::cerr << chaine;

		// HTTP/1.1 302 Found

		if (chaine[0] != 'H') {
			return;
		}
		if (chaine[1] != 'T') {
			return;
		}
		if (chaine[2] != 'T') {
			return;
		}
		if (chaine[3] != 'P') {
			return;
		}

		//auto protocol = 10 + (chaine[7] == '1');

		//auto resultat = std::string_view(&chaine[9], 3);

		//auto raison = std::string_view(&chaine[10], 5);
	}
};

}  /* namespace reseau */
