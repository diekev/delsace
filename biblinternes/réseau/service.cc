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

#include "serveuse.hh"
#include "serveuse_http.hh"

static bool utilisateur_connecte = false;

static void calcul_gabarit(const std::string &/*gabarit*/, reseau::type_reponse &reponse)
{
	reponse.status = reseau::status_http::ok;
	reponse.corps = "<h1>Salut, tout le monde !</h1>";
	reponse.entetes.insert({"Content-Type", "text/html; charset=utf-8"});
}

static void redirige(const std::string &cible, reseau::type_reponse &reponse)
{
	reponse.status = reseau::status_http::bouge;
	reponse.entetes.insert({"Location", "http://localhost:5007" + cible});
}

static void affiche_page_accueil(reseau::type_reponse &reponse)
{
	calcul_gabarit("accueil.html", reponse);
}

static void affiche_page_compte(reseau::type_reponse &reponse)
{
	if (!utilisateur_connecte) {
		return redirige("/connexion/", reponse);
	}

	calcul_gabarit("compte_utilisateur.html", reponse);
}

static void affiche_page_connexion(reseau::type_reponse &reponse)
{
	if (utilisateur_connecte) {
		return redirige("/compte/", reponse);
	}

	calcul_gabarit("connexion.html", reponse);
}

static void affiche_page_deconnexion(reseau::type_reponse &reponse)
{
	redirige("/", reponse);
}

struct site_internet {
	static void repond_requete(
			const reseau::type_requete &requete,
			reseau::type_reponse &reponse)
	{
		auto chemin = requete.uri.chemin();
		auto methode = requete.methode;

		if (methode == reseau::methode_http::GET) {
			if (chemin == "/") {
				affiche_page_accueil(reponse);
			}
			else if (chemin == "/compte/") {
				affiche_page_compte(reponse);
			}
			else if (chemin == "/connexion/") {
				affiche_page_connexion(reponse);
			}
			else if (chemin == "/déconnexion/") {
				affiche_page_deconnexion(reponse);
			}
			else {
				reponse.status = reseau::status_http::pas_trouve;
			}
		}
		else if (methode == reseau::methode_http::POST) {
			reponse.status = reseau::status_http::pas_trouve;
		}
		else {
			reponse.status = reseau::status_http::methode_non_autorisee;
		}
	}
};

int main()
{
	reseau::serveuse<10, reseau::serveuse_http<site_internet>> serveuse{};
	serveuse.demarre();

	return 0;
}
