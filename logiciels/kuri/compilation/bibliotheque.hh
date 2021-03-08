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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine.hh"
#include "structures/tableau_compresse.hh"

#include "identifiant.hh"

struct Bibliotheque;
struct Espace;
struct NoeudExpression;
struct Statistiques;

//// pour Alembic
//lib_abc :: #bibliothèque ""
//lib_abc_kuri :: #bibliothèque ""

//#dépendance_bibliothèque lib_abc_kuri lib_abc

//#externe libc
//#externe lib_pthread

//directive #externe : argument définissant la bibliothèque où trouver le symbole
//- libc :: #bibiliothèque_externe "c"
//- write :: fonc (...) -> z32 #externe libc

//    // pour le réusinage des bibliothèques

// -- ajout d'un identifiant après "#externe" pour les fonctions
// -- ajout d'une chaine possible après l'identifiant suscité pour définir le nom du symbole
// -- ajout de Symbole *symbole à NoeudDeclarationEnteteFonction pour définir le symbole dans la bibliothèque
// -- ajout d'un noeud syntaxique ?
// -- définis libc, libpthread, libm, etc. par la compilatrice

struct Symbole {
	Bibliotheque *bibliotheque = nullptr;
	kuri::chaine nom = "";
	// dso::symbole;
	// pointeur pour appel;
};

struct Bibliotheque {
	IdentifiantCode *ident = nullptr;
	NoeudExpression *site = nullptr;

	kuri::chaine nom = "";

	kuri::chaine chemin_statique = "";
	kuri::chaine chemin_dynamique = "";

	// dso::shared_object

	kuri::tableau_compresse<Bibliotheque *, int> dependances{};
	tableau_page<Symbole *> symboles;

	Symbole *symbole(kuri::chaine_statique nom_symbole);
};

struct GestionnaireBibliotheque {
	Espace *espace = nullptr;
	tableau_page<Bibliotheque> bibliotheques{};

	GestionnaireBibliotheque(Espace *espace_)
		: espace(espace_)
	{}

	Bibliotheque *trouve_bibliotheque(NoeudExpression *site);

	Bibliotheque *cree_bibliotheque(NoeudExpression *site);

	void rassemble_statistiques(Statistiques &stats);
};
