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

namespace reseau {

static constexpr auto NOUVELLE_LIGNE = "\r\n";

enum methode_http : char {
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE,
	PATCH,
};

enum status_http {
	/* 1xx */
	_continue = 100,
	changeant_protocol = 101,

	/* 2xx */
	ok = 200,
	cree = 201,
	accepte = 202,
	information_non_autoritative = 203,
	aucun_contenu = 204,
	contenu_reset = 205,
	contenu_partiel = 206,

	/* 3xx */
	multiples_choix = 300,
	bouge = 301,
	trouve = 302,
	voir_autre = 303,
	non_modifie = 304,
	utilise_proxy = 305,
	redirection_temporaire = 307,

	/* 4xx */
	mauvaise_requete = 400,
	non_autorise = 401,
	paiement_requis = 402,
	interdit = 403,
	pas_trouve = 404,
	methode_non_autorisee = 405,
	innacceptable = 406,
	authentification_proxy_requise = 407,
	temps_requete_ecoule = 408,
	conflit = 409,
	disparu = 410,
	longueur_requise = 411,
	echec_precondition = 412,
	entite_requete_trop_large = 413,
	uri_requete_trop_long = 414,
	type_media_non_supporte = 415,
	requete_insatisfaisable = 416,
	echec_esperance = 417,

	/* 5xx */
	erreur_interne = 500,
	non_implemente = 501,
	service_indisponible = 503,
	temps_ecoule = 504,
	version_http_non_supportee = 505,
};

}  /* namespace reseau */
