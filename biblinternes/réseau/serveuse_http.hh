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

#include "message.hh"

#include <iostream>
#include <sstream>

namespace reseau {

template <typename SiteInternet>
struct serveuse_http {
	static constexpr uint16_t PORT_DEFAUT = 5007;

	static type_requete construit_requete(const dls::chaine &requete)
	{
		dls::dico<dls::vue_chaine, dls::vue_chaine> champs;

		/* decoupe la requete */
		size_t decalage = 0;
		size_t pos = 0;

		while (requete[pos] != ' ') {
			++pos;
		}

		auto chaine_methode = dls::vue_chaine(&requete[decalage], pos);

		decalage = pos + 1;
		pos = decalage;

		while (requete[pos] != ' ') {
			++pos;
		}

		auto cible = dls::vue_chaine(&requete[decalage], pos - decalage);

		decalage = pos + 1;
		pos = decalage;

		while (requete[pos] != '\n') {
			++pos;
		}

		auto version = dls::vue_chaine(&requete[decalage], pos - decalage + 1);

		dls::chaine cible_corrige;
		cible_corrige.reserve(cible.taille());

		auto converti_hex = [](const char c)
		{
			if (c >= '0' && c <= '9') {
				return c - '0';
			}

			if (c >= 'a' && c <= 'f') {
				return (c - 'a') + 10;
			}

			if (c >= 'A' && c <= 'F') {
				return (c - 'A') + 10;
			}

			return 0;
		};

		for (size_t i = 0; i < cible.taille(); ++i) {
			if (cible[i] == '%') {
				auto f1 = converti_hex(cible[i + 1]);
				auto f2 = converti_hex(cible[i + 2]);
				cible_corrige.pousse(static_cast<char>((f1 << 4) | f2));
				i += 2;
			}
			else {
				cible_corrige.pousse(cible[i]);
			}
		}

		std::cerr << "Méthode : " << chaine_methode << '\n';
		std::cerr << "Cible   : " << cible_corrige << '\n';
		std::cerr << "Version : " << version << '\n';
		std::cout << requete;

		auto methode = -1;

		if (chaine_methode == "GET") {
			methode = methode_http::GET;
		}
		else if (chaine_methode == "POST") {
			methode = methode_http::POST;
		}
		else if (chaine_methode == "HEAD") {
			methode = methode_http::HEAD;
		}
		else if (chaine_methode == "PUT") {
			methode = methode_http::PUT;
		}
		else if (chaine_methode == "DELETE") {
			methode = methode_http::DELETE;
		}
		else if (chaine_methode == "CONNECT") {
			methode = methode_http::CONNECT;
		}
		else if (chaine_methode == "OPTIONS") {
			methode = methode_http::OPTIONS;
		}
		else if (chaine_methode == "TRACE") {
			methode = methode_http::TRACE;
		}
		else if (chaine_methode == "PATCH") {
			methode = methode_http::PATCH;
		}

		return type_requete{methode, uri(dls::chaine("http:").append(cible_corrige))};
	}

	static void construit_reponse(const type_reponse &reponse, dls::chaine &chaine)
	{
		std::stringstream ss;

		ss << "HTTP/1.1 ";

		switch (reponse.status) {
			case status_http::_continue:
				ss << "100 Continue";
				break;
			case status_http::changeant_protocol:
				ss << "101 Switching Protocols";
				break;
			case status_http::ok:
				ss << "200 OK";
				break;
			case status_http::cree:
				ss << "201 Created";
				break;
			case status_http::accepte:
				ss << "202 Accepted";
				break;
			case status_http::information_non_autoritative:
				ss << "203 Non-Authoritative Information";
				break;
			case status_http::aucun_contenu:
				ss << "204 No Content";
				break;
			case status_http::contenu_reset:
				ss << "205 Reset Content";
				break;
			case status_http::contenu_partiel:
				ss << "206 Partial Content";
				break;
			case status_http::multiples_choix:
				ss << "300 Multiple Choices";
				break;
			case status_http::bouge:
				ss << "301 Moved Permanently";
				break;
			case status_http::trouve:
				ss << "302 Found";
				break;
			case status_http::voir_autre:
				ss << "303 See Other";
				break;
			case status_http::non_modifie:
				ss << "304 Not Modified";
				break;
			case status_http::utilise_proxy:
				ss << "305 Use Proxy";
				break;
			case status_http::redirection_temporaire:
				ss << "307 Temporary Redirect";
				break;
			case status_http::mauvaise_requete:
				ss << "400 Bad Request";
				break;
			case status_http::non_autorise:
				ss << "401 Unauthorized";
				break;
			case status_http::paiement_requis:
				ss << "402 Payment Required";
				break;
			case status_http::interdit:
				ss << "403 Forbidden";
				break;
			case status_http::pas_trouve:
				ss << "404 Not Found";
				break;
			case status_http::methode_non_autorisee:
				ss << "405 Method Not Allowed";
				break;
			case status_http::innacceptable:
				ss << "406 Not Acceptable";
				break;
			case status_http::authentification_proxy_requise:
				ss << "407 Proxy Authentication Required";
				break;
			case status_http::temps_requete_ecoule:
				ss << "408 Request Timeout";
				break;
			case status_http::conflit:
				ss << "409 Conflict";
				break;
			case status_http::disparu:
				ss << "410 Gone";
				break;
			case status_http::longueur_requise:
				ss << "411 Length Required";
				break;
			case status_http::echec_precondition:
				ss << "412 Precondition Failed";
				break;
			case status_http::entite_requete_trop_large:
				ss << "413 Request Entity Too Large";
				break;
			case status_http::uri_requete_trop_long:
				ss << "414 Request-URI Too Long";
				break;
			case status_http::type_media_non_supporte:
				ss << "415 Unsupported Media Type";
				break;
			case status_http::requete_insatisfaisable:
				ss << "416 Requested Range Not Satisfiable";
				break;
			case status_http::echec_esperance:
				ss << "417 Expectation Failed";
				break;
			case status_http::erreur_interne:
				ss << "500 Internal Server Error";
				break;
			case status_http::non_implemente:
				ss << "501 Not Implemented";
				break;
			case status_http::service_indisponible:
				ss << "503 Service Unavailable";
				break;
			case status_http::temps_ecoule:
				ss << "504 Gateway Timeout";
				break;
			case status_http::version_http_non_supportee:
				ss << "505 HTTP Version Not Supported";
				break;
		}

		ss << NOUVELLE_LIGNE;

		ss << "Date: " << "Tue, 09 Oct 2018 02:19:14 GMT" << NOUVELLE_LIGNE;
		ss << "Server: " << "agaric" << NOUVELLE_LIGNE;
		ss << "Content-Length: " << reponse.corps.taille() << NOUVELLE_LIGNE;
		ss << "Connection: " << "close" << NOUVELLE_LIGNE;

		for (const auto &entete : reponse.entetes) {
			ss << entete.first << ": " << entete.second << NOUVELLE_LIGNE;
		}

		ss << NOUVELLE_LIGNE;
		ss << reponse.corps;

		chaine = ss.str();

		std::cerr << "Réponse :\n";
		std::cerr << chaine;
	}

	static void repond_requete(const type_requete &requete, type_reponse &reponse)
	{
		SiteInternet::repond_requete(requete, reponse);
	}
};

}  /* namespace reseau */
