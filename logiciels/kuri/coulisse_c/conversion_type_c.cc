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

#include "conversion_type_c.hh"

#include "broyage.hh"
#include "contexte_generation_code.h"

#include "generatrice_code_c.hh"

void cree_typedef(Type *type, GeneratriceCodeC &generatrice)
{
	auto const &nom_broye = nom_broye_type(type, true);

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			break;
		}
		case GenreType::ERREUR:
		case GenreType::ENUM:
		{
			/* RÀF : les énumérations sont des constantes et nous utilisons le
			 * type des valeurs dans le code C. */
			break;
		}
		case GenreType::BOOL:
		{
			generatrice << "typedef unsigned char " << nom_broye << ";\n";
			break;
		}
		case GenreType::OCTET:
		{
			generatrice << "typedef unsigned char " << nom_broye << ";\n";
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				generatrice << "typedef unsigned char " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 2) {
				generatrice << "typedef unsigned short " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 4) {
				generatrice << "typedef unsigned int " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 8) {
				generatrice << "typedef unsigned long " << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				generatrice << "typedef char " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 2) {
				generatrice << "typedef short " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 4) {
				generatrice << "typedef int " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 8) {
				generatrice << "typedef long " << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				generatrice << "typedef short " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 4) {
				generatrice << "typedef float " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 8) {
				generatrice << "typedef double " << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::REFERENCE:
		{
			auto type_pointe = static_cast<TypeReference *>(type)->type_pointe;
			generatrice << "typedef " << nom_broye_type(type_pointe, true) << "* " << nom_broye << ";\n";
			break;
		}
		case GenreType::POINTEUR:
		{
			auto type_pointe = static_cast<TypePointeur *>(type)->type_pointe;
			generatrice << "typedef " << nom_broye_type(type_pointe, true) << "* " << nom_broye << ";\n";
			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto nom_struct = broye_nom_simple(type_struct->nom);

			if (nom_struct != "pthread_mutex_t" && nom_struct != "pthread_cond_t" && nom_struct != "MY_CHARSET_INFO") {
				generatrice << "typedef struct " << nom_struct << ' ' << nom_broye << ";\n";
			}
			else {
				generatrice << "typedef " << nom_struct << ' ' << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = static_cast<TypeUnion *>(type);
			auto nom_struct = broye_nom_simple(type_struct->nom);
			auto decl = type_struct->decl;

			if (nom_struct != "pthread_mutex_t" && nom_struct != "pthread_cond_t") {
				if (decl->est_nonsure || decl->est_externe) {
					generatrice << "typedef union " << nom_struct << ' ' << nom_broye << ";\n";
				}
				else {
					generatrice << "typedef struct " << nom_struct << ' ' << nom_broye << ";\n";
				}
			}
			else {
				generatrice << "typedef " << nom_struct << ' ' << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_pointe = static_cast<TypeTableauFixe *>(type)->type_pointe;

			// [X][X]Type
			if (type_pointe->genre == GenreType::TABLEAU_FIXE) {
				auto type_tabl = static_cast<TypeTableauFixe *>(type_pointe);
				auto taille_tableau = type_tabl->taille;

				generatrice << "typedef " << nom_broye_type(type_tabl->type_pointe, true);
				generatrice << "(" << nom_broye << ')';
				generatrice << '[' << static_cast<TypeTableauFixe *>(type)->taille << ']';
				generatrice << '[' << taille_tableau << ']';
				generatrice << ";\n\n";
			}
			else {
				generatrice << "typedef " << nom_broye_type(type_pointe, true);
				generatrice << ' ' << nom_broye;
				generatrice << '[' << static_cast<TypeTableauFixe *>(type)->taille << ']';
				generatrice << ";\n\n";
			}

			break;
		}
		case GenreType::VARIADIQUE:
		{
			// les arguments variadiques sont transformés en tableaux, donc RÀF
			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_pointe = static_cast<TypeTableauDynamique *>(type)->type_pointe;

			if (type_pointe == nullptr) {
				return;
			}

			generatrice << "typedef struct Tableau_" << nom_broye;

			generatrice << "{\n\t";

			if (type_pointe->genre == GenreType::TABLEAU_FIXE) {
				auto type_tabl = static_cast<TypeTableauFixe *>(type_pointe);
				auto taille_tableau = type_tabl->taille;
				generatrice << nom_broye_type(type_tabl->type_pointe, true) << " *pointeur[" << taille_tableau << "];";
			}
			else {
				generatrice << nom_broye_type(type_pointe, true) << " *pointeur;";
			}

			generatrice << "\n\tlong taille;\n} " << nom_broye << ";\n\n";
			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = static_cast<TypeFonction *>(type);

			auto prefixe = dls::chaine("");
			auto suffixe = dls::chaine("");

			auto nouveau_nom_broye = dls::chaine("Kf");
			nouveau_nom_broye += dls::vers_chaine(type_fonc->types_entrees.taille);

			if (type_fonc->types_sorties.taille > 1) {
				prefixe += "void (*";
			}
			else {
				auto const &nom_broye_dt = nom_broye_type(type_fonc->types_sorties[0], true);
				prefixe += nom_broye_dt + " (*";
			}

			auto virgule = "(";

			POUR (type_fonc->types_entrees) {
				auto const &nom_broye_dt = nom_broye_type(it, true);

				suffixe += virgule;
				suffixe += nom_broye_dt;
				nouveau_nom_broye += nom_broye_dt;
				virgule = ",";
			}

			if (type_fonc->types_entrees.taille == 0) {
				suffixe += virgule;
				virgule = ",";
			}

			nouveau_nom_broye += dls::vers_chaine(type_fonc->types_sorties.taille);

			POUR (type_fonc->types_sorties) {
				auto const &nom_broye_dt = nom_broye_type(it, true);

				if (type_fonc->types_sorties.taille > 1) {
					suffixe += virgule;
					suffixe += nom_broye_dt;
				}

				nouveau_nom_broye += nom_broye_dt;
			}

			suffixe += ")";

			type->nom_broye = nouveau_nom_broye;

			nouveau_nom_broye = prefixe + nouveau_nom_broye + ")" + suffixe;

			generatrice << "typedef " << nouveau_nom_broye << ";\n\n";

			break;
		}
		case GenreType::EINI:
		{
			generatrice << "typedef eini " << nom_broye << ";\n";
			break;
		}
		case GenreType::RIEN:
		{
			generatrice << "typedef void " << nom_broye << ";\n";
			break;
		}
		case GenreType::CHAINE:
		{
			generatrice << "typedef chaine " << nom_broye << ";\n";
			break;
		}
	}
}
