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

#include <arpa/inet.h>
#include <strings.h> /* bzero */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> /* close */
#include <sys/wait.h>

#include <iostream>

#include "message.hh"
#include "uri.hh"

#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename T>
concept bool ConceptTypeServeuse = requires(const std::string &requete)
{
	{ T::PORT_DEFAUT } -> unsigned short;
	{ T::construit_requete(requete) };
};
#else
#	define ConceptTypeServeuse typename
#endif

namespace reseau {

template <
		int MAX_CONNEXION,
		ConceptTypeServeuse TypeServeuse
>
struct serveuse {
	int m_prise{-1};

public:
	serveuse()
	{
		/* contrôle d'erreur */
		m_prise = socket(AF_INET, SOCK_STREAM, 0);

		if (m_prise == -1) {
			std::cerr << "Erreur !\n";
			return;
		}

		sockaddr_in mon_addre;
		mon_addre.sin_family = AF_INET;
		mon_addre.sin_port = htons(TypeServeuse::PORT_DEFAUT);
		/* trouve notre propre addresse */
		mon_addre.sin_addr.s_addr = 0;
		bzero(&(mon_addre.sin_zero), 8);

		auto err = bind(m_prise, reinterpret_cast<sockaddr *>(&mon_addre), sizeof(sockaddr));

		if (err == -1) {
			std::cerr << "Erreur de la liaison !\n";
			return;
		}

		err = listen(m_prise, MAX_CONNEXION);

		if (err == -1) {
			std::cerr << "Erreur lors de l'écoute !\n";
			return;
		}
	}

	~serveuse()
	{
		if (m_prise != -1) {
			close(m_prise);
		}
	}

	void demarre()
	{
		if (m_prise == -1) {
			return;
		}

		sockaddr_in their_addr; /* Adresse du connecté  */
		unsigned sin_size = sizeof(sockaddr_in);

		while (true) {
			auto prise = accept(m_prise, reinterpret_cast<sockaddr *>(&their_addr), &sin_size);

			if (prise == -1) {
				perror("accept");
				continue;
			}

			if (fork() == 0) {
				auto requete = charge_donnees_requete(prise, their_addr);

				auto reponse = type_reponse{};

				TypeServeuse::repond_requete(requete, reponse);

				auto chaine_reponse = std::string{""};

				TypeServeuse::construit_reponse(reponse, chaine_reponse);

				if (send(prise, chaine_reponse.c_str(), chaine_reponse.size(), 0) == -1) {
					perror("send");
				}

				close(prise);
				exit(0);
			}

			/* le parent n'a pas besoin de cela */
			close(prise);

			/* nettoyage des processus fils */
			while (waitpid(-1, nullptr, WNOHANG) > 0) {}
		}
	}

	type_requete charge_donnees_requete(int prise, const sockaddr_in &their_addr)
	{
		printf("serveur: Reçu connection de %s\n", inet_ntoa(their_addr.sin_addr));
		static constexpr auto MAXDATASIZE = 1024ul;

		char tampon[MAXDATASIZE];

		std::string requete;

		do {
			auto taille = recv(prise, tampon, MAXDATASIZE, 0);

			if (taille > 0) {
				tampon[taille] = '\0';
				requete.append(tampon, static_cast<size_t>(taille));

				if (taille < static_cast<long>(MAXDATASIZE)) {
					break;
				}
			}
			else if (taille == 0) {
				break;
			}
			else if (taille == -1) {
				/* À FAIRE : erreur */
				break;
			}
		}
		while (true);

		return TypeServeuse::construit_requete(requete);
	}

	void proces_requete()
	{

	}
};

}  /* namespace reseau */
