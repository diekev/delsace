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

#include <netdb.h>
#include <string>
#include <strings.h> /* bzero */
#include <unistd.h> /* close */

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "uri.hh"

#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename T>
concept bool ConceptTypeCliente = requires(
									  const typename T::donnees_requete &d,
									  std::string &requete,
									  const std::string &reponse)
{
	typename T::donnees_requete;
	{ T::construit_requete(d, requete) } -> void;
	{ T::construit_reponse(reponse) } -> void;
};
#else
#	define ConceptTypeCliente typename
#endif

namespace reseau {

void imprime_erreur_ssl(SSL *ssl, int erreur, const char *message, bool pile = false)
{
	printf("%s", message);

	erreur = SSL_get_error(ssl, erreur);

	switch (erreur) {
		case SSL_ERROR_NONE:
			printf("SSL_ERROR_NONE\n");
			break;
		case SSL_ERROR_ZERO_RETURN:
			printf("SSL_ERROR_ZERO_RETURN\n");
			break;
		case SSL_ERROR_WANT_READ:
			printf("SSL_ERROR_WANT_READ\n");
			break;
		case SSL_ERROR_WANT_WRITE:
			printf("SSL_ERROR_WANT_WRITE\n");
			break;
		case SSL_ERROR_WANT_CONNECT:
			printf("SSL_ERROR_WANT_CONNECT\n");
			break;
		case SSL_ERROR_WANT_ACCEPT:
			printf("SSL_ERROR_WANT_ACCEPT\n");
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			printf("SSL_ERROR_WANT_X509_LOOKUP\n");
			break;
		case SSL_ERROR_SYSCALL:
			printf("SSL_ERROR_SYSCALL\n");
			break;
		case SSL_ERROR_SSL:
			printf("SSL_ERROR_SSL\n");
			break;
	}

	if (pile) {
		ERR_print_errors_fp(stderr);
	}
}

template <
		ConceptTypeCliente TypeCliente
>
class cliente {
	SSL *m_ssl = nullptr;
	int m_prise = -1;
	bool m_securise = false;

public:
	~cliente()
	{
		if (m_prise != -1) {
			close(m_prise);
		}

		if (m_ssl != nullptr) {
			SSL_free(m_ssl);
		}
	}

	bool connecte_vers(const uri &u)
	{
		if (!u.est_valide()) {
			return false;
		}

		const auto hote = std::string(u.hote());
		hostent *he = gethostbyname(hote.c_str());

		if (he == nullptr) {  /* Info de l'hôte */
			herror("gethostbyname");
			return false;
		}

		m_prise = socket(AF_INET, SOCK_STREAM, 0);

		if (m_prise == -1) {
			perror("socket");
			return false;
		}

		/* Adresse de celui qui se connecte */
		struct sockaddr_in their_addr;
		their_addr.sin_family = AF_INET;      /* host byte order */

		uint16_t port = 0;

		if (u.port() == "") {
			if (u.schema() == "https") {
				m_securise = true;
				port = 443;
			}
			else if (u.schema() == "http") {
				m_securise = false;
				port = 80;
			}
		}
		else {
			port = static_cast<uint16_t>(std::atoi(std::string(u.port()).c_str()));
		}

		their_addr.sin_port = htons(port);    /* short, network byte order */
		their_addr.sin_addr = *(reinterpret_cast<in_addr *>(he->h_addr));
		bzero(&(their_addr.sin_zero), 8);     /* zero pour le reste de struct */

		if (connect(m_prise, reinterpret_cast<sockaddr *>(&their_addr), sizeof(sockaddr)) == -1) {
			perror("connect");
			return false;
		}

		if (m_securise) {
			SSL_library_init();
			OpenSSL_add_ssl_algorithms();
			SSL_load_error_strings();

			const SSL_METHOD *method = SSLv23_client_method();

			if (method == nullptr) {
				printf("Erreur lors de la création de la méthode SSL\n");
				return false;
			}

			auto ctx = SSL_CTX_new(method);

			if (ctx == nullptr) {
				imprime_erreur_ssl(nullptr, 0, "Erreur lors de la création du contexte SSL : ", true);
				return false;
			}

			m_ssl = SSL_new(ctx);

			if (m_ssl == nullptr) {
				imprime_erreur_ssl(m_ssl, 0, "Erreur lors de la création SSL : ", true);
				return false;
			}

			SSL_set_fd(m_ssl, m_prise);

			auto connexion = SSL_connect(m_ssl);

			if (connexion <= 0) {
				imprime_erreur_ssl(m_ssl, connexion, "Erreur lors de la connexion SSL : ", true);
				return false;
			}
		}

		return true;
	}

	void envoie_requete(const typename TypeCliente::donnees_requete &donnees)
	{
		if (m_prise == -1) {
			return;
		}

		std::string requete;
		TypeCliente::construit_requete(donnees, requete);

		if (m_securise) {
			auto taille = SSL_write(m_ssl, requete.c_str(), requete.size());

			if (taille <= 0) {
				imprime_erreur_ssl(m_ssl, taille, "Erreur lors de l'écriture SSL : ");
			}
		}
		else {
			if (send(m_prise, requete.c_str(), requete.size(), 0) == -1) {
				perror("send");
			}
		}
	}

	void recois_reponse()
	{
		static constexpr auto MAXDATASIZE = 1024;

		char tampon[MAXDATASIZE];

		std::string reponse;

		if (m_securise) {
			do {
				auto taille = SSL_read(m_ssl, tampon, MAXDATASIZE);

				if (taille > 0) {
					tampon[taille] = '\0';
					reponse.append(tampon, taille);
				}
				else if (taille <= 0) {
					auto erreur = SSL_get_error(m_ssl, taille);

					if (erreur != SSL_ERROR_NONE || erreur != SSL_ERROR_ZERO_RETURN) {
						printf("Erreur lors de la lecture SSL : ");

						switch (erreur) {
							case SSL_ERROR_NONE:
								printf("SSL_ERROR_NONE\n");
								break;
							case SSL_ERROR_ZERO_RETURN:
								printf("SSL_ERROR_ZERO_RETURN\n");
								break;
							case SSL_ERROR_WANT_READ:
								printf("SSL_ERROR_WANT_READ\n");
								break;
							case SSL_ERROR_WANT_WRITE:
								printf("SSL_ERROR_WANT_WRITE\n");
								break;
							case SSL_ERROR_WANT_CONNECT:
								printf("SSL_ERROR_WANT_CONNECT\n");
								break;
							case SSL_ERROR_WANT_ACCEPT:
								printf("SSL_ERROR_WANT_ACCEPT\n");
								break;
							case SSL_ERROR_WANT_X509_LOOKUP:
								printf("SSL_ERROR_WANT_X509_LOOKUP\n");
								break;
							case SSL_ERROR_SYSCALL:
								printf("SSL_ERROR_SYSCALL\n");
								break;
							case SSL_ERROR_SSL:
								printf("SSL_ERROR_SSL\n");
								break;
						}
					}

					break;
				}
			}
			while (true);
		}
		else {
			do {
				auto taille = recv(m_prise, tampon, MAXDATASIZE, 0);

				if (taille > 0) {
					tampon[taille] = '\0';
					reponse.append(tampon, taille);

					if (taille < MAXDATASIZE) {
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
		}

		return TypeCliente::construit_reponse(reponse);
	}
};

}  /* namespace reseau */
