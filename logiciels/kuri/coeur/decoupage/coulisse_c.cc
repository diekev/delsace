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

#include "coulisse_c.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include <cassert>
#include <iostream>

#include "arbre_syntactic.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "modules.hh"
#include "validation_semantique.hh"

using denombreuse = lng::decoupeuse_nombre<id_morceau>;

namespace noeud {

/* ************************************************************************** */

#if 0
struct Tampon {
	char tampon[16384];
	char *ptr;
};

// imprime(tampon, "for (%0 %1 = %2; %1 <= %3 - 1; ++%1) {\n", {str_type, str_nom, str_expr_debut, str_expr_fin});

static auto imprime(
		Tampon &tampon,
		const char *chn_fmt,
		dls::tableau<const char *> const &args)
{
	auto ptr = chn_fmt;
	auto ptr_tampon = tampon.ptr;

	for (;*ptr != '\0'; ++ptr) {
		if (*ptr != '%') {
			*ptr_tampon++ = *ptr;
			continue;
		}

		auto suivant = *(ptr + 1);
		++ptr;

		if (suivant == '%') {
			*ptr_tampon++ = suivant;
			continue;
		}

		auto index = static_cast<size_t>('0' - *ptr);

		auto chn_arg = args[index];

		while (*chn_arg != '\0') {
			*ptr_tampon++ = *chn_arg++;
		}
	}
}
#endif

/* ************************************************************************** */

static void genere_code_extra_pre_retour(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	if (contexte.donnees_fonction->est_coroutine) {
		os << "__etat->__termine_coro = 1;\n";
	}

	/* génère le code pour les blocs déférés */
	auto pile_noeud = contexte.noeuds_differes();

	while (!pile_noeud.est_vide()) {
		auto noeud = pile_noeud.back();
		genere_code_C(noeud, contexte, true, os, os);
		pile_noeud.pop_back();
	}
}

/* ************************************************************************** */

static inline auto broye_chaine(base *b)
{
	return broye_nom_simple(b->chaine());
}

/* ************************************************************************** */

/* À FAIRE : trouve une bonne manière de générer des noms uniques. */
static int index = 0;

static auto cree_info_type_defaul_C(
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		dls::chaine const &id_type)
{
	auto nom_info_type = "__info_type" + id_type + dls::vers_chaine(index++);

	os_decl << "static InfoType " << nom_info_type << ";\n";
	os_init << nom_info_type << ".id = " << id_type << ";\n";

	return nom_info_type;
}

static auto cree_info_type_entier_C(
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		int taille_en_octet,
		bool est_signe)
{
	auto nom_info_type = "__info_type_entier" + dls::vers_chaine(index++);

	os_decl << "static InfoTypeEntier " << nom_info_type << ";\n";
	os_init << nom_info_type << ".id = id_info_ENTIER;\n";
	os_init << nom_info_type << broye_nom_simple(".est_signé = ") << est_signe << ";\n";
	os_init << nom_info_type << ".taille_en_octet = " << taille_en_octet << ";\n";

	return nom_info_type;
}

static auto cree_info_type_reel_C(
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		int taille_en_octet)
{
	auto nom_info_type = "__info_type_reel" + dls::vers_chaine(index++);

	os_decl << broye_nom_simple("static InfoTypeRéel ") << nom_info_type << ";\n";
	os_init << nom_info_type << broye_nom_simple(".id = id_info_RÉEL;\n");
	os_init << nom_info_type << ".taille_en_octet = " << taille_en_octet << ";\n";

	return nom_info_type;
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		DonneesType &donnees_type);

static unsigned int taille_type_octet(ContexteGenerationCode &contexte, DonneesType const &donnees_type)
{
	auto type_base = donnees_type.type_base();

	switch (type_base & 0xff) {
		default:
		{
			assert(false);
			break;
		}
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::Z8:
		{
			return 1;
		}
		case id_morceau::N16:
		case id_morceau::Z16:
		{
			return 2;
		}
		case id_morceau::R16:
		{
			/* À FAIRE : type r16 */
			return 4;
		}
		case id_morceau::N32:
		case id_morceau::Z32:
		case id_morceau::R32:
		{
			return 4;
		}
		case id_morceau::N64:
		case id_morceau::Z64:
		case id_morceau::R64:
		{
			return 8;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto index_struct = static_cast<long>(type_base >> 8);
			auto &ds = contexte.donnees_structure(index_struct);

			if (ds.est_enum) {
				auto dt_enum = contexte.magasin_types.donnees_types[ds.noeud_decl->index_type];
				return taille_type_octet(contexte, dt_enum);
			}

			/* À FAIRE */
			return 0;
		}
		case id_morceau::POINTEUR:
		case id_morceau::FONC:
		{
			/* À FAIRE : pointeur 32-bit/64-bit */
			return 8;
		}
		case id_morceau::TABLEAU:
		case id_morceau::EINI:
		case id_morceau::CHAINE:
		{
			return 16;
		}
		case id_morceau::RIEN:
		{
			return 0;
		}
	}

	return 0;
}

static auto cree_info_type_structure_C(
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		ContexteGenerationCode &contexte,
		dls::vue_chaine const &nom_struct,
		DonneesStructure const &donnees_structure,
		DonneesType &dt)
{
	auto nom_info_type = "__info_type_struct" + dls::vers_chaine(index++);

	auto nom_broye = broye_nom_simple(nom_struct);

	/* met en place le 'pointeur' direction au cas où une structure s'incluerait
	 * elle-même via un pointeur */
	dt.ptr_info_type = nom_info_type;

	/* crée la chaine pour le nom */
	auto nom_chaine = "__nom_" + nom_broye + dls::vers_chaine(index++);

	os_init << "static chaine " << nom_chaine << " = ";
	os_init << "{.pointeur = \"" << nom_struct << "\""
	   << ", .taille = " << nom_struct.taille() << "};\n";

	os_decl << "static InfoTypeStructure " << nom_info_type << ";\n";
	os_init << nom_info_type << ".id = id_info_STRUCTURE;\n";
	os_init << nom_info_type << ".nom = " << nom_chaine << ";\n";

	/* crée un tableau fixe puis converti le en tableau dyn */
	auto nombre_membres = donnees_structure.donnees_types.taille();

	if (donnees_structure.est_externe && nombre_membres == 0) {
		os_init << nom_info_type << ".membres.taille = 0;\n";
		os_init << nom_info_type << ".membres.pointeur = 0;\n";
		return nom_info_type;
	}

	/* crée des structures pour chaque membre, et rassemble les pointeurs */
	dls::tableau<dls::chaine> pointeurs;
	pointeurs.reserve(nombre_membres);

	auto decalage = 0u;

	for (auto i = 0l; i < donnees_structure.donnees_types.taille(); ++i) {
		auto index_dt = donnees_structure.donnees_types[i];
		auto &dt_membre = contexte.magasin_types.donnees_types[index_dt];

		for (auto paire_idx_mb : donnees_structure.donnees_membres) {
			if (paire_idx_mb.second.index_membre != i) {
				continue;
			}

			auto suffixe = dls::vers_chaine(i) + dls::vers_chaine(index++);

			auto nom_info_type_membre = "__info_type_membre" + suffixe;

			auto idx = contexte.magasin_types.ajoute_type(dt_membre);
			auto &rderef = contexte.magasin_types.donnees_types[idx];

			if (rderef.ptr_info_type == "") {
				rderef.ptr_info_type = cree_info_type_C(
							contexte, os_decl, os_init, dt_membre);
			}

			auto align_type = alignement(contexte, dt_membre);
			auto padding = (align_type - (decalage % align_type)) % align_type;
			decalage += padding;

			os_decl << "static InfoTypeMembreStructure " << nom_info_type_membre << ";\n";
			os_init << nom_info_type_membre << ".nom.pointeur = \"" << paire_idx_mb.first << "\";\n";
			os_init << nom_info_type_membre << ".nom.taille = " << paire_idx_mb.first.taille()  << ";\n";
			os_init << nom_info_type_membre << broye_nom_simple(".décalage = ") << decalage << ";\n";
			os_init << nom_info_type_membre << ".id = (InfoType *)(&" << rderef.ptr_info_type  << ");\n";

			decalage += taille_type_octet(contexte, dt_membre);

			pointeurs.pousse(nom_info_type_membre);
			break;
		}
	}

	/* alloue un tableau fixe pour stocker les pointeurs */
	auto idx_type_tabl = contexte.donnees_structure("InfoTypeMembreStructure").id;
	auto type_struct_membre = DonneesType{};
	type_struct_membre.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(idx_type_tabl << 8));

	auto dt_tfixe = DonneesType{};
	dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(nombre_membres << 8));
	dt_tfixe.pousse(type_struct_membre);

	auto nom_tableau_fixe = dls::chaine("__tabl_fix_membres") + dls::vers_chaine(index++);

	contexte.magasin_types.converti_type_C(
				contexte, nom_tableau_fixe, dt_tfixe, os_init);

	os_init << " = ";

	auto virgule = '{';

	for (auto const &ptr : pointeurs) {
		os_init << virgule;
		os_init << ptr;
		virgule = ',';
	}

	os_init << "};\n";

	/* alloue un tableau dynamique */
	auto dt_tdyn = DonneesType{};
	dt_tdyn.pousse(id_morceau::TABLEAU);
	dt_tdyn.pousse(type_struct_membre);

	auto nom_tableau_dyn = dls::chaine("__tabl_dyn_membres") + dls::vers_chaine(index++);

	contexte.magasin_types.converti_type_C(
				contexte, nom_tableau_dyn, dt_tdyn, os_init);

	os_init << ";\n";
	os_init << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
	os_init << nom_tableau_dyn << ".taille = " << nombre_membres << ";\n";

	os_init << nom_info_type << ".membres = " << nom_tableau_dyn << ";\n";

	return nom_info_type;
}

static auto cree_info_type_enum_C(
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		ContexteGenerationCode &contexte,
		dls::vue_chaine const &nom_struct,
		DonneesStructure const &donnees_structure)
{
	auto nom_info_type = "__info_type_enum" + dls::vers_chaine(index++);

	auto nom_broye = broye_nom_simple(nom_struct);

	/* crée la chaine pour le nom */
	auto nom_chaine = "__nom_"
			+ nom_broye
			+ dls::vers_chaine(index++);

	os_init << "static chaine " << nom_chaine << " = ";
	os_init << "{.pointeur = \"" << nom_struct << "\""
	   << ", .taille = " << nom_struct.taille() << "};\n";

	os_decl << broye_nom_simple("static InfoTypeÉnum ") << nom_info_type << ";\n";
	os_init << nom_info_type << broye_nom_simple(".id = id_info_ÉNUM;\n");
	os_init << nom_info_type << ".nom = " << nom_chaine << ";\n";

	auto noeud_decl = donnees_structure.noeud_decl;
	auto nombre_enfants = noeud_decl->enfants.taille();

	/* crée un tableau pour les noms des énumérations */

	auto nom_tabl_fixe = "__noms_membres_"
			+ nom_broye
			+ dls::vers_chaine(index++);

	os_init << "chaine " << nom_tabl_fixe << "[" << nombre_enfants << "] = {\n";

	for (auto enfant : noeud_decl->enfants) {
		auto enf0 = enfant->enfants.front();
		os_init << "{.pointeur=\"" << enf0->chaine() << "\", .taille=" << enf0->chaine().taille() << "},";
	}

	os_init << "};\n";

	os_init << nom_info_type << ".noms.pointeur = " << nom_tabl_fixe << ";\n";
	os_init << nom_info_type << ".noms.taille = " << nombre_enfants << ";\n";


	/* crée un tableau pour les noms des énumérations */
	auto nom_tabl_fixe_vals = "__valeurs_membres_"
			+ nom_broye
			+ dls::vers_chaine(index++);

	auto &dt = contexte.magasin_types.donnees_types[noeud_decl->index_type];

	contexte.magasin_types.converti_type_C(
				contexte,
				"",
				dt,
				os_init);

	os_init << " " << nom_tabl_fixe_vals << "[" << nombre_enfants << "] = {\n";

	for (auto enfant : noeud_decl->enfants) {
		auto enf1 = enfant->enfants.back();

		genere_code_C(enf1, contexte, false, os_init, os_init);

		os_init << ',';
	}

	os_init << "};\n";

	os_init << nom_info_type << ".valeurs.pointeur = " << nom_tabl_fixe_vals << ";\n";
	os_init << nom_info_type << ".valeurs.taille = " << nombre_enfants << ";\n";

	return nom_info_type;
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		DonneesType &donnees_type)
{
	auto valeur = dls::chaine("");

	switch (donnees_type.type_base() & 0xff) {
		default:
		{
			std::cerr << chaine_type(donnees_type, contexte) << '\n';
			std::cerr << chaine_identifiant(donnees_type.type_base() & 0xff) << '\n';
			assert(false);
			break;
		}
		case id_morceau::BOOL:
		{
			valeur = cree_info_type_defaul_C(os_decl, os_init, broye_nom_simple("id_info_BOOLÉEN"));
			break;
		}
		case id_morceau::N8:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 8, false);
			break;
		}
		case id_morceau::OCTET:
		case id_morceau::Z8:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 8, true);
			break;
		}
		case id_morceau::N16:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 16, false);
			break;
		}
		case id_morceau::Z16:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 16, true);
			break;
		}
		case id_morceau::N32:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 32, false);
			break;
		}
		case id_morceau::Z32:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 32, true);
			break;
		}
		case id_morceau::N64:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 64, false);
			break;
		}
		case id_morceau::Z64:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 64, true);
			break;
		}
		case id_morceau::R16:
		{
			/* À FAIRE : type r16 */
			valeur = cree_info_type_reel_C(os_decl, os_init, 32);
			break;
		}
		case id_morceau::R32:
		{
			valeur = cree_info_type_reel_C(os_decl, os_init, 32);
			break;
		}
		case id_morceau::R64:
		{
			valeur = cree_info_type_reel_C(os_decl, os_init, 64);
			break;
		}
		case id_morceau::REFERENCE:
		case id_morceau::POINTEUR:
		{
			auto deref = donnees_type.derefence();

			auto idx = contexte.magasin_types.ajoute_type(deref);
			auto &rderef = contexte.magasin_types.donnees_types[idx];

			if (rderef.ptr_info_type == "") {
				rderef.ptr_info_type = cree_info_type_C(contexte, os_decl, os_init, rderef);
			}

			auto nom_info_type = "__info_type_pointeur" + dls::vers_chaine(index++);

			os_decl << "static InfoTypePointeur " << nom_info_type << ";\n";
			os_init << nom_info_type << ".id = id_info_POINTEUR;\n";
			os_init << nom_info_type << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << rderef.ptr_info_type << ");\n";
			os_init << nom_info_type << broye_nom_simple(".est_référence = ")
					<< (donnees_type.type_base() == id_morceau::REFERENCE) << ";\n";

			valeur = nom_info_type;
			donnees_type.ptr_info_type = valeur;
			break;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto id_structure = static_cast<long>(donnees_type.type_base() >> 8);
			auto donnees_structure = contexte.donnees_structure(id_structure);

			if (donnees_structure.est_enum) {
				valeur = cree_info_type_enum_C(
							os_decl,
							os_init,
							contexte,
							contexte.nom_struct(id_structure),
							donnees_structure);

				donnees_type.ptr_info_type = valeur;
			}
			else {
				valeur = cree_info_type_structure_C(
							os_decl,
							os_init,
							contexte,
							contexte.nom_struct(id_structure),
							donnees_structure,
							donnees_type);
			}

			break;
		}
		case id_morceau::TROIS_POINTS:
		case id_morceau::TABLEAU:
		{
			auto deref = donnees_type.derefence();

			auto nom_info_type = "__info_type_tableau" + dls::vers_chaine(index++);

			/* dans le cas des arguments variadics des fonctions externes */
			if (deref.est_invalide()) {
				os_decl << "static InfoTypeTableau " << nom_info_type << ";\n";
				os_init << nom_info_type << ".id = id_info_TABLEAU;\n";
				os_init << nom_info_type << broye_nom_simple(".type_pointé") << " = 0;\n";
			}
			else {
				auto idx = contexte.magasin_types.ajoute_type(deref);
				auto &rderef = contexte.magasin_types.donnees_types[idx];

				if (rderef.ptr_info_type == "") {
					rderef.ptr_info_type = cree_info_type_C(contexte, os_decl, os_init, rderef);
				}

				os_decl << "static InfoTypeTableau " << nom_info_type << ";\n";
				os_init << nom_info_type << ".id = id_info_TABLEAU;\n";
				os_init << nom_info_type << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << rderef.ptr_info_type << ");\n";
			}

			valeur = nom_info_type;
			break;
		}
		case id_morceau::COROUT:
		case id_morceau::FONC:
		{
			auto nom_info_type = "__info_type_FONCTION" + dls::vers_chaine(index++);

			os_decl << "static InfoTypeFonction " << nom_info_type << ";\n";
			os_init << nom_info_type << ".id = id_info_FONCTION;\n";
			os_init << nom_info_type << ".est_coroutine = " << (donnees_type.type_base() == id_morceau::COROUT) << ";\n";

			donnees_type.ptr_info_type = nom_info_type;

			auto nombre_types_retour = 0l;
			auto dt_params = donnees_types_parametres(contexte.magasin_types, donnees_type, nombre_types_retour);
			auto nombre_types_entree = dt_params.taille() - nombre_types_retour;

			for (auto i = 0; i < dt_params.taille(); ++i) {
				auto &dt_prm = contexte.magasin_types.donnees_types[dt_params[i]];

				if (dt_prm.ptr_info_type != "") {
					continue;
				}

				cree_info_type_C(contexte, os_decl, os_init, dt_prm);
			}

			auto nom_tabl_fix_entree = "__tabl_fix_entree" + dls::vers_chaine(index++);

			os_init << "InfoType *" << nom_tabl_fix_entree << "[" << nombre_types_entree << "] = {";

			auto virgule = ' ';
			for (auto i = 0; i < nombre_types_entree; ++i) {
				auto const &dt_prm = contexte.magasin_types.donnees_types[dt_params[i]];
				os_init << virgule << "(InfoType *)&" << dt_prm.ptr_info_type;
				virgule = ',';
			}

			os_init << "};";

			os_init << nom_info_type << broye_nom_simple(".types_entrée.pointeur = ") << nom_tabl_fix_entree << ";\n";
			os_init << nom_info_type << broye_nom_simple(".types_entrée.taille = ") << nombre_types_entree << ";\n";

			auto nom_tabl_fix_sortie = "__tabl_fix_sortie" + dls::vers_chaine(index++);

			os_init << "InfoType *" << nom_tabl_fix_sortie << "[" << nombre_types_retour << "] = {";

			virgule = ' ';
			for (auto i = nombre_types_entree; i < dt_params.taille(); ++i) {
				auto const &dt_prm = contexte.magasin_types.donnees_types[dt_params[i]];
				os_init << virgule << "(InfoType *)&" << dt_prm.ptr_info_type;
				virgule = ',';
			}

			os_init << "};";

			os_init << nom_info_type << ".types_sortie.pointeur = " << nom_tabl_fix_sortie << ";\n";
			os_init << nom_info_type << ".types_sortie.taille = " << nombre_types_retour << ";\n";

			valeur = nom_info_type;
			break;
		}
		case id_morceau::EINI:
		{
			valeur = cree_info_type_defaul_C(os_decl, os_init, "id_info_EINI");
			break;
		}
		case id_morceau::NUL: /* À FAIRE */
		case id_morceau::RIEN:
		{
			valeur = cree_info_type_defaul_C(os_decl, os_init, "id_info_RIEN");
			break;
		}
		case id_morceau::CHAINE:
		{
			valeur = cree_info_type_defaul_C(os_decl, os_init, "id_info_CHAINE");
			break;
		}
	}

	if (donnees_type.ptr_info_type == "") {
		donnees_type.ptr_info_type = valeur;
	}

	return valeur;
}

static void genere_code_C_prepasse(
		base *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche,
		dls::flux_chaine &os);

static auto cree_eini(ContexteGenerationCode &contexte, dls::flux_chaine &os, base *b)
{
	auto nom_eini = dls::chaine("__eini_")
			.append(dls::vers_chaine(b->morceau.ligne_pos).c_str());

	auto nom_var = dls::chaine{};

	auto &dt = contexte.magasin_types.donnees_types[b->index_type];

	genere_code_C_prepasse(b, contexte, false, os);

	/* dans le cas d'un nombre ou d'un tableau, etc. */
	if (b->type != type_noeud::VARIABLE) {
		nom_var = dls::chaine("__var_").append(nom_eini);

		contexte.magasin_types.converti_type_C(
					contexte,
					nom_var,
					dt,
					os);

		os << " = ";
		genere_code_C(b, contexte, false, os, os);
		os << ";\n";
	}

	os << "eini " << nom_eini << ";\n";
	os << nom_eini << ".pointeur = &";

	if (!nom_var.est_vide()) {
		os << nom_var;
	}
	else {
		genere_code_C(b, contexte, false, os, os);
	}

	os << ";\n";
	os << nom_eini << ".info = (InfoType *)(&" << dt.ptr_info_type << ");\n";

	return nom_eini;
}

static void cree_appel(
		base *b,
		dls::flux_chaine &os,
		ContexteGenerationCode &contexte,
		dls::chaine const &nom_broye,
		dls::liste<base *> const &enfants)
{
	for (auto enf : enfants) {
		if ((enf->drapeaux & CONVERTI_TABLEAU) != 0) {
			auto nom_tabl_fixe = dls::chaine(enf->chaine());

			if (enf->type == type_noeud::CONSTRUIT_TABLEAU) {
				genere_code_C_prepasse(enf, contexte, false, os);
				nom_tabl_fixe = std::any_cast<dls::chaine>(enf->valeur_calculee);
			}

			auto nom_tableau = dls::chaine("__tabl_dyn") + dls::vers_chaine(index++);
			enf->valeur_calculee = nom_tableau;

			os << "Tableau_";
			auto &dt = contexte.magasin_types.donnees_types[enf->index_type];
			contexte.magasin_types.converti_type_C(contexte, "", dt.derefence(), os);

			os << ' ' << nom_tableau << ";\n";
			os << nom_tableau << ".taille = " << static_cast<size_t>(dt.type_base() >> 8) << ";\n";
			os << nom_tableau << ".pointeur = " << nom_tabl_fixe << ";\n";
		}
		else if ((enf->drapeaux & CONVERTI_EINI) != 0) {
			enf->valeur_calculee = cree_eini(contexte, os, enf);
		}
		else if ((enf->drapeaux & EXTRAIT_CHAINE_C) != 0) {
			auto nom_var_chaine = dls::chaine("");

			if (enf->type != type_noeud::VARIABLE) {
				nom_var_chaine = "__chaine" + dls::vers_chaine(enf->morceau.ligne_pos);

				genere_code_C_prepasse(enf, contexte, false, os);
				os << "chaine " << nom_var_chaine << " = ";
				genere_code_C(enf, contexte, false, os, os);
				os << ";\n";
			}
			else {
				nom_var_chaine = enf->chaine();
			}

			auto nom_var = "__pointeur" + dls::vers_chaine(enf->morceau.ligne_pos);

			os << "const char *" + nom_var;
			os << " = " << nom_var_chaine << ".pointeur;\n";

			enf->valeur_calculee = nom_var;
		}
		else if ((enf->drapeaux & CONVERTI_TABLEAU_OCTET) != 0) {
			auto nom_var_tableau = "__tableau_octet" + dls::vers_chaine(enf->morceau.ligne_pos);

			os << "Tableau_octet " << nom_var_tableau << ";\n";

			auto &dt = contexte.magasin_types.donnees_types[enf->index_type];
			auto type_base = dt.type_base();

			switch (type_base & 0xff) {
				default:
				{
					os << nom_var_tableau << ".pointeur = (unsigned char*)(&";
					genere_code_C(enf, contexte, false, os, os);
					os << ");\n";

					os << nom_var_tableau << ".taille = sizeof(";
					contexte.magasin_types.converti_type_C(contexte, "", dt, os);
					os << ");\n";
					break;
				}
				case id_morceau::POINTEUR:
				{
					os << nom_var_tableau << ".pointeur = (unsigned char*)(";
					genere_code_C(enf, contexte, false, os, os);
					os << ".pointeur);\n";

					os << nom_var_tableau << ".taille = ";
					genere_code_C(enf, contexte, false, os, os);
					os << ".sizeof(";
					contexte.magasin_types.converti_type_C(contexte, "", dt.derefence(), os);
					os << ");\n";
					break;
				}
				case id_morceau::CHAINE:
				{
					genere_code_C_prepasse(enf, contexte, false, os);

					os << nom_var_tableau << ".pointeur = ";
					genere_code_C(enf, contexte, false, os, os);
					os << ".pointeur;\n";

					os << nom_var_tableau << ".taille = ";
					genere_code_C(enf, contexte, false, os, os);
					os << ".taille;\n";
					break;
				}
				case id_morceau::TABLEAU:
				{
					auto taille = static_cast<int>(type_base >> 8);

					if (taille == 0) {
						os << nom_var_tableau << ".pointeur = (unsigned char*)(";
						genere_code_C(enf, contexte, false, os, os);
						os << ".pointeur);\n";

						os << nom_var_tableau << ".taille = ";
						genere_code_C(enf, contexte, false, os, os);
						os << ".taille * sizeof(";
						contexte.magasin_types.converti_type_C(contexte, "", dt.derefence(), os);
						os << ");\n";
					}
					else {
						os << nom_var_tableau << ".pointeur = (unsigned char*)(";
						genere_code_C(enf, contexte, false, os, os);
						os << ");\n";

						os << nom_var_tableau << ".taille = " << taille << " * sizeof(";
						contexte.magasin_types.converti_type_C(contexte, "", dt.derefence(), os);
						os << ");\n";
					}

					break;
				}
			}

			enf->valeur_calculee = nom_var_tableau;
		}
		else {
			genere_code_C_prepasse(enf, contexte, false, os);
		}
	}

	auto &dt = contexte.magasin_types.donnees_types[b->index_type];

	dls::liste<base *> liste_var_retour{};
	dls::tableau<dls::chaine> liste_noms_retour{};

	if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET) {
		liste_var_retour = std::any_cast<dls::liste<base *>>(b->valeur_calculee);
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}
	else if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET2) {
		liste_noms_retour = std::any_cast<dls::tableau<dls::chaine>>(b->valeur_calculee);
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}
	else if (dt.type_base() != id_morceau::RIEN && (b->aide_generation_code == APPEL_POINTEUR_FONCTION || ((b->df != nullptr) && !b->df->est_coroutine))) {
		auto nom_indirection = "__ret" + dls::vers_chaine(b->morceau.ligne_pos);
		contexte.magasin_types.converti_type_C(contexte, nom_indirection, dt, os);

		os << " = ";
		b->valeur_calculee = nom_indirection;
	}
	else {
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}

	os << nom_broye;

	auto virgule = '(';

	if (enfants.est_vide() && liste_var_retour.est_vide() && liste_noms_retour.est_vide()) {
		os << virgule;
		virgule = ' ';
	}

	if ((b->df != nullptr) && b->df->est_coroutine) {
		os << virgule << "&__etat" << b->morceau.ligne_pos;
		virgule = ',';
	}

	for (auto enf : enfants) {
		os << virgule;

		if ((enf->drapeaux & CONVERTI_TABLEAU) != 0) {
			os << std::any_cast<dls::chaine>(enf->valeur_calculee);
		}
		else if ((enf->drapeaux & CONVERTI_EINI) != 0) {
			os << std::any_cast<dls::chaine>(enf->valeur_calculee);
		}
		else if ((enf->drapeaux & EXTRAIT_CHAINE_C) != 0) {
			os << std::any_cast<dls::chaine>(enf->valeur_calculee);
		}
		else if ((enf->drapeaux & CONVERTI_TABLEAU_OCTET) != 0) {
			os << std::any_cast<dls::chaine>(enf->valeur_calculee);
		}
		else if ((enf->drapeaux & PREND_REFERENCE) != 0) {
			os << "&";

			/* Petit hack pour pouvoir passer des non-références à des
			 * paramètres de type références : les références ont pour type le
			 * type déréférencé, et sont automatiquement 'déréférencer' via (*x),
			 * donc passer des références à des références n'est pas un problème
			 * même si le code C généré ici devien '&(*x)'.
			 * Mais lorsque nous devons passer une non-référence à une référence
			 * le code devient '&&x' si le drapeaux PREND_REFERENCE est actif
			 * donc on le désactive pour ne générer que '&x'.
			 */

			enf->drapeaux &= ~PREND_REFERENCE;

			/* Pour les références des accès membres, on ne doit pas avoir de
			 * prépasse, donc expr_gauche = true. */
			genere_code_C(enf, contexte, true, os, os);

			enf->drapeaux |= PREND_REFERENCE;
		}
		else {
			genere_code_C(enf, contexte, false, os, os);
		}

		virgule = ',';
	}

	for (auto n : liste_var_retour) {
		os << virgule;

		os << "&(";
		genere_code_C(n, contexte, false, os, os);
		os << ')';

		virgule = ',';
	}

	for (auto n : liste_noms_retour) {
		os << virgule << n;
		virgule = ',';
	}

	os << ");";
}

static void declare_structures_C(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	contexte.magasin_types.declare_structures_C(contexte, os);

	/* À FAIRE : optimise. */
	for (auto is = 0l; is < contexte.structures.taille(); ++is) {
		auto const &donnees = contexte.donnees_structure(is);

		if (donnees.est_enum || donnees.est_externe) {
			continue;
		}

		auto const &nom_struct = broye_nom_simple(contexte.nom_struct(is));
		os << "typedef struct " << nom_struct << "{\n";

		for (auto i = 0l; i < donnees.donnees_types.taille(); ++i) {
			auto index_dt = donnees.donnees_types[i];
			auto &dt = contexte.magasin_types.donnees_types[index_dt];

			for (auto paire_idx_mb : donnees.donnees_membres) {
				if (paire_idx_mb.second.index_membre == i) {
					auto nom = broye_nom_simple(paire_idx_mb.first);

					contexte.magasin_types.converti_type_C(
								contexte,
								nom,
								dt,
								os,
								false,
								true);

					os << ";\n";
					break;
				}
			}
		}

		os << "} " << nom_struct << ";\n\n";
	}
}

static auto genere_code_acces_membre(
		base *structure,
		base *membre,
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	auto const &index_type = structure->index_type;
	auto type_structure = contexte.magasin_types.donnees_types[index_type];
	auto est_pointeur = type_structure.type_base() == id_morceau::POINTEUR;

	/* vérifie si nous avons une énumération */
	if (contexte.structure_existe(structure->chaine())) {
		auto &ds = contexte.donnees_structure(structure->chaine());

		if (ds.est_enum) {
			os << broye_chaine(structure) << '_' << broye_chaine(membre);
			return;
		}
	}

	if (est_type_tableau_fixe(type_structure)) {
		auto taille_tableau = static_cast<size_t>(type_structure.type_base() >> 8);
		os << taille_tableau;
		return;
	}

	genere_code_C(structure, contexte, true, os, os);
	os << ((est_pointeur) ? "->" : ".");
	os << broye_chaine(membre);
}

static void cree_initialisation(
		ContexteGenerationCode &contexte,
		DonneesType const &dt_parent,
		dls::chaine const &chaine_parent,
		dls::vue_chaine const &accesseur,
		dls::flux_chaine &os)
{
	if (dt_parent.type_base() == id_morceau::CHAINE || dt_parent.type_base() == id_morceau::TABLEAU) {
		os << chaine_parent << accesseur << "pointeur = 0;\n";
		os << chaine_parent << accesseur << "taille = 0;\n";
	}
	else if ((dt_parent.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
		auto const index_structure = static_cast<long>(dt_parent.type_base() >> 8);
		auto const &ds = contexte.donnees_structure(index_structure);

		for (auto i = 0l; i < ds.donnees_types.taille(); ++i) {
			auto index_dt = ds.donnees_types[i];
			auto &dt_enf = contexte.magasin_types.donnees_types[index_dt];

			for (auto paire_idx_mb : ds.donnees_membres) {
				auto const &donnees_membre = paire_idx_mb.second;

				if (donnees_membre.index_membre != i) {
					continue;
				}

				auto nom = paire_idx_mb.first;
				auto decl_nom = chaine_parent + dls::chaine(accesseur) + broye_nom_simple(nom);

				if (donnees_membre.noeud_decl != nullptr) {
					/* indirection pour les chaines ou autres */
					genere_code_C_prepasse(donnees_membre.noeud_decl, contexte, false, os);
					os << decl_nom << " = ";
					genere_code_C(donnees_membre.noeud_decl, contexte, false, os, os);
					os << ";\n";
				}
				else {
					cree_initialisation(
								contexte,
								dt_enf,
								decl_nom,
								".",
								os);
				}
			}
		}
	}
	else if ((dt_parent.type_base() & 0xff) == id_morceau::TABLEAU) {
		/* À FAIRE */
	}
	else {
		os << chaine_parent << " = 0;\n";
	}
}

static void prepasse_acces_membre(
		ContexteGenerationCode &contexte,
		base *b,
		base *structure,
		base *membre,
		dls::flux_chaine &os)
{
	if (dls::outils::possede_drapeau(b->drapeaux, PREND_REFERENCE)) {
		return;
	}

	if (b->aide_generation_code == APPEL_FONCTION_SYNT_UNI) {
		cree_appel(membre, os, contexte, membre->nom_fonction_appel, membre->enfants);
		b->valeur_calculee = membre->valeur_calculee;
		return;
	}

	auto const &index_type = structure->index_type;
	auto type_structure = contexte.magasin_types.donnees_types[index_type];

	auto est_pointeur = type_structure.type_base() == id_morceau::POINTEUR;

	if (est_pointeur) {
		type_structure = type_structure.derefence();
	}

	auto nom_acces = "__acces" + dls::vers_chaine(b->morceau.ligne_pos);
	b->valeur_calculee = nom_acces;

	if ((type_structure.type_base() & 0xff) == id_morceau::TABLEAU) {
		auto taille = static_cast<size_t>(type_structure.type_base() >> 8);

		if (taille != 0) {
			os << "long " << nom_acces << " = " << taille << ";\n";
			return;
		}
	}

	if (est_type_tableau_fixe(type_structure)) {
		auto taille_tableau = static_cast<size_t>(type_structure.type_base() >> 8);
		os << "long " << nom_acces << " = " << taille_tableau << ";\n";
		return;
	}

	/* vérifie si nous avons une énumération */
	if (contexte.structure_existe(structure->chaine())) {
		auto &ds = contexte.donnees_structure(structure->chaine());

		if (ds.est_enum) {
			os << "long " << nom_acces << " = "
			   << broye_chaine(structure) << '_' << broye_chaine(membre) << ";\n";
			return;
		}
	}

	dls::flux_chaine ss;

	genere_code_C(structure, contexte, true, ss, ss);
	ss << ((est_pointeur) ? "->" : ".");
	ss << broye_chaine(membre);

	/* le membre peut-être un pointeur de fonction, donc fais une
	 * prépasse pour générer cette appel */
	if (membre->type == type_noeud::APPEL_FONCTION) {
		membre->nom_fonction_appel = ss.chn();
		genere_code_C_prepasse(membre, contexte, false, os);

		auto &dt = contexte.magasin_types.donnees_types[b->index_type];
		contexte.magasin_types.converti_type_C(
					contexte,
					nom_acces,
					dt,
					os,
					false,
					false,
					true);
		os << " = "
		   << std::any_cast<dls::chaine>(membre->valeur_calculee) << ";\n";
	}
	else {
		auto &dt = contexte.magasin_types.donnees_types[b->index_type];
		contexte.magasin_types.converti_type_C(
					contexte,
					nom_acces,
					dt,
					os,
					false,
					false,
					true);
		os << " = " << ss.chn() << ";\n";
	}
}

/* La prépasse nous permet de générer du code avant celui des expressions afin
 * d'éviter les problèmes dans les cas où par exemple le paramètre d'une
 * fonction ou une condition ou une réassignation d'une chaine requiers une
 * déclaration temporaire.
 *
 * Par exemple, sans prépasse ce code ne serait pas converti en C correctement :
 *
 * si !fichier_existe("/chemin/vers/fichier") { ... }
 *
 * il donnerait :
 *
 * if (!fichier_existe({.pointeur="/chemin/vers/fichier", .taille=20 })) { ... }
 *
 * avec prépasse :
 *
 * chaine __chaine_tmp134831354 = {.pointeur="/chemin/vers/fichier", .taille=20 };
 * bool __ret_fichier_existe04554573 = fichier_existe(__chaine_tmp134831354);
 * if (!__ret_fichier_existe04554573) { ... }
 *
 * Il y a plus de variables temporaires, mais le code est correct, et le
 * compileur C supprimera ces temporaires de toute façon.
 */
static void genere_code_C_prepasse(
		base *b,
		ContexteGenerationCode &contexte,
		bool /*expr_gauche*/,
		dls::flux_chaine &os)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			assert(false);
			break;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			assert(false);
			break;
		}
		case type_noeud::APPEL_FONCTION:
		{
			cree_appel(b, os, contexte, b->nom_fonction_appel, b->enfants);
			break;
		}
		case type_noeud::VARIABLE:
		{
			// À FAIRE
			break;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			auto structure = b->enfants.front();
			auto membre = b->enfants.back();

			prepasse_acces_membre(contexte, b, structure, membre, os);

			break;
		}
		case type_noeud::ACCES_MEMBRE_DE:
		{
			auto structure = b->enfants.back();
			auto membre = b->enfants.front();

			prepasse_acces_membre(contexte, b, structure, membre, os);

			break;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			/* a, b = foo(); -> foo(&a, &b); */
			if (variable->identifiant() == id_morceau::VIRGULE) {
				dls::tableau<base *> feuilles;
				rassemble_feuilles(variable, feuilles);

				dls::liste<base *> noeuds;

				for (auto f : feuilles) {
					f->drapeaux |= POUR_ASSIGNATION;

					/* déclare au besoin */
					if (f->aide_generation_code == GENERE_CODE_DECL_VAR) {
						genere_code_C(f, contexte, true, os, os);
						os << ';' << '\n';
					}

					f->drapeaux &= ~POUR_ASSIGNATION;
					f->aide_generation_code = 0;
					noeuds.pousse(f);
				}

				expression->aide_generation_code = APPEL_FONCTION_MOULT_RET;
				expression->valeur_calculee = noeuds;

				genere_code_C_prepasse(expression,
									   contexte,
									   true,
									   os);
				return;
			}

			genere_code_C_prepasse(expression,
								   contexte,
								   true,
								   os);

			break;
		}
		case type_noeud::NOMBRE_REEL:
		{
			// À FAIRE
			break;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			// À FAIRE
			break;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			for (auto enfant : b->enfants) {
				genere_code_C_prepasse(enfant,
									   contexte,
									   true,
									   os);
			}

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			genere_code_C_prepasse(b->enfants.front(),
								   contexte,
								   true,
								   os);
			break;
		}
		case type_noeud::RETIENS:
		case type_noeud::RETOUR:
		{
			break;
		}
		case type_noeud::CHAINE_LITTERALE:
		{
			/* Note : dû à la possibilité de différer le code, nous devons
			 * utiliser la chaine originale. */
			auto chaine = b->morceau.chaine;

			auto nom_chaine = "__chaine_tmp" + dls::vers_chaine(b->morceau.ligne_pos);

			os << "chaine " << nom_chaine << " = {.pointeur=";
			os << '"';

			for (auto c : chaine) {
				if (c == '\n') {
					os << '\\' << 'n';
				}
				else if (c == '\t') {
					os << '\\' << 't';
				}
				else {
					os << c;
				}
			}

			os << '"';
			os << ", .taille=" << chaine.taille() << "};\n";
			b->valeur_calculee = nom_chaine;
			break;
		}
		case type_noeud::BOOLEEN:
		{
			break;
		}
		case type_noeud::CARACTERE:
		{
			break;
		}
		case type_noeud::BLOC:
		{
			assert(false);
			break;
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		case type_noeud::POUR:
		case type_noeud::CONTINUE_ARRETE:
		case type_noeud::BOUCLE:
		case type_noeud::TANTQUE:
		case type_noeud::TRANSTYPE:
		{
			break;
		}
		case type_noeud::NUL:
		{
			// À FAIRE
			break;
		}
		case type_noeud::TAILLE_DE:
		{
			auto index_dt = std::any_cast<long>(b->valeur_calculee);
			auto const &donnees = contexte.magasin_types.donnees_types[index_dt];
			os << "sizeof(";
			contexte.magasin_types.converti_type_C(contexte, "", donnees, os);
			os << ")";
			break;
		}
		case type_noeud::PLAGE:
		{
			assert(false);
			break;
		}
		case type_noeud::DIFFERE:
		{
			break;
		}
		case type_noeud::NONSUR:
		{
			break;
		}
		case type_noeud::TABLEAU:
		{
			/* utilisé principalement pour convertir les listes d'arguments
			 * variadics en un tableau */

			auto taille_tableau = b->enfants.taille();

			auto &type = contexte.magasin_types.donnees_types[b->index_type];

			/* cherche si une conversion est requise */
			for (auto enfant : b->enfants) {
				if ((enfant->drapeaux & CONVERTI_EINI) != 0) {
					enfant->valeur_calculee = cree_eini(contexte, os, enfant);
				}
			}

			/* alloue un tableau fixe */
			auto dt_tfixe = DonneesType{};
			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(taille_tableau << 8));
			dt_tfixe.pousse(type);

			auto nom_tableau_fixe = dls::chaine("__tabl_fix")
					.append(dls::vers_chaine(b->morceau.ligne_pos >> 32));

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_fixe, dt_tfixe, os);

			os << " = ";

			auto virgule = '{';

			for (auto enfant : b->enfants) {
				os << virgule;
				if ((enfant->drapeaux & CONVERTI_EINI) != 0) {
					os << std::any_cast<dls::chaine>(enfant->valeur_calculee);
				}
				else {
					genere_code_C(enfant, contexte, false, os, os);
				}

				virgule = ',';
			}

			os << "};\n";

			/* alloue un tableau dynamique */
			auto dt_tdyn = DonneesType{};
			dt_tdyn.pousse(id_morceau::TABLEAU);
			dt_tdyn.pousse(type);

			auto nom_tableau_dyn = dls::chaine("__tabl_dyn")
					.append(dls::vers_chaine(b->morceau.ligne_pos));

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_dyn, dt_tdyn, os);

			os << ";\n";
			os << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			os << nom_tableau_dyn << ".taille = " << taille_tableau << ";\n";

			b->valeur_calculee = nom_tableau_dyn;
			break;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			break;
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			auto nom_tableau = "__tabl" + dls::vers_chaine(b->morceau.ligne_pos);
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];

			contexte.magasin_types.converti_type_C(
						contexte,
						nom_tableau,
						dt,
						os);

			os << " = ";

			dls::tableau<base *> feuilles;
			rassemble_feuilles(b->enfants.front(), feuilles);

			auto virgule = '{';

			for (auto f : feuilles) {
				os << virgule;
				genere_code_C(f, contexte, false, os, os);
				virgule = ',';
			}

			os << "};\n";

			b->valeur_calculee = nom_tableau;

			break;
		}
		case type_noeud::INFO_DE:
		{
			// À FAIRE
			break;
		}
		case type_noeud::MEMOIRE:
		{
			break;
		}
		case type_noeud::LOGE:
		{
			// À FAIRE
			break;
		}
		case type_noeud::DELOGE:
		{
			break;
		}
		case type_noeud::RELOGE:
		{
			// À FAIRE
			break;
		}
		case type_noeud::DECLARATION_STRUCTURE:
		{
			assert(false);
			break;
		}
		case type_noeud::DECLARATION_ENUM:
		{
			assert(false);
			break;
		}
		case type_noeud::ASSOCIE:
		{
			break;
		}
		case type_noeud::PAIRE_ASSOCIATION:
		{
			/* RAF : pris en charge dans type_noeud::ASSOCIE, ce noeud n'est que
			 * pour ajouter un niveau d'indirection et faciliter la compilation
			 * des associations. */
			assert(false);
			break;
		}
	}
}

void genere_code_C(
		base *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche,
		dls::flux_chaine &os,
		dls::flux_chaine &os_init)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			auto temps_validation = 0.0;
			auto temps_generation = 0.0;

			for (auto noeud : b->enfants) {
				auto debut_validation = dls::chrono::maintenant();
				performe_validation_semantique(noeud, contexte);
				contexte.magasin_chaines.efface();
				temps_validation += dls::chrono::delta(debut_validation);
			}

			declare_structures_C(contexte, os);

			auto debut_generation = dls::chrono::maintenant();
			/* Crée les infos types pour tous les types connus.
			 * À FAIRE : évite de créer ceux qui ne sont pas utiles */
			for (auto &dt : contexte.magasin_types.donnees_types) {
				if (dt.type_base() != id_morceau::TYPE_DE) {
					cree_info_type_C(contexte, os, os_init, dt);
				}
			}
			temps_generation += dls::chrono::delta(debut_generation);

			/* génère le code */
			for (auto noeud : b->enfants) {
				debut_generation = dls::chrono::maintenant();
				genere_code_C(noeud, contexte, false, os, os);
				contexte.magasin_chaines.efface();
				temps_generation += dls::chrono::delta(debut_generation);
			}

			contexte.temps_generation = temps_generation;
			contexte.temps_validation = temps_validation;

			break;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			auto module = contexte.module(static_cast<size_t>(b->morceau.module));
			auto &vdf = module->donnees_fonction(b->morceau.chaine);
			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);

			for (auto &df : vdf) {
				if (df.noeud_decl == b) {
					donnees_fonction = &df;
					break;
				}
			}

			if (!donnees_fonction->est_utilisee) {
				return;
			}

			/* Pour les fonctions variadiques nous transformons la liste d'argument en
			 * un tableau dynamique transmis à la fonction. La raison étant que les
			 * instruction de LLVM pour les arguments variadiques ne fonctionnent
			 * vraiment que pour les types simples et les pointeurs. Pour pouvoir passer
			 * des structures, il faudrait manuellement gérer les instructions
			 * d'incrémentation et d'extraction des arguments pour chaque plateforme.
			 * Nos tableaux, quant à eux, sont portables.
			 */

			auto const est_externe = dls::outils::possede_drapeau(b->drapeaux, EST_EXTERNE);

			if (est_externe) {
				return;
			}

			/* Crée fonction */
			auto nom_fonction = donnees_fonction->nom_broye;

			auto moult_retour = donnees_fonction->idx_types_retours.taille() > 1;

			if (donnees_fonction->est_coroutine) {
				os << "typedef struct __etat_coro" << nom_fonction << " { bool __reprend_coro; bool __termine_coro; ";

				auto idx_ret = 0l;
				for (auto idx : donnees_fonction->idx_types_retours) {
					auto &nom_ret = donnees_fonction->noms_retours[idx_ret++];

					auto &dt = contexte.magasin_types.donnees_types[idx];
					contexte.magasin_types.converti_type_C(
								contexte,
								nom_ret,
								dt,
								os);

					os << ";\n";
				}

				auto &donnees_coroutine = donnees_fonction->donnees_coroutine;

				for (auto const &paire : donnees_coroutine.variables) {
					contexte.magasin_types.converti_type_C(contexte,
												"",
												contexte.magasin_types.donnees_types[paire.second.first],
											os);

					/* Stocke un pointeur pour ne pas qu'il soit invalidé par
					 * le retour de la coroutine. */
					if ((paire.second.second & BESOIN_DEREF) != 0) {
						os << '*';
					}

					os << ' ' << broye_nom_simple(paire.first) << ";\n";
				}

				os << " } __etat_coro" << nom_fonction << ";\n";

				os << "static ";

				if (!dls::outils::possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
					os << "inline ";
				}

				os << "void " << nom_fonction;
			}
			else if (moult_retour) {
				if (dls::outils::possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
					os << "static inline void ";
				}
				else if (dls::outils::possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
					os << "static void __attribute__ ((noinline)) ";
				}
				else {
					os << "static void ";
				}

				os << nom_fonction;
			}
			else {
				if (dls::outils::possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
					os << "static inline ";
				}
				else if (dls::outils::possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
					os << "__attribute__ ((noinline)) ";
				}

				contexte.magasin_types.converti_type_C(contexte,
							nom_fonction,
							contexte.magasin_types.donnees_types[b->index_type],
						os);
			}

			contexte.commence_fonction(donnees_fonction);

			/* Crée code pour les arguments */

			auto virgule = '(';

			if (donnees_fonction->nom_args.taille() == 0 && !moult_retour) {
				os << '(';
				virgule = ' ';
			}

			if (donnees_fonction->est_coroutine) {
				os << virgule;
				os << "__etat_coro" << nom_fonction << " *__etat";
				virgule = ',';
			}

			for (auto const &nom : donnees_fonction->nom_args) {
				os << virgule;

				auto &argument = donnees_fonction->args[nom];
				auto index_type = argument.donnees_type;

				auto dt = DonneesType{};

				if (argument.est_variadic) {
					auto &dt_var = contexte.magasin_types.donnees_types[index_type];

					dt.pousse(id_morceau::TABLEAU);
					dt.pousse(dt_var.derefence());

					index_type = argument.donnees_type;
					contexte.magasin_types.ajoute_type(dt);
				}
				else {
					dt = contexte.magasin_types.donnees_types[index_type];
				}

				auto nom_broye = broye_nom_simple(nom);

				contexte.magasin_types.converti_type_C(
							contexte,
							nom_broye,
							dt,
							os);

				virgule = ',';

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = argument.est_dynamic;
				donnees_var.est_variadic = argument.est_variadic;
				donnees_var.donnees_type = index_type;
				donnees_var.est_argument = true;

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
					dt = dt.derefence();
					donnees_var.donnees_type = contexte.magasin_types.ajoute_type(dt);
				}

				contexte.pousse_locale(nom, donnees_var);

				if (argument.est_employe) {
					auto &dt_var = contexte.magasin_types.donnees_types[argument.donnees_type];
					auto id_structure = 0l;
					auto est_pointeur = false;

					if (dt_var.type_base() == id_morceau::POINTEUR) {
						est_pointeur = true;
						id_structure = static_cast<long>(dt_var.derefence().type_base() >> 8);
					}
					else {
						id_structure = static_cast<long>(dt_var.type_base() >> 8);
					}

					auto &ds = contexte.donnees_structure(id_structure);

					/* pousse chaque membre de la structure sur la pile */

					for (auto &dm : ds.donnees_membres) {
						auto index_dt_m = ds.donnees_types[dm.second.index_membre];

						donnees_var.est_dynamique = argument.est_dynamic;
						donnees_var.donnees_type = index_dt_m;
						donnees_var.est_argument = true;
						donnees_var.est_membre_emploie = true;
						donnees_var.structure = broye_nom_simple(nom) + (est_pointeur ? "->" : ".");

						contexte.pousse_locale(dm.first, donnees_var);
					}
				}
			}

			if (moult_retour && !donnees_fonction->est_coroutine) {
				auto idx_ret = 0l;
				for (auto idx : donnees_fonction->idx_types_retours) {
					os << virgule;

					auto nom_ret = "*" + donnees_fonction->noms_retours[idx_ret++];

					auto &dt = contexte.magasin_types.donnees_types[idx];
					contexte.magasin_types.converti_type_C(
								contexte,
								nom_ret,
								dt,
								os);

					virgule = ',';
				}
			}

			os << ")\n";

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.front();
			os << "{\n";

			if (donnees_fonction->est_coroutine) {
				for (auto i = 1; i <= donnees_fonction->donnees_coroutine.nombre_retenues; ++i) {
					os << "if (__etat->__reprend_coro == " << i << ") { goto __reprend_coro" << i << "; }";
				}

				/* remet à zéro car nous avons besoin de les compter pour
				 * générer les labels des goto */
				donnees_fonction->donnees_coroutine.nombre_retenues = 0;
			}

			genere_code_C(bloc, contexte, false, os, os);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(contexte, os);
			}

			os << "}\n";

			contexte.termine_fonction();

			break;
		}
		case type_noeud::APPEL_FONCTION:
		{
			os << std::any_cast<dls::chaine>(b->valeur_calculee);
			break;
		}
		case type_noeud::VARIABLE:
		{
			auto drapeaux = contexte.drapeaux_variable(b->morceau.chaine);

			if (b->aide_generation_code == GENERE_CODE_DECL_VAR) {
				auto dt = contexte.magasin_types.donnees_types[b->index_type];

				/* pour les assignations de tableaux fixes, remplace les crochets
				 * par des pointeurs pour la déclaration */
				if (dls::outils::possede_drapeau(b->drapeaux, POUR_ASSIGNATION)) {
					if (dt.type_base() != id_morceau::TABLEAU && (dt.type_base() & 0xff) == id_morceau::TABLEAU) {
						auto ndt = DonneesType{};
						ndt.pousse(id_morceau::POINTEUR);
						ndt.pousse(dt.derefence());

						dt = ndt;
					}
				}

				auto nom_broye = broye_chaine(b);

				contexte.magasin_types.converti_type_C(
							contexte,
							nom_broye,
							dt,
							os);

				if (contexte.donnees_fonction == nullptr) {
					auto donnees_var = DonneesVariable{};
					donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
					donnees_var.donnees_type = b->index_type;

					if (dt.type_base() == id_morceau::REFERENCE) {
						donnees_var.drapeaux |= BESOIN_DEREF;
					}

					contexte.pousse_globale(b->chaine(), donnees_var);
					return;
				}

				/* nous avons une déclaration, initialise à zéro */
				if (!dls::outils::possede_drapeau(b->drapeaux, POUR_ASSIGNATION)) {
					os << ";\n";
					cree_initialisation(
								contexte,
								dt,
								nom_broye,
								".",
								os);
				}

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
				donnees_var.donnees_type = b->index_type;

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
				}

				contexte.pousse_locale(b->chaine(), donnees_var);
			}
			/* désactive la vérification car les variables dans les
			 * constructions de structures n'ont pas cette aide */
			else /*if (b->aide_generation_code == GENERE_CODE_ACCES_VAR)*/ {
				if ((drapeaux & BESOIN_DEREF) != 0) {
					os << "(*" << broye_chaine(b) << ")";
				}
				else {
					if (b->nom_fonction_appel != "") {
						os << b->nom_fonction_appel;
					}
					else {
						auto dv = contexte.donnees_variable(b->morceau.chaine);

						if (dv.est_membre_emploie) {
							os << dv.structure;
						}

						if ((b->drapeaux & PREND_REFERENCE) != 0) {
							os << '&';
						}

						os << broye_chaine(b);
					}
				}
			}

			break;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			if (expr_gauche == false || b->aide_generation_code == APPEL_FONCTION_SYNT_UNI) {
				os << std::any_cast<dls::chaine>(b->valeur_calculee);
			}
			else {
				auto structure = b->enfants.front();
				auto membre = b->enfants.back();
				genere_code_acces_membre(structure, membre, contexte, os);
			}

			break;
		}
		case type_noeud::ACCES_MEMBRE_DE:
		{
			if (expr_gauche == false) {
				os << std::any_cast<dls::chaine>(b->valeur_calculee);
			}
			else {
				auto structure = b->enfants.back();
				auto membre = b->enfants.front();
				genere_code_acces_membre(structure, membre, contexte, os);
			}

			break;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.taille() == 2);

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			/* a, b = foo(); -> foo(&a, &b); */
			if (variable->identifiant() == id_morceau::VIRGULE) {
				/* fais dans la prépasse */
				return;
			}

			auto compatibilite = std::any_cast<niveau_compat>(b->valeur_calculee);
			auto expression_modifiee = false;
			auto nouvelle_expr = dls::chaine();

			/* À FAIRE */
			if ((compatibilite & niveau_compat::converti_tableau) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_TABLEAU;
			}

			if ((compatibilite & niveau_compat::converti_eini) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_EINI;
				expression_modifiee = true;

				nouvelle_expr = cree_eini(contexte, os, expression);
			}

			if ((compatibilite & niveau_compat::extrait_eini) != niveau_compat::aucune) {
				expression->drapeaux |= EXTRAIT_EINI;
				expression->index_type = variable->index_type;
				expression_modifiee = true;

				auto nom_eini = dls::chaine("__eini_ext_")
						.append(dls::vers_chaine(expression->morceau.ligne_pos >> 32));

				auto &dt = contexte.magasin_types.donnees_types[expression->index_type];

				contexte.magasin_types.converti_type_C(contexte, "", dt, os);

				os << " " << nom_eini << " = *(";

				contexte.magasin_types.converti_type_C(contexte, "", dt, os);

				os << " *)(";
				genere_code_C(expression, contexte, false, os, os);
				os << ".pointeur);\n";

				nouvelle_expr = nom_eini;
			}

			if (expression->type == type_noeud::LOGE) {
				expression_modifiee = true;
				genere_code_C(expression, contexte, false, os, os);
				nouvelle_expr = std::any_cast<dls::chaine>(expression->valeur_calculee);
			}

			variable->drapeaux |= POUR_ASSIGNATION;
			genere_code_C(variable, contexte, true, os, os);
			os << " = ";

			if (!expression_modifiee) {
				genere_code_C(expression, contexte, false, os, os);
			}
			else {
				os << nouvelle_expr;
			}

			/* pour les globales */
			os << ";\n";

			break;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_reel(
												 b->morceau.chaine,
												 b->morceau.identifiant);

			os << valeur;
			break;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<long>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_entier(
												 b->morceau.chaine,
												 b->morceau.identifiant);

			os << valeur;
			break;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			auto const index_type1 = enfant1->index_type;
			auto const &type1 = contexte.magasin_types.donnees_types[index_type1];

			/* À FAIRE : typage */

			/* À FAIRE : tests */
			auto est_operateur_comp = [](id_morceau id)
			{
				switch (id) {
					default:
					{
						return false;
					}
					case id_morceau::INFERIEUR:
					case id_morceau::INFERIEUR_EGAL:
					case id_morceau::SUPERIEUR:
					case id_morceau::SUPERIEUR_EGAL:
					case id_morceau::EGALITE:
					case id_morceau::DIFFERENCE:
					{
						return true;
					}
				}
			};

			switch (b->morceau.identifiant) {
				default:
				{
					/* vérifie (a comp b comp c), transforme ((a comp b) && (b comp c)) */
					if (est_operateur_comp(b->morceau.identifiant)) {
						if (enfant1->morceau.identifiant == b->morceau.identifiant) {
							/* (a comp b) */
							os << '(';
							genere_code_C(enfant1, contexte, expr_gauche, os, os);
							os << ')';

							os << "&&";

							/* (b comp c) */
							os << '(';
							genere_code_C(enfant1->enfants.back(), contexte, expr_gauche, os, os);
							os << b->morceau.chaine;
							genere_code_C(enfant2, contexte, expr_gauche, os, os);
							os << ')';
						}
						else {
							os << '(';
							genere_code_C(enfant1, contexte, expr_gauche, os, os);
							os << b->morceau.chaine;
							genere_code_C(enfant2, contexte, expr_gauche, os, os);
							os << ')';
						}
					}
					else {
						/* À FAIRE : prépasse pour les accès membre. */
						os << '(';
						genere_code_C(enfant1, contexte, true, os, os);
						os << b->morceau.chaine;
						genere_code_C(enfant2, contexte, true, os, os);
						os << ')';
					}

					break;
				}
				case id_morceau::CROCHET_OUVRANT:
				{
					auto type_base = type1.type_base();

					switch (type_base & 0xff) {
						case id_morceau::POINTEUR:
						{
							genere_code_C(enfant1, contexte, expr_gauche, os, os);
							os << '[';
							genere_code_C(enfant2, contexte, expr_gauche, os, os);
							os << ']';
							break;
						}
						case id_morceau::CHAINE:
						{
							genere_code_C(enfant1, contexte, expr_gauche, os, os);
							os << ".pointeur";
							os << '[';
							genere_code_C(enfant2, contexte, expr_gauche, os, os);
							os << ']';
							break;
						}
						case id_morceau::TABLEAU:
						{
							auto taille_tableau = static_cast<int>(type_base >> 8);

							genere_code_C(enfant1, contexte, expr_gauche, os, os);

							if (taille_tableau == 0) {
								os << ".pointeur";
							}

							os << '[';
							genere_code_C(enfant2, contexte, expr_gauche, os, os);
							os << ']';
							break;
						}
						default:
						{
							assert(false);
							break;
						}
					}

					break;
				}
			}

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto enfant = b->enfants.front();

			switch (b->morceau.identifiant) {
				case id_morceau::EXCLAMATION:
				{
					os << "!(";
					break;
				}
				case id_morceau::TILDE:
				{
					os << "~(";
					break;
				}
				case id_morceau::AROBASE:
				{
					os << "&(";
					break;
				}
				case id_morceau::PLUS_UNAIRE:
				{
					os << "(";
					break;
				}
				case id_morceau::MOINS_UNAIRE:
				{
					os << "-(";
					break;
				}
				default:
				{
					break;
				}
			}

			genere_code_C(enfant, contexte, expr_gauche, os, os);
			os << ")";

			break;
		}
		case type_noeud::RETOUR:
		{
			auto nom_variable = dls::chaine("");

			if (b->aide_generation_code == GENERE_CODE_RETOUR_MOULT) {
				auto enfant = b->enfants.front();
				auto df = contexte.donnees_fonction;

				if (enfant->type == type_noeud::APPEL_FONCTION) {
					/* retourne foo() -> foo(__ret...); return; */
					enfant->aide_generation_code = APPEL_FONCTION_MOULT_RET2;
					enfant->valeur_calculee = df->noms_retours;
					genere_code_C_prepasse(enfant, contexte, false, os);
				}
				else if (enfant->identifiant() == id_morceau::VIRGULE) {
					/* retourne a, b; -> *__ret1 = a; *__ret2 = b; return; */
					dls::tableau<base *> feuilles;
					rassemble_feuilles(enfant, feuilles);

					auto idx = 0l;
					for (auto f : feuilles) {
						genere_code_C_prepasse(f, contexte, false, os);

						os << '*' << df->noms_retours[idx++] << " = ";
						genere_code_C(f, contexte, false, os, os);
						os << ';';
					}
				}
			}
			else if (b->aide_generation_code == GENERE_CODE_RETOUR_SIMPLE) {
				auto enfant = b->enfants.front();
				auto df = contexte.donnees_fonction;

				nom_variable = df->noms_retours[0];

				genere_code_C_prepasse(enfant, contexte, false, os);

				auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];

				contexte.magasin_types.converti_type_C(
							contexte,
							nom_variable,
							dt,
							os);

				os << " = ";

				genere_code_C(enfant, contexte, false, os, os);

				os << ";\n";
			}

			/* NOTE : le code différé doit être crée après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(contexte, os);

			os << "return " << nom_variable;

			break;
		}
		case type_noeud::CHAINE_LITTERALE:
		{
			os << std::any_cast<dls::chaine>(b->valeur_calculee);
			break;
		}
		case type_noeud::BOOLEEN:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
										   : (b->chaine() == "vrai");
			os << valeur;
			break;
		}
		case type_noeud::CARACTERE:
		{
			auto c = b->morceau.chaine[0];

			os << '\'';
			if (c == '\\') {
				os << c << b->morceau.chaine[1];
			}
			else {
				os << c;
			}

			os << '\'';
			break;
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();

			/* noeud 1 : condition */
			auto enfant1 = *iter_enfant++;

			genere_code_C_prepasse(enfant1, contexte, true, os);

			os << "if (";

			if (b->type == type_noeud::SAUFSI) {
				os << '!';
			}

			genere_code_C(enfant1, contexte, false, os, os);
			os << ") {\n";

			/* noeud 2 : bloc */
			auto enfant2 = *iter_enfant++;
			genere_code_C(enfant2, contexte, false, os, os);
			os << "}\n";

			/* noeud 3 : sinon (optionel) */
			if (nombre_enfants == 3) {
				os << "else {\n";
				auto enfant3 = *iter_enfant++;
				genere_code_C(enfant3, contexte, false, os, os);
				os << "}\n";
			}

			break;
		}
		case type_noeud::BLOC:
		{
			contexte.empile_nombre_locales();

			auto dernier_enfant = static_cast<base *>(nullptr);

			for (auto enfant : b->enfants) {
				genere_code_C_prepasse(enfant, contexte, true, os);
				genere_code_C(enfant, contexte, true, os, os);
				os << ";\n";

				dernier_enfant = enfant;

				if (enfant->type == type_noeud::RETOUR) {
					break;
				}
			}

			if (dernier_enfant != nullptr && dernier_enfant->type != type_noeud::RETOUR) {
				/* génère le code pour tous les noeuds différés de ce bloc */
				auto noeuds = contexte.noeuds_differes_bloc();

				while (!noeuds.est_vide()) {
					auto n = noeuds.back();
					genere_code_C(n, contexte, false, os, os);
					noeuds.pop_back();
				}
			}

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::POUR:
		{
			auto nombre_enfants = b->enfants.taille();
			auto iter = b->enfants.debut();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;

			auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

			auto enfant_sans_arret = enfant4;
			auto enfant_sinon = (nombre_enfants == 5) ? enfant5 : enfant4;

			auto index_type = enfant2->index_type;
			auto const &type_debut = contexte.magasin_types.donnees_types[index_type];
			auto const type = type_debut.type_base();

			enfant1->index_type = index_type;

			contexte.empile_nombre_locales();

			auto genere_code_tableau_chaine = [](
					dls::flux_chaine &os_loc,
					ContexteGenerationCode &contexte_loc,
					base *enfant_1,
					base *enfant_2,
					DonneesType const &dt,
					dls::chaine const &nom_var)
			{
				auto var = enfant_1;
				auto idx = static_cast<noeud::base *>(nullptr);

				if (enfant_1->morceau.identifiant == id_morceau::VIRGULE) {
					var = enfant_1->enfants.front();
					idx = enfant_1->enfants.back();
				}

				genere_code_C_prepasse(enfant_2, contexte_loc, false, os_loc);

				os_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= ";
				genere_code_C(enfant_2, contexte_loc, false, os_loc, os_loc);
				os_loc << ".taille - 1; ++"<< nom_var <<") {\n";
				contexte_loc.magasin_types.converti_type_C(contexte_loc, "", dt, os_loc);
				os_loc << " *" << broye_chaine(var) << " = &";
				genere_code_C(enfant_2, contexte_loc, false, os_loc, os_loc);
				os_loc << ".pointeur["<< nom_var <<"];\n";

				if (idx) {
					os_loc << "int " << broye_chaine(idx) << " = " << nom_var << ";\n";
				}
			};

			auto genere_code_tableau_fixe = [](
					dls::flux_chaine &os_loc,
					ContexteGenerationCode &contexte_loc,
					base *enfant_1,
					base *enfant_2,
					DonneesType const &dt,
					dls::chaine const &nom_var,
					uint64_t taille_tableau)
			{
				auto var = enfant_1;
				auto idx = static_cast<noeud::base *>(nullptr);

				if (enfant_1->morceau.identifiant == id_morceau::VIRGULE) {
					var = enfant_1->enfants.front();
					idx = enfant_1->enfants.back();
				}

				os_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= "
				   << taille_tableau << "-1; ++"<< nom_var <<") {\n";
				contexte_loc.magasin_types.converti_type_C(contexte_loc, "", dt, os_loc);
				os_loc << " *" << broye_chaine(var) << " = &";
				genere_code_C(enfant_2, contexte_loc, false, os_loc, os_loc);
				os_loc << "["<< nom_var <<"];\n";

				if (idx) {
					os_loc << "int " << broye_chaine(idx) << " = " << nom_var << ";\n";
				}
			};

			switch (b->aide_generation_code) {
				case GENERE_BOUCLE_PLAGE:
				case GENERE_BOUCLE_PLAGE_INDEX:
				{
					auto var = enfant1;
					auto idx = static_cast<noeud::base *>(nullptr);

					if (enfant1->morceau.identifiant == id_morceau::VIRGULE) {
						var = enfant1->enfants.front();
						idx = enfant1->enfants.back();
					}

					auto nom_broye = broye_chaine(var);

					if (idx != nullptr) {
						os << "int " << broye_chaine(idx) << " = 0;";
					}

					os << "\nfor (";
					contexte.magasin_types.converti_type_C(
								contexte,
								"",
								type_debut,
								os);
					os << " " << nom_broye << " = ";
					genere_code_C(enfant2->enfants.front(), contexte, false, os, os);

					os << "; "
					   << nom_broye << " <= ";

					genere_code_C(enfant2->enfants.back(), contexte, false, os, os);
					os <<"; ++" << nom_broye;

					if (idx != nullptr) {
						os << ", ++" << broye_chaine(idx);
					}

					os  << ") {\n";

					if (b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
						auto donnees_var = DonneesVariable{};

						donnees_var.donnees_type = var->index_type;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.donnees_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						auto donnees_var = DonneesVariable{};
						donnees_var.donnees_type = index_type;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					break;
				}
				case GENERE_BOUCLE_TABLEAU:
				case GENERE_BOUCLE_TABLEAU_INDEX:
				{
					auto nom_var = "__i" + dls::vers_chaine(b->morceau.ligne_pos);
					contexte.magasin_chaines.pousse(nom_var);

					auto donnees_var = DonneesVariable{};
					donnees_var.donnees_type = contexte.magasin_types[TYPE_Z32];

					contexte.pousse_locale(contexte.magasin_chaines.back(), donnees_var);

					if ((type & 0xff) == id_morceau::TABLEAU) {
						auto const taille_tableau = static_cast<uint64_t>(type >> 8);

						if (taille_tableau != 0) {
							genere_code_tableau_fixe(os, contexte, enfant1, enfant2, type_debut.derefence(), nom_var, taille_tableau);
						}
						else {
							genere_code_tableau_chaine(os, contexte, enfant1, enfant2, type_debut.derefence(), nom_var);
						}
					}
					else if (type == id_morceau::CHAINE) {
						auto dt = DonneesType(id_morceau::Z8);
						index_type = contexte.magasin_types[TYPE_Z8];
						genere_code_tableau_chaine(os, contexte, enfant1, enfant2, dt, nom_var);
					}

					if (b->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
						auto var = enfant1->enfants.front();
						auto idx = enfant1->enfants.back();

						donnees_var.donnees_type = var->index_type;
						donnees_var.drapeaux = BESOIN_DEREF;

						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.donnees_type = idx->index_type;
						donnees_var.drapeaux = 0;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						donnees_var.donnees_type = index_type;
						donnees_var.drapeaux = BESOIN_DEREF;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					auto nom_etat = "__etat" + dls::vers_chaine(enfant2->morceau.ligne_pos);

					os << "__etat_coro" << enfant2->df->nom_broye << " " << nom_etat << ";\n";
					os << nom_etat << ".__reprend_coro = 0;\n";
					os << nom_etat << ".__termine_coro = 0;\n";

					/* À FAIRE : utilisation du type */
					auto df = enfant2->df;
					auto nombre_vars_ret = df->idx_types_retours.taille();

					auto feuilles = dls::tableau<base *>{};
					rassemble_feuilles(enfant1, feuilles);

					auto idx = static_cast<noeud::base *>(nullptr);
					auto nom_idx = dls::chaine{};

					if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
						idx = feuilles.back();
						nom_idx = "__idx" + dls::vers_chaine(b->morceau.ligne_pos);
						os << "int " << nom_idx << " = 0;";
					}

					os << "while (1) {\n";
					genere_code_C_prepasse(enfant2, contexte, true, os);
					genere_code_C(enfant2, contexte, true, os, os);

					os << ";\n";

					os << "if (" << nom_etat << ".__termine_coro == 1) { break; }\n";

					for (auto i = 0l; i < nombre_vars_ret; ++i) {
						auto f = feuilles[i];
						auto nom_var_broye = broye_chaine(f);

						contexte.magasin_types.converti_type_C(
									contexte,
									nom_var_broye,
									type_debut,
									os);

						os << ';'
						   << nom_var_broye
						   << " = " << nom_etat
						   << '.' << df->noms_retours[i] << ";\n";

						auto donnees_var = DonneesVariable{};
						donnees_var.donnees_type = df->idx_types_retours[i];
						contexte.pousse_locale(f->chaine(), donnees_var);
					}

					if (idx) {
						os << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
						os << nom_idx << " += 1;";

						auto donnees_var = DonneesVariable{};
						donnees_var.donnees_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
				}
			}

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b->morceau.ligne_pos);
			auto goto_brise = "__boucle_pour_brise" + dls::vers_chaine(b->morceau.ligne_pos);

			contexte.empile_goto_continue(enfant1->chaine(), goto_continue);
			contexte.empile_goto_arrete(enfant1->chaine(), (enfant_sinon != nullptr) ? goto_brise : goto_apres);

			genere_code_C(enfant3, contexte, false, os, os);

			os << goto_continue << ":;\n";
			os << "}\n";

			if (enfant_sans_arret) {
				genere_code_C(enfant_sans_arret, contexte, false, os, os);
				os << "goto " << goto_apres << ";";
			}

			if (enfant_sinon) {
				os << goto_brise << ":;\n";
				genere_code_C(enfant_sinon, contexte, false, os, os);
			}

			os << goto_apres << ":;\n";

			contexte.depile_goto_arrete();
			contexte.depile_goto_continue();

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::CONTINUE_ARRETE:
		{
			auto chaine_var = b->enfants.est_vide() ? dls::vue_chaine{""} : b->enfants.front()->chaine();

			auto label_goto = (b->morceau.identifiant == id_morceau::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			os << "goto " << label_goto;
			break;
		}
		case type_noeud::BOUCLE:
		{
			/* boucle:
			 *	corps
			 *  br boucle
			 *
			 * apres_boucle:
			 *	...
			 */

			auto iter = b->enfants.debut();
			auto enfant1 = *iter++;

			/* création des blocs */

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b->morceau.ligne_pos);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			os << "while (1) {\n";

			genere_code_C(enfant1, contexte, false, os, os);

			os << goto_continue << ":;\n";
			os << "}\n";
			os << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case type_noeud::TANTQUE:
		{
			assert(b->enfants.taille() == 2);
			auto iter = b->enfants.debut();
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b->morceau.ligne_pos);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			/* NOTE : la prépasse est susceptible de générer le code d'un appel
			 * de fonction, donc nous utilisons
			 * while (1) { if (!cond) { break; } }
			 * au lieu de
			 * while (cond) {}
			 * pour être sûr que la fonction est appelée à chaque boucle.
			 */
			os << "while (1) {";
			genere_code_C_prepasse(enfant1, contexte, true, os);
			os << "if (!";
			genere_code_C(enfant1, contexte, false, os, os);
			os << ") { break; }\n";

			genere_code_C(enfant2, contexte, false, os, os);

			os << goto_continue << ":;\n";
			os << "}\n";
			os << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case type_noeud::TRANSTYPE:
		{
			auto enfant = b->enfants.front();
			auto const &index_type_de = enfant->index_type;

			if (index_type_de == b->index_type) {
				/* À FAIRE : prépasse pour les accès membres. */
				genere_code_C(enfant, contexte, true, os, os);
				return;
			}

			auto const &donnees_type_de = contexte.magasin_types.donnees_types[index_type_de];

			auto const &dt = contexte.magasin_types.donnees_types[b->index_type];

			auto type_de = donnees_type_de.type_base();
			auto type_vers = dt.type_base();

			/* À FAIRE : vérifie compatibilité */
			if (type_de == id_morceau::POINTEUR && type_vers == id_morceau::POINTEUR) {
				os << "(";
				contexte.magasin_types.converti_type_C(contexte, "", dt, os);
				os << ")(";
				genere_code_C(enfant, contexte, false, os, os);
				os << ")";
				return;
			}

			os << "(";
			contexte.magasin_types.converti_type_C(contexte, "", dt, os);
			os << ")(";
			genere_code_C(enfant, contexte, true, os, os);
			os << ")";
			break;
		}
		case type_noeud::NUL:
		{
			os << "0";
			break;
		}
		case type_noeud::TAILLE_DE:
		{
			auto index_dt = std::any_cast<long>(b->valeur_calculee);
			auto const &donnees = contexte.magasin_types.donnees_types[index_dt];
			os << "sizeof(";
			contexte.magasin_types.converti_type_C(contexte, "", donnees, os);
			os << ")";
			break;
		}
		case type_noeud::PLAGE:
		{
			/* pris en charge dans type_noeud::POUR */
			break;
		}
		case type_noeud::DIFFERE:
		{
			auto noeud = b->enfants.front();
			contexte.differe_noeud(noeud);
			break;
		}
		case type_noeud::NONSUR:
		{
			contexte.non_sur(true);
			genere_code_C(b->enfants.front(), contexte, false, os, os);
			contexte.non_sur(false);
			break;
		}
		case type_noeud::TABLEAU:
		{
			os << std::any_cast<dls::chaine>(b->valeur_calculee);
			break;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			auto liste_params = std::any_cast<dls::tableau<dls::vue_chaine>>(&b->valeur_calculee);

			auto enfant = b->enfants.debut();
			auto nom_param = liste_params->debut();
			auto virgule = '{';

			for (auto i = 0l; i < liste_params->taille(); ++i) {
				os << virgule;

				os << '.' << broye_nom_simple(*nom_param) << '=';
				genere_code_C(*enfant, contexte, expr_gauche, os, os);
				++enfant;
				++nom_param;

				virgule = ',';
			}

			os << '}';

			break;
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			os << std::any_cast<dls::chaine>(b->valeur_calculee);
			break;
		}
		case type_noeud::INFO_DE:
		{
			auto enfant = b->enfants.front();
			auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];
			os << "&" << dt.ptr_info_type;
			break;
		}
		case type_noeud::MEMOIRE:
		{
			os << "*(";
			genere_code_C(b->enfants.front(), contexte, false, os, os);
			os << ")";
			break;
		}
		case type_noeud::LOGE:
		{
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];
			auto enfant = b->enfants.debut();
			auto nombre_enfant = b->enfants.taille();
			auto a_pointeur = false;
			auto nom_ptr_ret = dls::chaine("");
			auto nom_taille = "__taille_allouee" + dls::vers_chaine(index++);

			if (dt.type_base() == id_morceau::TABLEAU) {
				a_pointeur = true;
				auto nom_ptr = "__ptr" + dls::vers_chaine(b->morceau.ligne_pos);
				auto nom_tabl = "__tabl" + dls::vers_chaine(b->morceau.ligne_pos);
				auto taille_tabl = std::any_cast<int>(b->valeur_calculee);

				os << "long " << nom_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.derefence(),
							os);
				os << ") * " << taille_tabl << ");\n";

				auto dt_ptr = DonneesType{};
				dt_ptr.pousse(id_morceau::POINTEUR);
				dt_ptr.pousse(dt.derefence());

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_ptr,
							os);

				os << nom_ptr << " = (";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_ptr,
							os);

				os << ")(malloc(" << nom_taille << ");\n";

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt,
							os);

				os << " " << nom_tabl << ";\n";
				os << nom_tabl << ".pointeur = " << nom_ptr << ";\n";
				os << nom_tabl << ".taille = " << taille_tabl << ";\n";

				nom_ptr_ret = nom_tabl;
			}
			else if (dt.type_base() == id_morceau::CHAINE) {
				a_pointeur = true;
				auto nom_ptr = "__ptr" + dls::vers_chaine(b->morceau.ligne_pos);
				auto nom_chaine = "__chaine" + dls::vers_chaine(b->morceau.ligne_pos);

				auto enf = *enfant++;

				/* Prépasse pour les accès de membres dans l'expression. */
				genere_code_C_prepasse(enf, contexte, false, os);

				os << "long " << nom_taille << " = ";

				genere_code_C(enf, contexte, false, os, os);
				nombre_enfant -= 1;

				os << ";\n";

				os << "char *" << nom_ptr << " = (char *)(malloc(sizeof(char) * (";
				os << nom_taille << ")));\n";

				os << "chaine " << nom_chaine << ";\n";
				os << nom_chaine << ".pointeur = " << nom_ptr << ";\n";
				os << nom_chaine << ".taille = " << nom_taille << ";\n";

				nom_ptr_ret = nom_chaine;
			}
			else {
				auto const dt_deref = dt.derefence();
				os << "long " << nom_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_deref,
							os);
				os << ");\n";
				auto nom_ptr = "__ptr" + dls::vers_chaine(b->morceau.ligne_pos);

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt,
							os);

				os << " " << nom_ptr << " = (";

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt,
							os);

				os << ")(malloc(" << nom_taille << "));\n";

				/* initialise la structure */
				if ((dt_deref.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
					cree_initialisation(
								contexte,
								dt_deref,
								nom_ptr,
								"->",
								os);
				}

				nom_ptr_ret = nom_ptr;
			}

			/* À FAIRE : que faire si le bloc est absent ? avorter ? */
			if (nombre_enfant == 1) {
				os << "if (" << nom_ptr_ret;

				if (a_pointeur) {
					os << ".pointeur ";
				}

				os << " == 0 ) {\n";
				genere_code_C(*enfant++, contexte, true, os, os);
				os << "}\n";
			}

			os << "__VG_memoire_utilisee__ += " << nom_taille << ";\n";
			b->valeur_calculee = nom_ptr_ret;

			break;
		}
		case type_noeud::DELOGE:
		{
			auto enfant = b->enfants.front();
			auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];
			auto nom_taille = "__taille_allouee" + dls::vers_chaine(index++);

			if (dt.type_base() == id_morceau::TABLEAU || dt.type_base() == id_morceau::CHAINE) {
				os << "long " << nom_taille << " = ";
				genere_code_C(enfant, contexte, true, os, os);
				os << ".taille";

				if (dt.type_base() == id_morceau::TABLEAU) {
					os << " * sizeof(";
					contexte.magasin_types.converti_type_C(
								contexte,
								"",
								dt.derefence(),
								os);
					os << ")";
				}

				os << ";\n";

				os << "free(";
				genere_code_C(enfant, contexte, true, os, os);
				os << ".pointeur);\n";
				genere_code_C(enfant, contexte, true, os, os);
				os << ".pointeur = 0;\n";
				genere_code_C(enfant, contexte, true, os, os);
				os << ".taille = 0;\n";
			}
			else {
				os << "long " << nom_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.derefence(),
							os);
				os << ");\n";

				os << "free(";
				genere_code_C(enfant, contexte, true, os, os);
				os << ");\n";
				genere_code_C(enfant, contexte, true, os, os);
				os << " = 0;\n";
			}

			os << "__VG_memoire_utilisee__ -= " << nom_taille << ";\n";

			break;
		}
		case type_noeud::RELOGE:
		{
			auto &dt_pointeur = contexte.magasin_types.donnees_types[b->index_type];
			auto enfant = b->enfants.debut();
			auto enfant1 = *enfant++;
			auto nombre_enfant = b->enfants.taille();
			auto a_pointeur = false;
			auto nom_ptr_ret = dls::chaine("");
			auto nom_ancienne_taille = "__ancienne_taille" + dls::vers_chaine(index++);
			auto nom_nouvelle_taille = "__nouvelle_taille" + dls::vers_chaine(index++);

			if (dt_pointeur.type_base() == id_morceau::TABLEAU) {
				/* À FAIRE : expression pour les types. */
			}
			else if (dt_pointeur.type_base() == id_morceau::CHAINE) {
				auto enfant2 = *enfant++;
				nombre_enfant -= 1;
				a_pointeur = true;

				os << "int " << nom_ancienne_taille << " = ";
				genere_code_C(enfant1, contexte, true, os, os);
				os << ".taille;\n";

				os << "int " << nom_nouvelle_taille << " = sizeof(char) *";
				genere_code_C(enfant2, contexte, true, os, os);
				os << ";\n";

				genere_code_C(enfant1, contexte, true, os, os);
				os << ".pointeur = (char *)(realloc(";
				genere_code_C(enfant1, contexte, true, os, os);
				os << ".pointeur, " << nom_nouvelle_taille<< "));\n";
				genere_code_C(enfant1, contexte, true, os, os);
				os << ".taille = " << nom_nouvelle_taille << ";\n";
			}
			else {
				os << "int " << nom_ancienne_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_pointeur.derefence(),
							os);
				os << ");\n";

				os << "int " << nom_nouvelle_taille << " = " << nom_ancienne_taille << ";\n";

				genere_code_C(enfant1, contexte, true, os, os);
				os << " = (";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_pointeur,
							os);
				os << ")(realloc(";
				genere_code_C(enfant1, contexte, true, os, os);
				os << ", sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_pointeur.derefence(),
							os);
				os << ")));\n";
			}

			/* À FAIRE : que faire si le bloc est absent ? avorter ? */
			if (nombre_enfant == 2) {
				os << "if (";
				genere_code_C(enfant1, contexte, true, os, os);

				if (a_pointeur) {
					os << ".pointeur ";
				}

				os << " == 0 ) {\n";
				genere_code_C(*enfant++, contexte, true, os, os);
				os << "}\n";
			}

			os << "__VG_memoire_utilisee__ += " << nom_nouvelle_taille
			   << " - " << nom_ancienne_taille << ";\n";

			break;
		}
		case type_noeud::DECLARATION_STRUCTURE:
		{
			/* RAF, puisque le code est généré pour toutes les structures avant
			 * les fonctions. */
			break;
		}
		case type_noeud::DECLARATION_ENUM:
		{
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];

			for (auto enfant : b->enfants) {
				os << "static const ";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt,
							os);

				auto enf0 = enfant->enfants.front();
				auto enf1 = enfant->enfants.back();

				os << ' ' << broye_chaine(b) << '_' << broye_chaine(enf0) << " = ";
				genere_code_C(enf1, contexte, false, os, os);
				os << ";\n";
			}

			break;
		}
		case type_noeud::ASSOCIE:
		{
			/* le premier enfant est l'expression, les suivants les paires */

			auto nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			auto condition = "if";

			for (auto i = 1l; i < nombre_enfants; ++i) {
				auto paire = *iter_enfant++;
				auto enf0 = paire->enfants.front();
				auto enf1 = paire->enfants.back();

				// (else) if (expr == enf0) {
				//     enf1
				// }

				/* À FAIRE : pour les accès membre -> prépasse */
				os << condition << "(";
				genere_code_C(expression, contexte, true, os, os);
				os << " == ";
				genere_code_C(enf0, contexte, true, os, os);
				os << ") {\n";
				genere_code_C(enf1, contexte, false, os, os);
				os << "}\n";

				condition = "else if";
			}

			break;
		}
		case type_noeud::PAIRE_ASSOCIATION:
		{
			/* RAF : pris en charge dans type_noeud::ASSOCIE, ce noeud n'est que
			 * pour ajouter un niveau d'indirection et faciliter la compilation
			 * des associations. */
			assert(false);
			break;
		}
		case type_noeud::RETIENS:
		{
			/* Transformation du code :
			 *
			 * retiens i;
			 *
			 * devient :
			 *
			 * __etat->val = i;
			 * __etat->reprend = 1;
			 * return;
			 * __reprend_coroN:
			 * i = __etat->val;
			 */

			auto df = contexte.donnees_fonction;
			auto &donnees_coroutine = df->donnees_coroutine;
			donnees_coroutine.nombre_retenues += 1;

			auto enfant = b->enfants.front();

			auto feuilles = dls::tableau<base *>{};
			rassemble_feuilles(enfant, feuilles);

			for (auto i = 0l; i < feuilles.taille(); ++i) {
				auto f = feuilles[i];

				genere_code_C_prepasse(f, contexte, true, os);

				os << "__etat->" << df->noms_retours[i] << " = ";
				genere_code_C(f, contexte, true, os, os);
				os << ";\n";
			}

			auto debut = contexte.debut_locales();
			auto fin   = contexte.fin_locales();

			for (; debut != fin; ++debut) {
				if (debut->second.est_argument) {
					continue;
				}

				auto nom_broye = broye_nom_simple(debut->first);
				os << "__etat->" << nom_broye << " = " << nom_broye << ";\n";
			}

			os << "__etat->__reprend_coro = " << donnees_coroutine.nombre_retenues << ";\n";
			os << "return;\n";
			os << "__reprend_coro" << donnees_coroutine.nombre_retenues << ":\n";

			debut = contexte.debut_locales();

			for (; debut != fin; ++debut) {
				if (debut->second.est_argument) {
					continue;
				}

				auto nom_broye = broye_nom_simple(debut->first);
				os << nom_broye << " = __etat->" << nom_broye << ";\n";
			}

			break;
		}
	}
}

}  /* namespace noeud */
