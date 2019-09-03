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

#pragma once

#include <any>
#include <functional>
#include <memory>

#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

enum class type_noeud : int;

struct Noeud;
struct PriseEntree;
struct PriseSortie;

/* ************************************************************************** */

/**
 * Struct pour contenir les données d'une connexion en train d'être créée par
 * l'utilisateur afin de la dessiner à l'écran.
 */
struct Connexion {
	PriseEntree *prise_entree;
	PriseSortie *prise_sortie;

	/* coordonnées xy de la souris sur l'écran, c'est à cette position que
	 * l'autre point de la connexion est dessiné. */
	float x;
	float y;
};

/* ************************************************************************** */

/**
 * Struct pour contenir les informations d'un noeud pour les afficher à
 * l'utilisateur.
 */
struct InfoNoeud {
	dls::chaine informations{};

	/* coordonnées xy de la souris sur l'écran, c'est à cette position que
	 * les informations seront dessinées. */
	float x{};
	float y{};
};

/* ************************************************************************** */

using type_function_creation_noeud = std::function<Noeud *()>;
using type_function_destruction_noeud = std::function<void(Noeud *)>;

enum class type_graphe : int {
	OBJET,
	COMPOSITE,
	DETAIL,
	RACINE_OBJET,
	RACINE_COMPOSITE,
	INVALIDE,
};

struct Graphe {
	/* données pour l'entreface utilisateur */

	/* connexion créée par l'utilisateur en cliquant sur une prise d'entrée ou
	 * de sortie */
	Connexion *connexion_active = nullptr;

	/* information du noeud actif */
	InfoNoeud *info_noeud = nullptr;

	/* centre du graphe sur l'axe des X */
	float centre_x = 0.0f;

	/* centre du graphe sur l'axe des Y */
	float centre_y = 0.0f;

	/* zoom du graphe */
	float zoom = 1.0f;

	type_graphe type = type_graphe::INVALIDE;

	/* le noeud qui possède ce graphe */
	Noeud &noeud_parent;

	/* dernier noeud sélectionné */
	Noeud *noeud_actif = nullptr;

	/* dernier noeud sortie sélectionné */
	Noeud *dernier_noeud_sortie = nullptr;

	/* vrai si le graphe a été modifié */
	bool besoin_ajournement = false;

	/* Données personnalisables pour les sur-entrées de ce graphe. */
	dls::tableau<std::any> entrees{};

	/* Données extras personnalisables de ce graphe. */
	dls::tableau<std::any> donnees{};

	/* entreface de programmation */

	using iterateur = dls::tableau<Noeud *>::iteratrice;
	using iterateur_const = dls::tableau<Noeud *>::const_iteratrice;

	using plage_noeud = dls::outils::plage_iterable<iterateur>;
	using plage_noeud_const = dls::outils::plage_iterable<iterateur_const>;

	Graphe(Noeud &parent);

	~Graphe();

	/* À FAIRE : considère l'utilisation de shared_ptr */
	Graphe(Graphe const &) = default;
	Graphe &operator=(Graphe const &) = default;

	Noeud *cree_noeud(dls::chaine const &nom_noeud, type_noeud type_n);

	/**
	 * Supprime un noeud du graphe. Le noeud est deconnecté de tous les noeuds
	 * auquel il était connecté puis est détruit. Si le noeud ne fait pas partie
	 * du graphe, il ne sera pas détruit.
	 */
	void supprime(Noeud *noeud);

	/**
	 * Connecte la prise de sortie à la prise d'entrée. Si la prise d'entrée est
	 * déjà connectée, aucune connexion n'est créée. Les noeuds en aval du
	 * noeud parent de la prise de sortie sont marqués comme surannés.
	 */
	void connecte(PriseSortie *sortie, PriseEntree *entree);

	/**
	 * Déconnecte la prise de sortie de la prise d'entrée. Si la prise d'entrée
	 * n'était pas connectée à la prise de sortie, aucune déconnexion n'est
	 * effectuée. Les noeuds en aval du noeud parent de la prise de sortie sont
	 * marqués comme surannés.
	 *
	 * Retourne vrai si une déconnexion a eu lieu.
	 */
	bool deconnecte(PriseSortie *sortie, PriseEntree *entree);

	/**
	 * Retourne une plage itérable couvrant les noeuds du graphe. Les noeuds ne
	 * sont pas triés selon un certain ordre.
	 */
	plage_noeud noeuds();

	/**
	 * Retourne une plage itérable constante couvrant les noeuds du graphe. Les
	 * noeuds ne sont pas triés selon un certain ordre.
	 */
	plage_noeud_const noeuds() const;

	/**
	 * Ajoute le noeud spécifié à l'ensemble des noeuds sélectionnés. Le
	 * pointeur vers le noeud actif est ajourné pour être égal au noeud passé en
	 * paramètre.
	 */
	void ajoute_selection(Noeud *noeud);

	/**
	 * Vide l'ensemble des noeuds sélectionnés. Le pointeur vers le noeud actif
	 * est ajourné pour être égal à nullptr.
	 */
	void vide_selection();

	/**
	 * Supprime tous les noeuds et vide la sélection.
	 */
	void supprime_tout();

private:
	/**
	 * Ajoute un noeud au graphe. Le noeud n'est pas connecté. Le nom du noeud
	 * est ajourné pour être unique : le nom est suffixé selon le nombre de
	 * noeuds qui ont déjà le même nom. Finalement, le noeud est ajouté à la
	 * sélection après avoir vidé celle-ci.
	 */
	void ajoute(Noeud *noeud);

	void supprime_noeud(Noeud *noeud);

	dls::tableau<Noeud *> m_noeuds{};
	dls::ensemble<Noeud *> m_noeuds_selectionnes{};

	dls::ensemble<dls::chaine> m_noms_noeuds{};
};

/* ************************************************************************** */

/**
 * Trouve le noeud dans le graphe selon les coordonnées x et y spécifiées.
 * Retourne nullptr si aucun noeud n'est trouvé.
 */
Noeud *trouve_noeud(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y);

/**
 * Trouve la prise d'entrée dans le graphe selon les coordonnées x et y
 * spécifiées. Retourne nullptr si aucune prise n'est trouvée.
 */
PriseEntree *trouve_prise_entree(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y);

/**
 * Trouve la prise de sortie dans le graphe selon les coordonnées x et y
 * spécifiées. Retourne nullptr si aucune prise n'est trouvée.
 */
PriseSortie *trouve_prise_sortie(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y);

/**
 * Trouve le noeud et la prise dans le graphe selon les coordonnées x et y
 * spécifiées.
 * Si aucun noeud n'est trouvé, les trois pointeurs spécifiés seront nuls.
 * Si un noeud est trouvé, mais aucune prise ne l'est, les pointeurs de prises
 * seront nuls.
 * Si un noeud est trouvé et une prise aussi, le pointeur de la prise
 * correspondante, soit d'entrée, soit de sortie, sera ajourné et l'autre sera
 * nul.
 */
void trouve_noeud_prise(Graphe::plage_noeud noeuds,
		const float x,
		const float y,
		Noeud *&noeud_r,
		PriseEntree *&prise_entree,
		PriseSortie *&prise_sortie);

/**
 * Performe un tri topologique du graphe où les noeuds sont triés en fonction
 * de leurs degrés d'entrée.
 */
void tri_topologique(Graphe &graphe);

/**
 * Implémentation de la logique de tri topologique. Le prédicat est la partie
 * qui permet de spécialiser le tri pour chaque graphe.
 */
template <typename I, typename P>
void tri_topologique(I debut, I fin, P predicat)
{
	while (debut != fin) {
		auto nouveau_debut = std::partition(debut, fin, predicat);

		/* Aucune solution n'a été trouvée, il est possible qu'il y ait
		 * un cycle dans le graphe. */
		if (nouveau_debut == debut) {
			break;
		}

		debut = nouveau_debut;
	}
}

