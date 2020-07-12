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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "structures.hh"

/* Structures utilisées pour passer les arbres syntaxiques aux métaprogrammes.
 * Nous utilisons des structures différents que celles de l'arbre syntaxique
 * afin de pouvoir cacher certaines informations propres au compilateur, et
 * également pour se prémunir de problèmes d'interface binaire dans le future si
 * nous changeons la structure de l'arbre syntaxique, pour ne pas briser les
 * métaprogrammes existants.
 *
 * Ces structures doivent être synchronisé avec celles du module Compilatrice.
 */

struct NoeudCodeBloc;
struct NoeudCodeDeclaration;
struct NoeudExpression;

struct NoeudCode {
	int genre;
};

struct NoeudCodeFonction : public NoeudCode {
	kuri::chaine nom{};

	NoeudCodeBloc *bloc = nullptr;

	kuri::tableau<NoeudCodeDeclaration *> params_entree{};
	kuri::tableau<NoeudCodeDeclaration *> params_sortie{};
};

struct NoeudCodeAssignation : public NoeudCode {
	NoeudCodeDeclaration *assigne = nullptr;
	NoeudCode *expression = nullptr;
};

struct NoeudCodeDeclaration : public NoeudCode {
};

struct NoeudCodeOperationUnaire : public NoeudCode {
	NoeudCode *operande = nullptr;
};

struct NoeudCodeOperationBinaire : public NoeudCode {
	NoeudCode *operande_gauche = nullptr;
	NoeudCode *operande_droite = nullptr;
};

struct NoeudCodeBloc : public NoeudCode {
	kuri::tableau<NoeudCode *> expressions{};
	kuri::tableau<NoeudCodeDeclaration *> membres{};
};

struct ConvertisseuseNoeudCode {
	tableau_page<NoeudCode> noeuds_codes{};
	tableau_page<NoeudCodeFonction> noeuds_fonctions{};
	tableau_page<NoeudCodeAssignation> noeuds_assignations{};
	tableau_page<NoeudCodeDeclaration> noeuds_declarations{};
	tableau_page<NoeudCodeOperationUnaire> noeuds_operations_unaire{};
	tableau_page<NoeudCodeOperationBinaire> noeuds_operations_binaire{};
	tableau_page<NoeudCodeBloc> noeuds_blocs{};

	NoeudCode *converti_noeud_syntaxique(NoeudExpression *noeud_expression);
};
