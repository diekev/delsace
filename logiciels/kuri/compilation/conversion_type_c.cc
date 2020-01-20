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

void cree_typedef(
		ContexteGenerationCode &contexte,
		DonneesTypeFinal &donnees,
		dls::flux_chaine &os)
{
	auto const &nom_broye = nom_broye_type(contexte, donnees);

	if (donnees.type_base() == id_morceau::TABLEAU || donnees.type_base() == id_morceau::TROIS_POINTS) {
		if (est_invalide(donnees.dereference())) {
			return;
		}

		os << "typedef struct Tableau_" << nom_broye;

		os << "{\n\t";

		converti_type_C(contexte, "", donnees.dereference(), os, false, true);

		os << " *pointeur;\n\tlong taille;\n} " << nom_broye << ";\n\n";
	}
	else if ((donnees.type_base() & 0xff) == id_morceau::TABLEAU) {
		os << "typedef ";
		converti_type_C(contexte, "", donnees.dereference(), os, false, true);
		os << ' ' << nom_broye;
		os << '[' << static_cast<size_t>(donnees.type_base() >> 8) << ']';
		os << ";\n\n";
	}
	else if (donnees.type_base() == id_morceau::COROUT) {
		/* ne peut prendre un pointeur vers une coroutine pour le moment */
	}
	else if (donnees.type_base() == id_morceau::FONC) {
		auto nombre_types_retour = 0l;
		auto type_parametres = donnees_types_parametres(contexte.typeuse, donnees, nombre_types_retour);

		auto prefixe = dls::chaine("");
		auto suffixe = dls::chaine("");

		auto nombre_types_entree = type_parametres.taille() - nombre_types_retour;

		auto nouveau_nom_broye = dls::chaine("Kf");
		nouveau_nom_broye += dls::vers_chaine(nombre_types_entree);

		if (nombre_types_retour > 1) {
			prefixe += "void (*";
		}
		else {
			auto &dt = contexte.typeuse[type_parametres.back()];
			auto const &nom_broye_dt = nom_broye_type(contexte, dt);
			prefixe += nom_broye_dt + " (*";
		}

		auto virgule = "(";

		for (auto i = 0; i < nombre_types_entree; ++i) {
			auto &dt = contexte.typeuse[type_parametres[i]];
			auto const &nom_broye_dt = nom_broye_type(contexte, dt);

			suffixe += virgule;
			suffixe += nom_broye_dt;
			nouveau_nom_broye += nom_broye_dt;
			virgule = ",";
		}

		if (nombre_types_entree == 0) {
			suffixe += virgule;
			virgule = ",";
		}

		nouveau_nom_broye += dls::vers_chaine(nombre_types_retour);

		for (auto i = nombre_types_entree; i < type_parametres.taille(); ++i) {
			auto &dt = contexte.typeuse[type_parametres[i]];
			auto const &nom_broye_dt = nom_broye_type(contexte, dt);

			if (nombre_types_retour > 1) {
				suffixe += virgule;
				suffixe += nom_broye_dt;
			}

			nouveau_nom_broye += nom_broye_dt;
		}

		suffixe += ")";

		donnees.nom_broye = nouveau_nom_broye;

		nouveau_nom_broye = prefixe + nouveau_nom_broye + ")" + suffixe;

		os << "typedef " << nouveau_nom_broye << ";\n\n";
	}
	/* cas spécial pour les types complexes : &[]z8 */
	else if (donnees.type_base() == id_morceau::POINTEUR || donnees.type_base() == id_morceau::REFERENCE) {
		auto index = contexte.typeuse.ajoute_type(donnees.dereference());
		auto &dt_deref = contexte.typeuse[index];

		auto nom_broye_deref = nom_broye_type(contexte, dt_deref);

		os << "typedef " << nom_broye_deref << "* " << nom_broye << ";\n\n";
	}
	else {
		os << "typedef ";
		converti_type_C(contexte, "", donnees.plage(), os, false, true);
		os << ' ' << nom_broye <<  ";\n\n";
	}
}

static auto converti_type_simple_C(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os,
		id_morceau id,
		bool echappe,
		bool echappe_struct,
		bool echappe_tableau_fixe)
{
	switch (id & 0xff) {
		case id_morceau::POINTEUR:
		{
			if (echappe) {
				os << "_ptr_";
			}
			else {
				os << '*';
			}

			break;
		}
		case id_morceau::REFERENCE:
		{
			if (echappe) {
				os << "_ref_";
			}
			else {
				os << '*';
			}

			break;
		}
		case id_morceau::TABLEAU:
		{
			if (echappe_tableau_fixe) {
				os << '*';
				return;
			}

			auto taille = static_cast<size_t>(id >> 8);
			os << '[' << taille << ']';

			break;
		}
		case id_morceau::OCTET:
		{
			os << "octet";
			break;
		}
		case id_morceau::BOOL:
		{
			os << "bool";
			break;
		}
		case id_morceau::N8:
		{
			if (echappe) {
				os << "unsigned_char";
			}
			else {
				os << "unsigned char";
			}

			break;
		}
		case id_morceau::N16:
		{
			if (echappe) {
				os << "unsigned_short";
			}
			else {
				os << "unsigned short";
			}

			break;
		}
		case id_morceau::N32:
		{
			if (echappe) {
				os << "unsigned_int";
			}
			else {
				os << "unsigned int";
			}

			break;
		}
		case id_morceau::N64:
		{
			if (echappe) {
				os << "unsigned_long";
			}
			else {
				os << "unsigned long";
			}

			break;
		}
		case id_morceau::N128:
		{
			os << ((echappe) ? "unsigned_long_long" : "unsigned long long");
			break;
		}
		case id_morceau::R16:
		{
			os << "r16";
			break;
		}
		case id_morceau::R32:
		{
			os << "float";
			break;
		}
		case id_morceau::R64:
		{
			os << "double";
			break;
		}
		case id_morceau::R128:
		{
			os << ((echappe) ? "long_double" : "long double");
			break;
		}
		case id_morceau::Z8:
		{
			os << "char";
			break;
		}
		case id_morceau::Z16:
		{
			os << "short";
			break;
		}
		case id_morceau::Z32:
		{
			os << "int";
			break;
		}
		case id_morceau::Z64:
		{
			os << "long";
			break;
		}
		case id_morceau::Z128:
		{
			os << ((echappe) ? "long_long" : "long long");
			break;
		}
		case id_morceau::CHAINE:
		{
			os << "chaine";
			break;
		}
		case id_morceau::COROUT:
		case id_morceau::FONC:
		{
			break;
		}
		case id_morceau::PARENTHESE_OUVRANTE:
		{
			os << '(';
			break;
		}
		case id_morceau::PARENTHESE_FERMANTE:
		{
			os << ')';
			break;
		}
		case id_morceau::VIRGULE:
		{
			os << ',';
			break;
		}
		case id_morceau::NUL: /* pour par exemple quand on retourne nul */
		case id_morceau::RIEN:
		{
			os << "void";
			break;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto id_struct = static_cast<long>(id >> 8);
			auto &donnees_struct = contexte.donnees_structure(id_struct);
			auto nom_structure = contexte.nom_struct(id_struct);

			if (donnees_struct.est_enum) {
				auto &dt = contexte.typeuse[donnees_struct.noeud_decl->index_type];
				converti_type_simple_C(contexte, os, dt.type_base(), false, false, false);
			}
			else {
				if (donnees_struct.est_union && donnees_struct.est_nonsur) {
					os << ((echappe) ? "union_" : "union ");
				}
				else if (echappe_struct || donnees_struct.est_externe) {
					os << ((echappe) ? "struct_" : "struct ");
				}

				os << broye_nom_simple(nom_structure);
			}

			break;
		}
		case id_morceau::EINI:
		{
			os << "eini";
			break;
		}
		case id_morceau::TYPE_DE:
		{
			assert(false && "type_de aurait dû être résolu");
			break;
		}
		default:
		{
			assert(false);
			break;
		}
	}

	return;
}

/* a : [2]z32 -> int x[2]
 * b : [4][4]r64 -> double y[4][4]
 * c : *bool -> bool *z
 * d : **bool -> bool **z
 * e : &bool -> bool *e
 * f : [2]*z32 -> int *f[2]
 * g : []z32 -> struct Tableau
 *
 * À FAIRE pour les accès ou assignations :
 * z : [4][4]z32 -> int (*z)[4] (pour l'instant -> int **z)
 */
void converti_type_C(
		ContexteGenerationCode &contexte,
		dls::vue_chaine const &nom_variable,
		type_plage_donnees_type donnees,
		dls::flux_chaine &os,
		bool echappe,
		bool echappe_struct,
		bool echappe_tableau_fixe)
{
	if (est_invalide(donnees)) {
		return;
	}

	if (donnees.front() == id_morceau::TABLEAU || donnees.front() == id_morceau::TROIS_POINTS) {
		if (echappe_struct) {
			os << "struct ";
		}

		os << "Tableau_";
		donnees.effronte();
		converti_type_C(contexte, nom_variable, donnees, os, true);
		return;
	}

	/* cas spécial pour convertir les types complexes comme *[]z8 */
	if (donnees.front() == id_morceau::POINTEUR || donnees.front() == id_morceau::REFERENCE) {
		donnees.effronte();
		converti_type_C(contexte, "", donnees, os, echappe, echappe_struct);

		if (echappe) {
			os << "_ptr_";
		}
		else {
			os << '*';
		}

		if (nom_variable != "") {
			os << ' ' << nom_variable;
		}

		return;
	}

	if (donnees.front() == id_morceau::FONC || donnees.front() == id_morceau::COROUT) {
		dls::tableau<dls::pile<id_morceau>> liste_pile_type;

		auto nombre_types_retour = 0l;
		auto parametres_finis = false;

		/* saute l'id fonction */
		donnees.effronte();

		dls::pile<id_morceau> pile;

		while (!donnees.est_finie()) {
			auto donnee = donnees.front();
			donnees.effronte();

			if (donnee == id_morceau::PARENTHESE_OUVRANTE) {
				/* RAF */
			}
			else if (donnee == id_morceau::PARENTHESE_FERMANTE) {
				/* évite d'empiler s'il n'y a pas de paramètre, càd 'foo()' */
				if (!pile.est_vide()) {
					liste_pile_type.pousse(pile);
				}

				pile = dls::pile<id_morceau>{};

				if (parametres_finis) {
					++nombre_types_retour;
				}

				parametres_finis = true;
			}
			else if (donnee == id_morceau::VIRGULE) {
				liste_pile_type.pousse(pile);
				pile = dls::pile<id_morceau>{};

				if (parametres_finis) {
					++nombre_types_retour;
				}
			}
			else {
				pile.empile(donnee);
			}
		}

		if (nombre_types_retour == 1) {
			auto &pile_type = liste_pile_type[liste_pile_type.taille() - nombre_types_retour];

			while (!pile_type.est_vide()) {
				converti_type_simple_C(
							contexte,
							os,
							pile_type.depile(),
							echappe,
							echappe_struct,
							echappe_tableau_fixe);
			}
		}
		else {
			os << "void ";
		}

		os << "(*" << nom_variable << ")";

		auto virgule = '(';

		if (liste_pile_type.taille() == nombre_types_retour) {
			os << virgule;
			virgule = ' ';
		}
		else {
			for (auto i = 0l; i < liste_pile_type.taille() - nombre_types_retour; ++i) {
				os << virgule;

				auto &pile_type = liste_pile_type[i];

				while (!pile_type.est_vide()) {
					converti_type_simple_C(
								contexte,
								os,
								pile_type.depile(),
								echappe,
								echappe_struct,
								echappe_tableau_fixe);
				}

				virgule = ',';
			}
		}

		if (nombre_types_retour > 1) {
			for (auto i = liste_pile_type.taille() - nombre_types_retour; i < liste_pile_type.taille(); ++i) {
				os << virgule;

				auto &pile_type = liste_pile_type[i];

				while (!pile_type.est_vide()) {
					converti_type_simple_C(
								contexte,
								os,
								pile_type.depile(),
								echappe,
								echappe_struct,
								echappe_tableau_fixe);
				}

				os << '*';
				virgule = ',';
			}
		}

		os << ')';

		return;
	}

	auto nom_consomme = false;

	while (!donnees.est_finie()) {
		auto donnee = donnees.cul();
		donnees.ecule();

		if (est_type_tableau_fixe(donnee) && nom_consomme == false && nom_variable != "" && !echappe_tableau_fixe) {
			os << ' ' << nom_variable;
			nom_consomme = true;
		}

		converti_type_simple_C(contexte, os, donnee, echappe, echappe_struct, echappe_tableau_fixe);
	}

	if (nom_consomme == false && nom_variable != "") {
		os << ' ' << nom_variable;
	}
}
