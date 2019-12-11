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

#include "arbre_syntactic.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "generatrice_code_c.hh"
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

void genere_code_C(
		base *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche);

static void genere_code_extra_pre_retour(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os)
{
	if (contexte.donnees_fonction->est_coroutine) {
		os << "__etat->__termine_coro = 1;\n";
	}

	/* génère le code pour les blocs déférés */
	auto pile_noeud = contexte.noeuds_differes();

	while (!pile_noeud.est_vide()) {
		auto noeud = pile_noeud.back();
		genere_code_C(noeud, generatrice, contexte, true);
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
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		DonneesTypeFinal &donnees_type);

static auto cree_info_type_structure_C(
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::vue_chaine_compacte const &nom_struct,
		DonneesStructure const &donnees_structure,
		DonneesTypeFinal &dt)
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
	auto nombre_membres = donnees_structure.index_types.taille();

	if (donnees_structure.est_externe && nombre_membres == 0) {
		os_init << nom_info_type << ".membres.taille = 0;\n";
		os_init << nom_info_type << ".membres.pointeur = 0;\n";
		return nom_info_type;
	}

	/* crée des structures pour chaque membre, et rassemble les pointeurs */
	dls::tableau<dls::chaine> pointeurs;
	pointeurs.reserve(nombre_membres);

	for (auto i = 0l; i < donnees_structure.index_types.taille(); ++i) {
		auto index_dt = donnees_structure.index_types[i];
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
							contexte, generatrice, os_decl, os_init, dt_membre);
			}

			os_decl << "static InfoTypeMembreStructure " << nom_info_type_membre << ";\n";
			os_init << nom_info_type_membre << ".nom.pointeur = \"" << paire_idx_mb.first << "\";\n";
			os_init << nom_info_type_membre << ".nom.taille = " << paire_idx_mb.first.taille()  << ";\n";
			os_init << nom_info_type_membre << broye_nom_simple(".décalage = ") << paire_idx_mb.second.decalage << ";\n";
			os_init << nom_info_type_membre << ".id = (InfoType *)(&" << rderef.ptr_info_type  << ");\n";

			pointeurs.pousse(nom_info_type_membre);
			break;
		}
	}

	/* alloue un tableau fixe pour stocker les pointeurs */
	auto idx_type_tabl = contexte.donnees_structure("InfoTypeMembreStructure").id;
	auto type_struct_membre = DonneesTypeFinal{};
	type_struct_membre.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(idx_type_tabl << 8));

	auto dt_tfixe = DonneesTypeFinal{};
	dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(nombre_membres << 8));
	dt_tfixe.pousse(type_struct_membre);

	auto nom_tableau_fixe = dls::chaine("__tabl_fix_membres") + dls::vers_chaine(index++);

	contexte.magasin_types.converti_type_C(
				contexte, nom_tableau_fixe, dt_tfixe.plage(), os_init);

	os_init << " = ";

	auto virgule = '{';

	for (auto const &ptr : pointeurs) {
		os_init << virgule;
		os_init << ptr;
		virgule = ',';
	}

	os_init << "};\n";

	/* alloue un tableau dynamique */
	auto dt_tdyn = DonneesTypeFinal{};
	dt_tdyn.pousse(id_morceau::TABLEAU);
	dt_tdyn.pousse(type_struct_membre);

	auto nom_tableau_dyn = dls::chaine("__tabl_dyn_membres") + dls::vers_chaine(index++);

	contexte.magasin_types.converti_type_C(
				contexte, nom_tableau_dyn, dt_tdyn.plage(), os_init);

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
		GeneratriceCodeC &generatrice,
		dls::vue_chaine_compacte const &nom_struct,
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

	for (auto enfant : noeud_decl->enfants) {
		auto enf1 = enfant->enfants.back();
		genere_code_C(enf1, generatrice, contexte, false);
	}

	auto &dt = contexte.magasin_types.donnees_types[noeud_decl->index_type];

	contexte.magasin_types.converti_type_C(
				contexte,
				"",
				dt.plage(),
				os_init);

	os_init << " " << nom_tabl_fixe_vals << "[" << nombre_enfants << "] = {\n";

	for (auto enfant : noeud_decl->enfants) {
		auto enf1 = enfant->enfants.back();
		os_init << std::any_cast<dls::chaine>(enf1->valeur_calculee);
		os_init << ',';
	}

	os_init << "};\n";

	os_init << nom_info_type << ".valeurs.pointeur = " << nom_tabl_fixe_vals << ";\n";
	os_init << nom_info_type << ".valeurs.taille = " << nombre_enfants << ";\n";

	return nom_info_type;
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		dls::flux_chaine &os_init,
		DonneesTypeFinal &donnees_type)
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
		case id_morceau::N128:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 128, false);
			break;
		}
		case id_morceau::Z64:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 64, true);
			break;
		}
		case id_morceau::Z128:
		{
			valeur = cree_info_type_entier_C(os_decl, os_init, 128, true);
			break;
		}
		case id_morceau::R16:
		{
			valeur = cree_info_type_reel_C(os_decl, os_init, 16);
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
		case id_morceau::R128:
		{
			valeur = cree_info_type_reel_C(os_decl, os_init, 128);
			break;
		}
		case id_morceau::REFERENCE:
		case id_morceau::POINTEUR:
		{
			auto deref = donnees_type.dereference();

			auto idx = contexte.magasin_types.ajoute_type(deref);
			auto &rderef = contexte.magasin_types.donnees_types[idx];

			if (rderef.ptr_info_type == "") {
				rderef.ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, os_init, rderef);
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
							generatrice,
							contexte.nom_struct(id_structure),
							donnees_structure);

				donnees_type.ptr_info_type = valeur;
			}
			else {
				valeur = cree_info_type_structure_C(
							os_decl,
							os_init,
							contexte,
							generatrice,
							contexte.nom_struct(id_structure),
							donnees_structure,
							donnees_type);
			}

			break;
		}
		case id_morceau::TROIS_POINTS:
		case id_morceau::TABLEAU:
		{
			auto deref = donnees_type.dereference();

			auto nom_info_type = "__info_type_tableau" + dls::vers_chaine(index++);

			/* dans le cas des arguments variadics des fonctions externes */
			if (est_invalide(deref)) {
				os_decl << "static InfoTypeTableau " << nom_info_type << ";\n";
				os_init << nom_info_type << ".id = id_info_TABLEAU;\n";
				os_init << nom_info_type << broye_nom_simple(".type_pointé") << " = 0;\n";
			}
			else {
				auto idx = contexte.magasin_types.ajoute_type(deref);
				auto &rderef = contexte.magasin_types.donnees_types[idx];

				if (rderef.ptr_info_type == "") {
					rderef.ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, os_init, rderef);
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

				cree_info_type_C(contexte, generatrice, os_decl, os_init, dt_prm);
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

static auto cree_eini(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os, base *b)
{
	auto nom_eini = dls::chaine("__eini_").append(dls::vers_chaine(b).c_str());
	auto nom_var = dls::chaine{};

	auto &dt = contexte.magasin_types.donnees_types[b->index_type];

	genere_code_C(b, generatrice, contexte, false);

	/* dans le cas d'un nombre ou d'un tableau, etc. */
	if (b->type != type_noeud::VARIABLE) {
		nom_var = dls::chaine("__var_").append(nom_eini);		
		generatrice.declare_variable(dt, nom_var, std::any_cast<dls::chaine>(b->valeur_calculee));
	}
	else {
		nom_var = std::any_cast<dls::chaine>(b->valeur_calculee);
	}

	os << "eini " << nom_eini << ";\n";
	os << nom_eini << ".pointeur = &" << nom_var << ";\n";
	os << nom_eini << ".info = (InfoType *)(&" << dt.ptr_info_type << ");\n";

	return nom_eini;
}

static void cree_appel(
		base *b,
		dls::flux_chaine &os,
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::chaine const &nom_broye,
		dls::liste<base *> const &enfants)
{
	for (auto enf : enfants) {
		if ((enf->drapeaux & CONVERTI_TABLEAU) != 0) {
			auto nom_tabl_fixe = dls::chaine(enf->chaine());

			if (enf->type == type_noeud::CONSTRUIT_TABLEAU) {
				genere_code_C(enf, generatrice, contexte, false);
				nom_tabl_fixe = std::any_cast<dls::chaine>(enf->valeur_calculee);
			}

			auto nom_tableau = dls::chaine("__tabl_dyn") + dls::vers_chaine(index++);
			enf->valeur_calculee = nom_tableau;

			os << "Tableau_";
			auto &dt = contexte.magasin_types.donnees_types[enf->index_type];
			contexte.magasin_types.converti_type_C(contexte, "", dt.dereference(), os);

			os << ' ' << nom_tableau << ";\n";
			os << nom_tableau << ".taille = " << static_cast<size_t>(dt.type_base() >> 8) << ";\n";
			os << nom_tableau << ".pointeur = " << nom_tabl_fixe << ";\n";
		}
		else if ((enf->drapeaux & CONVERTI_EINI) != 0) {
			enf->valeur_calculee = cree_eini(contexte, generatrice, os, enf);
		}
		else if ((enf->drapeaux & EXTRAIT_CHAINE_C) != 0) {
			auto nom_var_chaine = dls::chaine("");

			if (enf->type != type_noeud::VARIABLE) {
				nom_var_chaine = "__chaine" + dls::vers_chaine(enf);

				genere_code_C(enf, generatrice, contexte, false);
				os << "chaine " << nom_var_chaine << " = ";
				os << std::any_cast<dls::chaine>(enf->valeur_calculee);
				os << ";\n";
			}
			else {
				nom_var_chaine = enf->chaine();
			}

			auto nom_var = "__pointeur" + dls::vers_chaine(enf);

			os << "const char *" + nom_var;
			os << " = " << nom_var_chaine << ".pointeur;\n";

			enf->valeur_calculee = nom_var;
		}
		else if ((enf->drapeaux & CONVERTI_TABLEAU_OCTET) != 0) {
			auto nom_var_tableau = "__tableau_octet" + dls::vers_chaine(enf);

			os << "Tableau_octet " << nom_var_tableau << ";\n";

			auto &dt = contexte.magasin_types.donnees_types[enf->index_type];
			auto type_base = dt.type_base();

			switch (type_base & 0xff) {
				default:
				{
					os << nom_var_tableau << ".pointeur = (unsigned char*)(&";
					genere_code_C(enf, generatrice, contexte, false);
					os << ");\n";

					os << nom_var_tableau << ".taille = sizeof(";
					contexte.magasin_types.converti_type_C(contexte, "", dt.plage(), os);
					os << ");\n";
					break;
				}
				case id_morceau::POINTEUR:
				{
					os << nom_var_tableau << ".pointeur = (unsigned char*)(";
					genere_code_C(enf, generatrice, contexte, false);
					os << ".pointeur);\n";

					os << nom_var_tableau << ".taille = ";
					genere_code_C(enf, generatrice, contexte, false);
					os << ".sizeof(";
					contexte.magasin_types.converti_type_C(contexte, "", dt.dereference(), os);
					os << ");\n";
					break;
				}
				case id_morceau::CHAINE:
				{
					genere_code_C(enf, generatrice, contexte, false);

					os << nom_var_tableau << ".pointeur = ";
					os << std::any_cast<dls::chaine>(enf->valeur_calculee);
					os << ".pointeur;\n";

					os << nom_var_tableau << ".taille = ";
					genere_code_C(enf, generatrice, contexte, false);
					os << ".taille;\n";
					break;
				}
				case id_morceau::TABLEAU:
				{
					auto taille = static_cast<int>(type_base >> 8);

					if (taille == 0) {
						os << nom_var_tableau << ".pointeur = (unsigned char*)(";
						genere_code_C(enf, generatrice, contexte, false);
						os << ".pointeur);\n";

						os << nom_var_tableau << ".taille = ";
						genere_code_C(enf, generatrice, contexte, false);
						os << ".taille * sizeof(";
						contexte.magasin_types.converti_type_C(contexte, "", dt.dereference(), os);
						os << ");\n";
					}
					else {
						os << nom_var_tableau << ".pointeur = (unsigned char*)(";
						genere_code_C(enf, generatrice, contexte, false);
						os << ");\n";

						os << nom_var_tableau << ".taille = " << taille << " * sizeof(";
						contexte.magasin_types.converti_type_C(contexte, "", dt.dereference(), os);
						os << ");\n";
					}

					break;
				}
			}

			enf->valeur_calculee = nom_var_tableau;
		}
		else if ((enf->drapeaux & PREND_REFERENCE) != 0) {
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
			genere_code_C(enf, generatrice, contexte, true);

			enf->drapeaux |= PREND_REFERENCE;
		}
		else {
			genere_code_C(enf, generatrice, contexte, false);
		}
	}

	auto &dt = contexte.magasin_types.donnees_types[b->index_type];

	dls::liste<base *> liste_var_retour{};
	dls::tableau<dls::chaine> liste_noms_retour{};

	if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET) {
		liste_var_retour = std::any_cast<dls::liste<base *>>(b->valeur_calculee);
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");

		for (auto n : liste_var_retour) {
			genere_code_C(n, generatrice, contexte, false);
		}
	}
	else if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET2) {
		liste_noms_retour = std::any_cast<dls::tableau<dls::chaine>>(b->valeur_calculee);
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}
	else if (dt.type_base() != id_morceau::RIEN && (b->aide_generation_code == APPEL_POINTEUR_FONCTION || ((b->df != nullptr) && !b->df->est_coroutine))) {
		auto nom_indirection = "__ret" + dls::vers_chaine(b);
		contexte.magasin_types.converti_type_C(contexte, nom_indirection, dt.plage(), os);

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

	if (b->df != nullptr) {
		auto df = b->df;
		auto noeud_decl = df->noeud_decl;

		if (!df->est_externe && noeud_decl != nullptr && !dls::outils::possede_drapeau(noeud_decl->drapeaux, FORCE_NULCTX)) {
			os << virgule;
			os << "ctx";
			virgule = ',';
		}

		if (df->est_coroutine) {
			os << virgule << "&__etat" << dls::vers_chaine(b);
			virgule = ',';
		}
	}

	for (auto enf : enfants) {
		os << virgule;
		if ((enf->drapeaux & PREND_REFERENCE) != 0) {
			os << '&';
		}
		os << std::any_cast<dls::chaine>(enf->valeur_calculee);
		virgule = ',';
	}

	for (auto n : liste_var_retour) {
		os << virgule;
		os << "&(" << std::any_cast<dls::chaine>(n->valeur_calculee) << ')';
		virgule = ',';
	}

	for (auto n : liste_noms_retour) {
		os << virgule << n;
		virgule = ',';
	}

	os << ");\n";
}

static void declare_structures_C(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	contexte.magasin_types.declare_structures_C(contexte, os);

	for (auto is = 0l; is < contexte.structures.taille(); ++is) {
		auto const &donnees = contexte.donnees_structure(is);

		if (donnees.est_enum || donnees.est_externe) {
			continue;
		}

		auto const &nom_struct = broye_nom_simple(contexte.nom_struct(is));

		if (donnees.est_union) {
			if (donnees.est_nonsur) {
				os << "typedef union " << nom_struct << "{\n";
			}
			else {
				os << "typedef struct " << nom_struct << "{\n";
				os << "int membre_actif;\n";
				os << "union {\n";
			}
		}
		else {
			os << "typedef struct " << nom_struct << "{\n";
		}

		for (auto i = 0l; i < donnees.index_types.taille(); ++i) {
			auto index_dt = donnees.index_types[i];
			auto &dt = contexte.magasin_types.donnees_types[index_dt];

			for (auto paire_idx_mb : donnees.donnees_membres) {
				if (paire_idx_mb.second.index_membre == i) {
					auto nom = broye_nom_simple(paire_idx_mb.first);

					contexte.magasin_types.converti_type_C(
								contexte,
								nom,
								dt.plage(),
								os,
								false,
								true);

					os << ";\n";
					break;
				}
			}
		}

		if (donnees.est_union && !donnees.est_nonsur) {
			os << "};\n";
		}

		os << "} " << nom_struct << ";\n\n";
	}
}

static void cree_initialisation(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		DonneesTypeFinal::type_plage dt_parent,
		dls::chaine const &chaine_parent,
		dls::vue_chaine_compacte const &accesseur,
		dls::flux_chaine &os)
{
	if (dt_parent.front() == id_morceau::CHAINE || dt_parent.front() == id_morceau::TABLEAU) {
		os << chaine_parent << accesseur << "pointeur = 0;\n";
		os << chaine_parent << accesseur << "taille = 0;\n";
	}
	else if ((dt_parent.front() & 0xff) == id_morceau::CHAINE_CARACTERE) {
		auto const index_structure = static_cast<long>(dt_parent.front() >> 8);
		auto const &ds = contexte.donnees_structure(index_structure);

		for (auto i = 0l; i < ds.index_types.taille(); ++i) {
			auto index_dt = ds.index_types[i];
			auto dt_enf = contexte.magasin_types.donnees_types[index_dt].plage();

			for (auto paire_idx_mb : ds.donnees_membres) {
				auto const &donnees_membre = paire_idx_mb.second;

				if (donnees_membre.index_membre != i) {
					continue;
				}

				auto nom = paire_idx_mb.first;
				auto decl_nom = chaine_parent + dls::chaine(accesseur) + broye_nom_simple(nom);

				if (donnees_membre.noeud_decl != nullptr) {
					/* indirection pour les chaines ou autres */
					genere_code_C(donnees_membre.noeud_decl, generatrice, contexte, false);
					os << decl_nom << " = ";
					os << std::any_cast<dls::chaine>(donnees_membre.noeud_decl->valeur_calculee);
					os << ";\n";
				}
				else {
					cree_initialisation(
								contexte,
								generatrice,
								dt_enf,
								decl_nom,
								".",
								os);
				}
			}
		}
	}
	else if ((dt_parent.front() & 0xff) == id_morceau::TABLEAU) {
		/* À FAIRE */
	}
	else {
		os << chaine_parent << " = 0;\n";
	}
}

static void genere_code_acces_membre(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		base *b,
		base *structure,
		base *membre,
		bool expr_gauche)
{
	auto flux = dls::flux_chaine();

	auto nom_acces = "__acces" + dls::vers_chaine(b);
	b->valeur_calculee = nom_acces;

	if (b->aide_generation_code == ACCEDE_MODULE) {
		genere_code_C(membre, generatrice, contexte, false);
		flux << std::any_cast<dls::chaine>(membre->valeur_calculee);
	}
	else if (b->aide_generation_code == APPEL_FONCTION_SYNT_UNI) {
		cree_appel(membre, flux, contexte, generatrice, membre->nom_fonction_appel, membre->enfants);
		b->valeur_calculee = membre->valeur_calculee;
	}
	else {
		auto const &index_type = structure->index_type;
		auto type_structure = contexte.magasin_types.donnees_types[index_type].plage();
		auto est_pointeur = type_structure.front() == id_morceau::POINTEUR;

		if (est_pointeur) {
			type_structure.effronte();
		}

		if ((type_structure.front() & 0xff) == id_morceau::TABLEAU) {
			auto taille = static_cast<size_t>(type_structure.front() >> 8);

			if (taille != 0) {
				generatrice.os << "long " << nom_acces << " = " << taille << ";\n";
				flux << nom_acces;
			}
			else {
				genere_code_C(structure, generatrice, contexte, expr_gauche);

				if (membre->type != type_noeud::VARIABLE) {
					genere_code_C(membre, generatrice, contexte, expr_gauche);
				}

				flux << std::any_cast<dls::chaine>(structure->valeur_calculee);
				flux << ((est_pointeur) ? "->" : ".");
				flux << broye_chaine(membre);
			}			
		}
		else if (est_type_tableau_fixe(type_structure)) {
			auto taille_tableau = static_cast<size_t>(type_structure.front() >> 8);
			generatrice.os << "long " << nom_acces << " = " << taille_tableau << ";\n";
			flux << nom_acces;
		}
		/* vérifie si nous avons une énumération */
		else if (contexte.structure_existe(structure->chaine())) {
			auto &ds = contexte.donnees_structure(structure->chaine());

			if (ds.est_enum) {
				generatrice.os << "long " << nom_acces << " = "
				   << broye_chaine(structure) << '_' << broye_chaine(membre) << ";\n";
				flux << nom_acces;
			}
		}
		else {
			genere_code_C(structure, generatrice, contexte, expr_gauche);

			if (membre->type != type_noeud::VARIABLE) {
				genere_code_C(membre, generatrice, contexte, expr_gauche);
			}

			flux << std::any_cast<dls::chaine>(structure->valeur_calculee);
			flux << ((est_pointeur) ? "->" : ".");
			flux << broye_chaine(membre);
		}
	}

	if (expr_gauche) {
		b->valeur_calculee = dls::chaine(flux.chn());
	}
	else {
		if (membre->type == type_noeud::APPEL_FONCTION) {
			membre->nom_fonction_appel = flux.chn();
			genere_code_C(membre, generatrice, contexte, false);

			auto &dt = contexte.magasin_types.donnees_types[b->index_type];
			generatrice.declare_variable(dt, nom_acces, std::any_cast<dls::chaine>(membre->valeur_calculee));

			b->valeur_calculee = nom_acces;
		}
		else {
			auto nom_var = "__var_temp" + dls::vers_chaine(index++);
			generatrice.os << "const ";
			generatrice.declare_variable(b->index_type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
		}
	}
}

static void genere_code_echec_logement(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::chaine const &nom_ptr,
		bool a_pointeur,
		base *b,
		base *bloc)
{
	generatrice.os << "if (" << nom_ptr;
	if (a_pointeur) {
		generatrice.os << ".pointeur ";
	}
	generatrice.os << " == 0)";

	if (bloc) {
		genere_code_C(bloc, generatrice, contexte, true);
	}
	else {
		auto const &morceau = b->morceau;
		auto module = contexte.fichier(static_cast<size_t>(morceau.fichier));
		auto pos = trouve_position(morceau, module);

		generatrice.os << " {\n";
		generatrice.os << "KR__hors_memoire(";
		generatrice.os << '"' << module->chemin << '"' << ',';
		generatrice.os << pos.ligne + 1;
		generatrice.os << ");\n";
		generatrice.os << "}\n";
	}
}

/* Génère le code C pour la base b passée en paramètre.
 *
 * Le code est généré en visitant d'abord les enfants des noeuds avant ceux-ci.
 * Ceci nous permet de générer du code avant celui des expressions afin
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
void genere_code_C(
		base *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			break;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			auto module = contexte.fichier(static_cast<size_t>(b->morceau.fichier))->module;
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

			using dls::outils::possede_drapeau;

			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			if (est_externe) {
				return;
			}

			/* Crée fonction */
			auto nom_fonction = donnees_fonction->nom_broye;

			auto moult_retour = donnees_fonction->idx_types_retours.taille() > 1;

			if (donnees_fonction->est_coroutine) {
				generatrice.os << "typedef struct __etat_coro" << nom_fonction << " { bool __reprend_coro; bool __termine_coro; ";

				auto idx_ret = 0l;
				for (auto idx : donnees_fonction->idx_types_retours) {
					auto &nom_ret = donnees_fonction->noms_retours[idx_ret++];

					auto &dt = contexte.magasin_types.donnees_types[idx];
					contexte.magasin_types.converti_type_C(
								contexte,
								nom_ret,
								dt.plage(),
								generatrice.os);

					generatrice.os << ";\n";
				}

				auto &donnees_coroutine = donnees_fonction->donnees_coroutine;

				for (auto const &paire : donnees_coroutine.variables) {
					auto dt_m = contexte.magasin_types.donnees_types[paire.second.first].plage();

					/* Stocke un pointeur pour ne pas qu'il soit invalidé par
					 * le retour de la coroutine. */
					auto requiers_pointeur = (paire.second.second & BESOIN_DEREF) != 0;

					if (est_type_tableau_fixe(dt_m)) {
						/* déférence, on stockera un pointeur */
						dt_m.effronte();
						requiers_pointeur = true;
					}

					contexte.magasin_types.converti_type_C(
								contexte,
								"",
								dt_m,
								generatrice.os);

					if (requiers_pointeur) {
						generatrice.os << '*';
					}

					generatrice.os << ' ' << broye_nom_simple(paire.first) << ";\n";
				}

				generatrice.os << " } __etat_coro" << nom_fonction << ";\n";

				generatrice.os << "static ";

				if (!possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
					generatrice.os << "inline ";
				}

				generatrice.os << "void " << nom_fonction;
			}
			else if (moult_retour) {
				if (possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
					generatrice.os << "static inline void ";
				}
				else if (possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
					generatrice.os << "static void __attribute__ ((noinline)) ";
				}
				else {
					generatrice.os << "static void ";
				}

				generatrice.os << nom_fonction;
			}
			else {
				if (possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
					generatrice.os << "static inline ";
				}
				else if (possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
					generatrice.os << "__attribute__ ((noinline)) ";
				}

				contexte.magasin_types.converti_type_C(contexte,
							nom_fonction,
							contexte.magasin_types.donnees_types[b->index_type].plage(),
						generatrice.os);
			}

			contexte.commence_fonction(donnees_fonction);

			/* Crée code pour les arguments */

			auto virgule = '(';

			if (donnees_fonction->args.taille() == 0 && !moult_retour) {
				generatrice.os << '(';
				virgule = ' ';
			}

			if (!donnees_fonction->est_externe && !possede_drapeau(b->drapeaux, FORCE_NULCTX)) {
				generatrice.os << virgule;
				generatrice.os << "__contexte_global *ctx";
				virgule = ',';

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = true;
				donnees_var.est_variadic = false;
				donnees_var.index_type = contexte.index_type_ctx;
				donnees_var.est_argument = true;

				contexte.pousse_locale("ctx", donnees_var);
			}

			if (donnees_fonction->est_coroutine) {
				generatrice.os << virgule;
				generatrice.os << "__etat_coro" << nom_fonction << " *__etat";
				virgule = ',';
			}

			for (auto &argument : donnees_fonction->args) {
				generatrice.os << virgule;

				auto index_type = argument.index_type;

				auto dt = contexte.magasin_types.donnees_types[index_type];

				auto nom_broye = broye_nom_simple(argument.nom);

				contexte.magasin_types.converti_type_C(
							contexte,
							nom_broye,
							dt.plage(),
							generatrice.os);

				virgule = ',';

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = argument.est_dynamic;
				donnees_var.est_variadic = argument.est_variadic;
				donnees_var.index_type = index_type;
				donnees_var.est_argument = true;

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
					donnees_var.index_type = contexte.magasin_types.ajoute_type(dt.dereference());
					dt = contexte.magasin_types.donnees_types[donnees_var.index_type];
				}

				contexte.pousse_locale(argument.nom, donnees_var);

				if (argument.est_employe) {
					auto &dt_var = contexte.magasin_types.donnees_types[argument.index_type];
					auto id_structure = 0l;
					auto est_pointeur = false;

					if (dt_var.type_base() == id_morceau::POINTEUR) {
						est_pointeur = true;
						id_structure = static_cast<long>(dt_var.dereference().front() >> 8);
					}
					else {
						id_structure = static_cast<long>(dt_var.type_base() >> 8);
					}

					auto &ds = contexte.donnees_structure(id_structure);

					/* pousse chaque membre de la structure sur la pile */

					for (auto &dm : ds.donnees_membres) {
						auto index_dt_m = ds.index_types[dm.second.index_membre];

						donnees_var.est_dynamique = argument.est_dynamic;
						donnees_var.index_type = index_dt_m;
						donnees_var.est_argument = true;
						donnees_var.est_membre_emploie = true;
						donnees_var.structure = nom_broye + (est_pointeur ? "->" : ".");

						contexte.pousse_locale(dm.first, donnees_var);
					}
				}
			}

			if (moult_retour && !donnees_fonction->est_coroutine) {
				auto idx_ret = 0l;
				for (auto idx : donnees_fonction->idx_types_retours) {
					generatrice.os << virgule;

					auto nom_ret = "*" + donnees_fonction->noms_retours[idx_ret++];

					auto &dt = contexte.magasin_types.donnees_types[idx];
					contexte.magasin_types.converti_type_C(
								contexte,
								nom_ret,
								dt.plage(),
								generatrice.os);

					virgule = ',';
				}
			}

			generatrice.os << ")\n";

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.back();

			if (donnees_fonction->est_coroutine) {
				/* les coroutines ont du code avant le bloc, donc ajoute
				 * explicitement des accolades, pour les autres fonctions, le
				 * bloc se charge d'ajouter les accolades */
				generatrice.os << "{\n";

				for (auto i = 1; i <= donnees_fonction->donnees_coroutine.nombre_retenues; ++i) {
					generatrice.os << "if (__etat->__reprend_coro == " << i << ") { goto __reprend_coro" << i << "; }";
				}

				/* remet à zéro car nous avons besoin de les compter pour
				 * générer les labels des goto */
				donnees_fonction->donnees_coroutine.nombre_retenues = 0;
			}

			genere_code_C(bloc, generatrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			}

			if (donnees_fonction->est_coroutine) {
				generatrice.os << "}\n";
			}

			contexte.termine_fonction();

			break;
		}
		case type_noeud::LISTE_PARAMETRES_FONCTION:
		{
			/* géré dans DECLARATION_FONCTION */
			break;
		}
		case type_noeud::APPEL_FONCTION:
		{
			cree_appel(b, generatrice.os, contexte, generatrice, b->nom_fonction_appel, b->enfants);
			break;
		}
		case type_noeud::VARIABLE:
		{
			auto drapeaux = contexte.drapeaux_variable(b->morceau.chaine);
			auto flux = dls::flux_chaine();

			if (b->aide_generation_code == GENERE_CODE_DECL_VAR) {
				auto dt = contexte.magasin_types.donnees_types[b->index_type];

				/* pour les assignations de tableaux fixes, remplace les crochets
				 * par des pointeurs pour la déclaration */
				if (dls::outils::possede_drapeau(b->drapeaux, POUR_ASSIGNATION)) {
					if (dt.type_base() != id_morceau::TABLEAU && (dt.type_base() & 0xff) == id_morceau::TABLEAU) {
						auto ndt = DonneesTypeFinal{};
						ndt.pousse(id_morceau::POINTEUR);
						ndt.pousse(dt.dereference());

						dt = ndt;
					}
				}

				auto nom_broye = broye_chaine(b);

				contexte.magasin_types.converti_type_C(
							contexte,
							nom_broye,
							dt.plage(),
							flux);

				b->valeur_calculee = dls::chaine(flux.chn());

				if (contexte.donnees_fonction == nullptr) {
					auto donnees_var = DonneesVariable{};
					donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
					donnees_var.index_type = b->index_type;

					if (dt.type_base() == id_morceau::REFERENCE) {
						donnees_var.drapeaux |= BESOIN_DEREF;
					}

					contexte.pousse_globale(b->chaine(), donnees_var);
					return;
				}

				/* nous avons une déclaration, initialise à zéro */
				if (!dls::outils::possede_drapeau(b->drapeaux, POUR_ASSIGNATION)) {
					generatrice.os << flux.chn() << ";\n";
					cree_initialisation(
								contexte,
								generatrice,
								dt.plage(),
								nom_broye,
								".",
								generatrice.os);
				}

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
				donnees_var.index_type = b->index_type;

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
				}

				contexte.pousse_locale(b->chaine(), donnees_var);
			}
			/* désactive la vérification car les variables dans les
			 * constructions de structures n'ont pas cette aide */
			else /*if (b->aide_generation_code == GENERE_CODE_ACCES_VAR)*/ {
				if ((drapeaux & BESOIN_DEREF) != 0) {
					flux << "(*" << broye_chaine(b) << ")";
				}
				else {
					if (b->nom_fonction_appel != "") {
						flux << b->nom_fonction_appel;
					}
					else {
						auto dv = contexte.donnees_variable(b->morceau.chaine);

						if (dv.est_membre_emploie) {
							flux << dv.structure;
						}

						if ((b->drapeaux & PREND_REFERENCE) != 0) {
							flux << '&';
						}

						flux << broye_chaine(b);
					}
				}

				b->valeur_calculee = dls::chaine(flux.chn());
			}

			break;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			auto structure = b->enfants.front();
			auto membre = b->enfants.back();
			genere_code_acces_membre(contexte, generatrice, b, structure, membre, expr_gauche);
			break;
		}
		case type_noeud::ACCES_MEMBRE_DE:
		{
			auto structure = b->enfants.back();
			auto membre = b->enfants.front();
			genere_code_acces_membre(contexte, generatrice, b, structure, membre, expr_gauche);
			break;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.taille() == 2);

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
						genere_code_C(f, generatrice, contexte, true);
						generatrice.os << std::any_cast<dls::chaine>(f->valeur_calculee);
						generatrice.os << ';' << '\n';
					}

					f->drapeaux &= ~POUR_ASSIGNATION;
					f->aide_generation_code = 0;
					noeuds.pousse(f);
				}

				expression->aide_generation_code = APPEL_FONCTION_MOULT_RET;
				expression->valeur_calculee = noeuds;

				genere_code_C(expression,
							  generatrice,
							  contexte,
							  true);
				return;
			}

			auto compatibilite = std::any_cast<niveau_compat>(b->valeur_calculee);
			auto expression_modifiee = false;
			auto nouvelle_expr = dls::chaine();

			/* À FAIRE : conversion tableau */
			if ((compatibilite & niveau_compat::converti_tableau) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_TABLEAU;
			}

			if ((compatibilite & niveau_compat::converti_eini) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_EINI;
				expression_modifiee = true;

				nouvelle_expr = cree_eini(contexte, generatrice, generatrice.os, expression);
			}

			if ((compatibilite & niveau_compat::extrait_eini) != niveau_compat::aucune) {
				expression->drapeaux |= EXTRAIT_EINI;
				expression->index_type = variable->index_type;
				expression_modifiee = true;

				auto nom_eini = dls::chaine("__eini_ext_")
						.append(dls::vers_chaine(reinterpret_cast<long>(expression)));

				auto &dt = contexte.magasin_types.donnees_types[expression->index_type];

				contexte.magasin_types.converti_type_C(contexte, "", dt.plage(), generatrice.os);

				generatrice.os << " " << nom_eini << " = *(";

				contexte.magasin_types.converti_type_C(contexte, "", dt.plage(), generatrice.os);

				generatrice.os << " *)(";
				genere_code_C(expression, generatrice, contexte, false);
				generatrice.os << ".pointeur);\n";

				nouvelle_expr = nom_eini;
			}

			if (expression->type == type_noeud::LOGE) {
				expression_modifiee = true;
				genere_code_C(expression, generatrice, contexte, false);
				nouvelle_expr = std::any_cast<dls::chaine>(expression->valeur_calculee);
			}

			if (!expression_modifiee) {
				genere_code_C(expression, generatrice, contexte, false);
			}

			variable->drapeaux |= POUR_ASSIGNATION;
			genere_code_C(variable, generatrice, contexte, true);

			generatrice.os << std::any_cast<dls::chaine>(variable->valeur_calculee);
			generatrice.os << " = ";

			if (!expression_modifiee) {
				generatrice.os << std::any_cast<dls::chaine>(expression->valeur_calculee);
			}
			else {
				generatrice.os << nouvelle_expr;
			}

			/* pour les globales */
			generatrice.os << ";\n";

			break;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_reel(
												 b->morceau.chaine,
												 b->morceau.identifiant);

			b->valeur_calculee = dls::vers_chaine(valeur);
			break;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<long>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_entier(
												 b->morceau.chaine,
												 b->morceau.identifiant);

			b->valeur_calculee = dls::vers_chaine(valeur);
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

			auto flux = dls::flux_chaine();

			genere_code_C(enfant1, generatrice, contexte, expr_gauche);
			genere_code_C(enfant2, generatrice, contexte, expr_gauche);

			switch (b->morceau.identifiant) {
				default:
				{
					/* vérifie (a comp b comp c), transforme ((a comp b) && (b comp c)) */
					if (est_operateur_comp(b->morceau.identifiant)) {
						if (enfant1->morceau.identifiant == b->morceau.identifiant) {
							genere_code_C(enfant1->enfants.back(), generatrice, contexte, expr_gauche);

							/* (a comp b) */
							flux << '(';
							flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
							flux << ')';

							flux << "&&";

							/* (b comp c) */
							flux << '(';
							flux << std::any_cast<dls::chaine>(enfant1->enfants.back()->valeur_calculee);
							flux << b->morceau.chaine;
							flux << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							flux << ')';
						}
						else {
							flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
							flux << b->morceau.chaine;
							flux << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
						}
					}
					else {
						flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
						flux << b->morceau.chaine;
						flux << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
					}

					break;
				}
				case id_morceau::CROCHET_OUVRANT:
				{
					auto type_base = type1.type_base();

					/* À CONSIDÉRER :
					 * - directive pour ne pas générer le code de vérification,
					 *   car les branches nuisent à la vitesse d'exécution des
					 *   programmes
					 * - tests redondants ou inutiles, par exemple :
					 *    - ceci génère deux fois la même instruction
					 *      x[i] = 0;
					 *      y = x[i];
					 *    - ceci génère une instruction inutile
					 *	    dyn x : [6]z32;
					 *      x[0] = 8;
					 */

					auto const &morceau = b->morceau;
					auto module = contexte.fichier(static_cast<size_t>(morceau.fichier));
					auto pos = trouve_position(morceau, module);

					switch (type_base & 0xff) {
						case id_morceau::POINTEUR:
						{
							flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
							flux << '[';
							flux << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							flux << ']';
							break;
						}
						case id_morceau::CHAINE:
						{
							generatrice.os << "if (";
							generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							generatrice.os << " < 0 || ";
							generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							generatrice.os << " >= ";
							generatrice.os << std::any_cast<dls::chaine>(enfant1->valeur_calculee);					
							generatrice.os << ".taille) {\n";
							generatrice.os << "KR__depassement_limites(";
							generatrice.os << '"' << module->chemin << '"' << ',';
							generatrice.os << pos.ligne + 1 << ',';
							generatrice.os << "\"de la chaine\",";
							generatrice.os << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
							generatrice.os << ".taille,";
							generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							generatrice.os << ");\n}\n";

							flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
							flux << ".pointeur";
							flux << '[';
							flux << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							flux << ']';

							break;
						}
						case id_morceau::TABLEAU:
						{
							auto taille_tableau = static_cast<int>(type_base >> 8);

							generatrice.os << "if (";
							generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							generatrice.os << " < 0 || ";
							generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							generatrice.os << " >= ";

							if (taille_tableau == 0) {
								generatrice.os << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
								generatrice.os << ".taille";
							}
							else {
								generatrice.os << taille_tableau;
							}

							generatrice.os << ") {\n";
							generatrice.os << "KR__depassement_limites(";
							generatrice.os << '"' << module->chemin << '"' << ',';
							generatrice.os << pos.ligne + 1 << ',';
							generatrice.os << "\"du tableau\",";
							if (taille_tableau == 0) {
								generatrice.os << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
								generatrice.os << ".taille";
							}
							else {
								generatrice.os << taille_tableau;
							}
							generatrice.os << ",";
							generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							generatrice.os << ");\n}\n";

							flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);

							if (taille_tableau == 0) {
								flux << ".pointeur";
							}

							flux << '[';
							flux << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
							flux << ']';
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

			/* pour les accès tableaux à gauche d'un '=', il ne faut pas passer
			 * par une variable temporaire */
			if (expr_gauche) {
				b->valeur_calculee = dls::chaine(flux.chn());
			}
			else {
				auto nom_var = "__var_temp" + dls::vers_chaine(index++);
				generatrice.os << "const ";
				generatrice.declare_variable(b->index_type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto enfant = b->enfants.front();

			/* force une expression si l'opérateur est @, pour que les
			 * expressions du type @a[0] retourne le pointeur à a + 0 et non le
			 * pointeur de la variable temporaire du code généré */
			expr_gauche |= b->morceau.identifiant == id_morceau::AROBASE;
			genere_code_C(enfant, generatrice, contexte, expr_gauche);

			char const *pref = nullptr;

			switch (b->morceau.identifiant) {
				case id_morceau::EXCLAMATION:
				{
					pref = "!(";
					break;
				}
				case id_morceau::TILDE:
				{
					pref = "~(";
					break;
				}
				case id_morceau::AROBASE:
				{
					pref = "&(";
					break;
				}
				case id_morceau::PLUS_UNAIRE:
				{
					pref = "(";
					break;
				}
				case id_morceau::MOINS_UNAIRE:
				{
					pref = "-(";
					break;
				}
				default:
				{
					break;
				}
			}

			b->valeur_calculee = pref + std::any_cast<dls::chaine>(enfant->valeur_calculee) + ")";

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
					genere_code_C(enfant, generatrice, contexte, false);
				}
				else if (enfant->identifiant() == id_morceau::VIRGULE) {
					/* retourne a, b; -> *__ret1 = a; *__ret2 = b; return; */
					dls::tableau<base *> feuilles;
					rassemble_feuilles(enfant, feuilles);

					auto idx = 0l;
					for (auto f : feuilles) {
						genere_code_C(f, generatrice, contexte, false);

						generatrice.os << '*' << df->noms_retours[idx++] << " = ";
						generatrice.os << std::any_cast<dls::chaine>(f->valeur_calculee);
						generatrice.os << ';';
					}
				}
			}
			else if (b->aide_generation_code == GENERE_CODE_RETOUR_SIMPLE) {
				auto enfant = b->enfants.front();
				auto df = contexte.donnees_fonction;

				nom_variable = df->noms_retours[0];

				genere_code_C(enfant, generatrice, contexte, false);

				auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];

				generatrice.declare_variable(
							dt,
							nom_variable,
							std::any_cast<dls::chaine>(enfant->valeur_calculee));
			}

			/* NOTE : le code différé doit être crée après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);

			generatrice.os << "return " << nom_variable << ";\n";

			break;
		}
		case type_noeud::CHAINE_LITTERALE:
		{
			/* Note : dû à la possibilité de différer le code, nous devons
			 * utiliser la chaine originale. */
			auto chaine = b->morceau.chaine;

			auto nom_chaine = "__chaine_tmp" + dls::vers_chaine(b);

			generatrice.os << "chaine " << nom_chaine << " = {.pointeur=";
			generatrice.os << '"';

			for (auto c : chaine) {
				if (c == '\n') {
					generatrice.os << '\\' << 'n';
				}
				else if (c == '\t') {
					generatrice.os << '\\' << 't';
				}
				else {
					generatrice.os << c;
				}
			}

			generatrice.os << '"';
			generatrice.os << "};\n";
			/* on utilise strlen pour être sûr d'avoir la bonne taille à cause
			 * des caractères échappés */
			generatrice.os << nom_chaine << ".taille=strlen(" << nom_chaine << ".pointeur);\n";
			b->valeur_calculee = nom_chaine;
			break;
		}
		case type_noeud::BOOLEEN:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
										   : (b->chaine() == "vrai");
			b->valeur_calculee = valeur ? dls::chaine("1") : dls::chaine("0");
			break;
		}
		case type_noeud::CARACTERE:
		{
			auto c = b->morceau.chaine[0];

			auto flux = dls::flux_chaine();

			flux << '\'';
			if (c == '\\') {
				flux << c << b->morceau.chaine[1];
			}
			else {
				flux << c;
			}

			flux << '\'';

			b->valeur_calculee = dls::chaine(flux.chn());
			break;
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();

			if (!expr_gauche) {
				auto enfant1 = *iter_enfant++;
				auto enfant2 = *iter_enfant++;
				auto enfant3 = *iter_enfant++;

				genere_code_C(enfant1, generatrice, contexte, false);
				genere_code_C(enfant2->enfants.front(), generatrice, contexte, false);
				genere_code_C(enfant3->enfants.front(), generatrice, contexte, false);

				auto flux = dls::flux_chaine();

				if (b->type == type_noeud::SAUFSI) {
					flux << '!';
				}

				flux << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
				flux << " ? ";
				/* prenons les enfants des enfants pour ne mettre des accolades
				 * autour de l'expression vu qu'ils sont de type 'BLOC' */
				flux << std::any_cast<dls::chaine>(enfant2->enfants.front()->valeur_calculee);
				flux << " : ";
				flux << std::any_cast<dls::chaine>(enfant3->enfants.front()->valeur_calculee);

				auto nom_variable = "__var_temp" + dls::vers_chaine(index++);

				generatrice.declare_variable(
							enfant2->enfants.front()->index_type,
							nom_variable,
							flux.chn());

				enfant1->valeur_calculee = nom_variable;

				return;
			}

			/* noeud 1 : condition */
			auto enfant1 = *iter_enfant++;

			genere_code_C(enfant1, generatrice, contexte, false);

			generatrice.os << "if (";

			if (b->type == type_noeud::SAUFSI) {
				generatrice.os << '!';
			}

			generatrice.os << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
			generatrice.os << ")";

			/* noeud 2 : bloc */
			auto enfant2 = *iter_enfant++;
			genere_code_C(enfant2, generatrice, contexte, false);

			/* noeud 3 : sinon (optionel) */
			if (nombre_enfants == 3) {
				generatrice.os << "else ";
				auto enfant3 = *iter_enfant++;
				genere_code_C(enfant3, generatrice, contexte, false);
			}

			break;
		}
		case type_noeud::BLOC:
		{
			contexte.empile_nombre_locales();

			auto dernier_enfant = static_cast<base *>(nullptr);

			generatrice.os << "{\n";

			for (auto enfant : b->enfants) {
				genere_code_C(enfant, generatrice, contexte, true);

				dernier_enfant = enfant;

				if (enfant->type == type_noeud::RETOUR) {
					break;
				}

				if (enfant->type == type_noeud::OPERATION_BINAIRE) {
					/* les assignations opérées (+=, etc) n'ont pas leurs codes
					 * générées via genere_code_C  */
					if (est_assignation_operee(enfant->identifiant())) {
						generatrice.os << std::any_cast<dls::chaine>(enfant->valeur_calculee);
						generatrice.os << ";\n";
					}
				}
			}

			if (dernier_enfant != nullptr && dernier_enfant->type != type_noeud::RETOUR) {
				/* génère le code pour tous les noeuds différés de ce bloc */
				auto noeuds = contexte.noeuds_differes_bloc();

				while (!noeuds.est_vide()) {
					auto n = noeuds.back();
					genere_code_C(n, generatrice, contexte, false);
					noeuds.pop_back();
				}
			}

			generatrice.os << "}\n";

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

			auto genere_code_tableau_chaine = [&generatrice](
					dls::flux_chaine &os_loc,
					ContexteGenerationCode &contexte_loc,
					base *enfant_1,
					base *enfant_2,
					DonneesTypeFinal::type_plage dt,
					dls::chaine const &nom_var)
			{
				auto var = enfant_1;
				auto idx = static_cast<noeud::base *>(nullptr);

				if (enfant_1->morceau.identifiant == id_morceau::VIRGULE) {
					var = enfant_1->enfants.front();
					idx = enfant_1->enfants.back();
				}

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, generatrice, contexte_loc, true);

				os_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= ";
				os_loc << std::any_cast<dls::chaine>(enfant_2->valeur_calculee);
				os_loc << ".taille - 1; ++"<< nom_var <<") {\n";
				contexte_loc.magasin_types.converti_type_C(contexte_loc, "", dt, os_loc);
				os_loc << " *" << broye_chaine(var) << " = &";
				os_loc << std::any_cast<dls::chaine>(enfant_2->valeur_calculee);
				os_loc << ".pointeur["<< nom_var <<"];\n";

				if (idx) {
					os_loc << "int " << broye_chaine(idx) << " = " << nom_var << ";\n";
				}
			};

			auto genere_code_tableau_fixe = [&generatrice](
					dls::flux_chaine &os_loc,
					ContexteGenerationCode &contexte_loc,
					base *enfant_1,
					base *enfant_2,
					DonneesTypeFinal::type_plage dt,
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

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, generatrice, contexte_loc, true);
				contexte_loc.magasin_types.converti_type_C(contexte_loc, "", dt, os_loc);
				os_loc << " *" << broye_chaine(var) << " = &";
				os_loc << std::any_cast<dls::chaine>(enfant_2->valeur_calculee);
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
						generatrice.os << "int " << broye_chaine(idx) << " = 0;\n";
					}

					genere_code_C(enfant2->enfants.front(), generatrice, contexte, false);
					genere_code_C(enfant2->enfants.back(), generatrice, contexte, false);

					generatrice.os << "for (";
					contexte.magasin_types.converti_type_C(
								contexte,
								"",
								type_debut.plage(),
								generatrice.os);
					generatrice.os << " " << nom_broye << " = ";
					generatrice.os << std::any_cast<dls::chaine>(enfant2->enfants.front()->valeur_calculee);

					generatrice.os << "; "
					   << nom_broye << " <= ";
					generatrice.os << std::any_cast<dls::chaine>(enfant2->enfants.back()->valeur_calculee);

					generatrice.os <<"; ++" << nom_broye;

					if (idx != nullptr) {
						generatrice.os << ", ++" << broye_chaine(idx);
					}

					generatrice.os  << ") {\n";

					if (b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
						auto donnees_var = DonneesVariable{};

						donnees_var.index_type = var->index_type;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.index_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						auto donnees_var = DonneesVariable{};
						donnees_var.index_type = index_type;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					break;
				}
				case GENERE_BOUCLE_TABLEAU:
				case GENERE_BOUCLE_TABLEAU_INDEX:
				{
					auto nom_var = "__i" + dls::vers_chaine(b);
					contexte.magasin_chaines.pousse(nom_var);

					auto donnees_var = DonneesVariable{};
					donnees_var.index_type = contexte.magasin_types[TYPE_Z32];

					contexte.pousse_locale(contexte.magasin_chaines.back(), donnees_var);

					if ((type & 0xff) == id_morceau::TABLEAU) {
						auto const taille_tableau = static_cast<uint64_t>(type >> 8);

						if (taille_tableau != 0) {
							genere_code_tableau_fixe(generatrice.os, contexte, enfant1, enfant2, type_debut.dereference(), nom_var, taille_tableau);
						}
						else {
							genere_code_tableau_chaine(generatrice.os, contexte, enfant1, enfant2, type_debut.dereference(), nom_var);
						}
					}
					else if (type == id_morceau::CHAINE) {
						auto dt = DonneesTypeFinal(id_morceau::Z8);
						index_type = contexte.magasin_types[TYPE_Z8];
						genere_code_tableau_chaine(generatrice.os, contexte, enfant1, enfant2, dt.plage(), nom_var);
					}

					if (b->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
						auto var = enfant1->enfants.front();
						auto idx = enfant1->enfants.back();

						donnees_var.index_type = var->index_type;
						donnees_var.drapeaux = BESOIN_DEREF;

						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.index_type = idx->index_type;
						donnees_var.drapeaux = 0;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						donnees_var.index_type = index_type;
						donnees_var.drapeaux = BESOIN_DEREF;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					auto nom_etat = "__etat" + dls::vers_chaine(enfant2);

					generatrice.os << "__etat_coro" << enfant2->df->nom_broye << " " << nom_etat << ";\n";
					generatrice.os << nom_etat << ".__reprend_coro = 0;\n";
					generatrice.os << nom_etat << ".__termine_coro = 0;\n";

					/* À FAIRE : utilisation du type */
					auto df = enfant2->df;
					auto nombre_vars_ret = df->idx_types_retours.taille();

					auto feuilles = dls::tableau<base *>{};
					rassemble_feuilles(enfant1, feuilles);

					auto idx = static_cast<noeud::base *>(nullptr);
					auto nom_idx = dls::chaine{};

					if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
						idx = feuilles.back();
						nom_idx = "__idx" + dls::vers_chaine(b);
						generatrice.os << "int " << nom_idx << " = 0;";
					}

					generatrice.os << "while (1) {\n";
					genere_code_C(enfant2, generatrice, contexte, true);

					generatrice.os << "if (" << nom_etat << ".__termine_coro == 1) { break; }\n";

					for (auto i = 0l; i < nombre_vars_ret; ++i) {
						auto f = feuilles[i];
						auto nom_var_broye = broye_chaine(f);

						contexte.magasin_types.converti_type_C(
									contexte,
									nom_var_broye,
									type_debut.plage(),
									generatrice.os);

						generatrice.os << ';'
						   << nom_var_broye
						   << " = " << nom_etat
						   << '.' << df->noms_retours[i] << ";\n";

						auto donnees_var = DonneesVariable{};
						donnees_var.index_type = df->idx_types_retours[i];
						contexte.pousse_locale(f->chaine(), donnees_var);
					}

					if (idx) {
						generatrice.os << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
						generatrice.os << nom_idx << " += 1;";

						auto donnees_var = DonneesVariable{};
						donnees_var.index_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
				}
			}

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);
			auto goto_brise = "__boucle_pour_brise" + dls::vers_chaine(b);

			contexte.empile_goto_continue(enfant1->chaine(), goto_continue);
			contexte.empile_goto_arrete(enfant1->chaine(), (enfant_sinon != nullptr) ? goto_brise : goto_apres);

			genere_code_C(enfant3, generatrice, contexte, false);

			generatrice.os << goto_continue << ":;\n";
			generatrice.os << "}\n";

			if (enfant_sans_arret) {
				genere_code_C(enfant_sans_arret, generatrice, contexte, false);
				generatrice.os << "goto " << goto_apres << ";";
			}

			if (enfant_sinon) {
				generatrice.os << goto_brise << ":;\n";
				genere_code_C(enfant_sinon, generatrice, contexte, false);
			}

			generatrice.os << goto_apres << ":;\n";

			contexte.depile_goto_arrete();
			contexte.depile_goto_continue();

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::CONTINUE_ARRETE:
		{
			auto chaine_var = b->enfants.est_vide() ? dls::vue_chaine_compacte{""} : b->enfants.front()->chaine();

			auto label_goto = (b->morceau.identifiant == id_morceau::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			generatrice.os << "goto " << label_goto << ";\n";
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

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			generatrice.os << "while (1) {\n";

			genere_code_C(enfant1, generatrice, contexte, false);

			generatrice.os << goto_continue << ":;\n";
			generatrice.os << "}\n";
			generatrice.os << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case type_noeud::REPETE:
		{
			auto iter = b->enfants.debut();
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			/* création des blocs */

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			generatrice.os << "while (1) {\n";
			genere_code_C(enfant1, generatrice, contexte, false);
			generatrice.os << goto_continue << ":;\n";
			genere_code_C(enfant2, generatrice, contexte, false);
			generatrice.os << "if (!";
			generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
			generatrice.os << ") {\nbreak;\n}\n";
			generatrice.os << "}\n";
			generatrice.os << goto_apres << ":;\n";

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

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			/* NOTE : la prépasse est susceptible de générer le code d'un appel
			 * de fonction, donc nous utilisons
			 * while (1) { if (!cond) { break; } }
			 * au lieu de
			 * while (cond) {}
			 * pour être sûr que la fonction est appelée à chaque boucle.
			 */
			generatrice.os << "while (1) {";
			genere_code_C(enfant1, generatrice, contexte, false);
			generatrice.os << "if (!";
			generatrice.os << std::any_cast<dls::chaine>(enfant1->valeur_calculee);
			generatrice.os << ") { break; }\n";

			genere_code_C(enfant2, generatrice, contexte, false);

			generatrice.os << goto_continue << ":;\n";
			generatrice.os << "}\n";
			generatrice.os << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case type_noeud::TRANSTYPE:
		{
			auto enfant = b->enfants.front();
			auto const &index_type_de = enfant->index_type;

			genere_code_C(enfant, generatrice, contexte, false);

			if (index_type_de == b->index_type) {
				b->valeur_calculee = enfant->valeur_calculee;
				return;
			}

			auto const &dt = contexte.magasin_types.donnees_types[b->index_type];

			auto flux = dls::flux_chaine();

			flux << "(";
			contexte.magasin_types.converti_type_C(contexte, "", dt.plage(), flux);
			flux << ")(";
			flux << std::any_cast<dls::chaine>(enfant->valeur_calculee);
			flux << ")";

			b->valeur_calculee = dls::chaine(flux.chn());

			break;
		}
		case type_noeud::NUL:
		{
			b->valeur_calculee = dls::chaine("0");
			break;
		}
		case type_noeud::TAILLE_DE:
		{
			auto index_dt = std::any_cast<long>(b->valeur_calculee);
			auto const &donnees = contexte.magasin_types.donnees_types[index_dt];
			b->valeur_calculee = dls::vers_chaine(taille_type_octet(contexte, donnees));
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
			genere_code_C(b->enfants.front(), generatrice, contexte, false);
			contexte.non_sur(false);
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
					enfant->valeur_calculee = cree_eini(contexte, generatrice, generatrice.os, enfant);
				}
				else {
					genere_code_C(enfant, generatrice, contexte, false);
				}
			}

			/* alloue un tableau fixe */
			auto dt_tfixe = DonneesTypeFinal{};
			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(taille_tableau << 8));
			dt_tfixe.pousse(type);

			auto nom_tableau_fixe = dls::chaine("__tabl_fix")
					.append(dls::vers_chaine(reinterpret_cast<long>(b)));

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_fixe, dt_tfixe.plage(), generatrice.os);

			generatrice.os << " = ";

			auto virgule = '{';

			for (auto enfant : b->enfants) {
				generatrice.os << virgule;
				generatrice.os << std::any_cast<dls::chaine>(enfant->valeur_calculee);
				virgule = ',';
			}

			generatrice.os << "};\n";

			/* alloue un tableau dynamique */
			auto dt_tdyn = DonneesTypeFinal{};
			dt_tdyn.pousse(id_morceau::TABLEAU);
			dt_tdyn.pousse(type);

			auto nom_tableau_dyn = dls::chaine("__tabl_dyn")
					.append(dls::vers_chaine(b));

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_dyn, dt_tdyn.plage(), generatrice.os);

			generatrice.os << ";\n";
			generatrice.os << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			generatrice.os << nom_tableau_dyn << ".taille = " << taille_tableau << ";\n";

			b->valeur_calculee = nom_tableau_dyn;
			break;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			auto liste_params = std::any_cast<dls::tableau<dls::vue_chaine_compacte>>(&b->valeur_calculee);

			for (auto enfant : b->enfants) {
				genere_code_C(enfant, generatrice, contexte, false);
			}

			auto flux = dls::flux_chaine();

			auto enfant = b->enfants.debut();
			auto nom_param = liste_params->debut();
			auto virgule = '{';

			for (auto i = 0l; i < liste_params->taille(); ++i) {
				flux << virgule;

				flux << '.' << broye_nom_simple(*nom_param) << '=';
				flux << std::any_cast<dls::chaine>((*enfant)->valeur_calculee);
				++enfant;
				++nom_param;

				virgule = ',';
			}

			flux << '}';

			b->valeur_calculee = dls::chaine(flux.chn());

			break;
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			auto nom_tableau = "__tabl" + dls::vers_chaine(b);
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];

			dls::tableau<base *> feuilles;
			rassemble_feuilles(b->enfants.front(), feuilles);

			for (auto f : feuilles) {
				genere_code_C(f, generatrice, contexte, false);
			}

			contexte.magasin_types.converti_type_C(
						contexte,
						nom_tableau,
						dt.plage(),
						generatrice.os);

			generatrice.os << " = ";

			auto virgule = '{';

			for (auto f : feuilles) {
				generatrice.os << virgule;
				generatrice.os << std::any_cast<dls::chaine>(f->valeur_calculee);
				virgule = ',';
			}

			generatrice.os << "};\n";

			b->valeur_calculee = nom_tableau;
			break;
		}
		case type_noeud::INFO_DE:
		{
			auto enfant = b->enfants.front();
			auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];
			b->valeur_calculee = "&" + dt.ptr_info_type;
			break;
		}
		case type_noeud::MEMOIRE:
		{
			genere_code_C(b->enfants.front(), generatrice, contexte, false);
			b->valeur_calculee = "*(" + std::any_cast<dls::chaine>(b->enfants.front()->valeur_calculee) + ")";
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
				auto expr = b->type_declare.expressions[0];
				performe_validation_semantique(expr, contexte);

				a_pointeur = true;
				auto nom_ptr = "__ptr" + dls::vers_chaine(b);
				auto nom_tabl = "__tabl" + dls::vers_chaine(b);
				auto taille_tabl = "__taille_tabl" + dls::vers_chaine(b);

				genere_code_C(expr, generatrice, contexte, false);

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							taille_tabl,
							std::any_cast<dls::chaine>(expr->valeur_calculee));

				auto flux = dls::flux_chaine();
				flux << "sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.dereference(),
							flux);
				flux << ") * " << taille_tabl;

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							nom_taille,
							flux.chn());

				auto dt_ptr = DonneesTypeFinal{};
				dt_ptr.pousse(id_morceau::POINTEUR);
				dt_ptr.pousse(dt.dereference());

				auto expr_m = generatrice.expression_malloc(dt_ptr, nom_taille);
				generatrice.declare_variable(dt_ptr, nom_ptr, expr_m);

				generatrice.declare_variable(dt, nom_tabl, "");
				generatrice.os << nom_tabl << ".pointeur = " << nom_ptr << ";\n";
				generatrice.os << nom_tabl << ".taille = " << taille_tabl << ";\n";

				nom_ptr_ret = nom_tabl;
			}
			else if (dt.type_base() == id_morceau::CHAINE) {
				a_pointeur = true;
				auto nom_ptr = "__ptr" + dls::vers_chaine(b);
				auto nom_chaine = "__chaine" + dls::vers_chaine(b);

				auto enf = *enfant++;

				/* Prépasse pour les accès de membres dans l'expression. */
				genere_code_C(enf, generatrice, contexte, false);

				auto flux = dls::flux_chaine();

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							nom_taille,
							std::any_cast<dls::chaine>(enf->valeur_calculee));

				nombre_enfant -= 1;

				generatrice.os << "char *" << nom_ptr << " = (char *)(malloc(sizeof(char) * (";
				generatrice.os << nom_taille << ")));\n";

				generatrice.os << "chaine " << nom_chaine << ";\n";
				generatrice.os << nom_chaine << ".pointeur = " << nom_ptr << ";\n";
				generatrice.os << nom_chaine << ".taille = " << nom_taille << ";\n";

				nom_ptr_ret = nom_chaine;
			}
			else {
				auto const dt_deref = dt.dereference();
				generatrice.os << "long " << nom_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_deref,
							generatrice.os);
				generatrice.os << ");\n";
				auto nom_ptr = "__ptr" + dls::vers_chaine(b);

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.plage(),
							generatrice.os);

				generatrice.os << " " << nom_ptr << " = (";

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.plage(),
							generatrice.os);

				generatrice.os << ")(malloc(" << nom_taille << "));\n";

				/* initialise la structure */
				if ((dt_deref.front() & 0xff) == id_morceau::CHAINE_CARACTERE) {
					cree_initialisation(
								contexte,
								generatrice,
								dt_deref,
								nom_ptr,
								"->",
								generatrice.os);
				}

				nom_ptr_ret = nom_ptr;
			}

			auto bloc_sinon = static_cast<base *>(nullptr);

			if (nombre_enfant == 1) {
				bloc_sinon = *enfant++;
			}

			genere_code_echec_logement(
						contexte,
						generatrice,
						nom_ptr_ret,
						a_pointeur,
						b,
						bloc_sinon);

			generatrice.os << "__VG_memoire_utilisee__ += " << nom_taille << ";\n";
			b->valeur_calculee = nom_ptr_ret;

			break;
		}
		case type_noeud::DELOGE:
		{
			auto enfant = b->enfants.front();
			auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];
			auto nom_taille = "__taille_allouee" + dls::vers_chaine(index++);

			genere_code_C(enfant, generatrice, contexte, true);
			auto chn_enfant = std::any_cast<dls::chaine>(enfant->valeur_calculee);

			if (dt.type_base() == id_morceau::TABLEAU || dt.type_base() == id_morceau::CHAINE) {

				generatrice.os << "long " << nom_taille << " = ";
				generatrice.os << chn_enfant << ".taille";

				if (dt.type_base() == id_morceau::TABLEAU) {
					generatrice.os << " * sizeof(";
					contexte.magasin_types.converti_type_C(
								contexte,
								"",
								dt.dereference(),
								generatrice.os);
					generatrice.os << ")";
				}

				generatrice.os << ";\n";

				generatrice.os << "free(" << chn_enfant << ".pointeur);\n";
				generatrice.os << chn_enfant << ".pointeur = 0;\n";
				generatrice.os << chn_enfant << ".taille = 0;\n";
			}
			else {
				generatrice.os << "long " << nom_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.dereference(),
							generatrice.os);
				generatrice.os << ");\n";

				generatrice.os << "free(" << chn_enfant << ");\n";
				generatrice.os << chn_enfant << " = 0;\n";
			}

			generatrice.os << "__VG_memoire_utilisee__ -= " << nom_taille << ";\n";

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
				a_pointeur = true;
				auto expr = b->type_declare.expressions[0];
				performe_validation_semantique(expr, contexte);

				auto taille_tabl = "__taille_tabl" + dls::vers_chaine(b);

				genere_code_C(expr, generatrice, contexte, false);
				genere_code_C(enfant1, generatrice, contexte, true);

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							taille_tabl,
							std::any_cast<dls::chaine>(expr->valeur_calculee));

				nom_ptr_ret = std::any_cast<dls::chaine>(enfant1->valeur_calculee);
				auto acces_taille = nom_ptr_ret + ".taille";
				auto acces_pointeur = nom_ptr_ret + ".pointeur";

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							nom_ancienne_taille,
							acces_taille);

				auto flux_type = dls::flux_chaine();
				auto dt_ptr = DonneesTypeFinal{};
				dt_ptr.pousse(id_morceau::POINTEUR);
				dt_ptr.pousse(dt_pointeur.dereference());

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_ptr.plage(),
							flux_type);

				generatrice.os << "long " << nom_nouvelle_taille << " = sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_pointeur.dereference(),
							generatrice.os);
				generatrice.os << ") * " << taille_tabl << ";\n";
				generatrice.os << ";\n";

				generatrice.os << acces_pointeur << " = (" << flux_type.chn() << ")(realloc(";
				generatrice.os << acces_pointeur << ", " << nom_nouvelle_taille << "));\n";
				generatrice.os << acces_taille << " = " << taille_tabl << ";\n";
			}
			else if (dt_pointeur.type_base() == id_morceau::CHAINE) {
				auto enfant2 = *enfant++;
				nombre_enfant -= 1;
				a_pointeur = true;

				genere_code_C(enfant1, generatrice, contexte, true);
				nom_ptr_ret = std::any_cast<dls::chaine>(enfant1->valeur_calculee);
				genere_code_C(enfant2, generatrice, contexte, true);

				auto acces_taille = nom_ptr_ret + ".taille";
				auto acces_pointeur = nom_ptr_ret + ".pointeur";

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							nom_ancienne_taille,
							acces_taille);

				generatrice.os << "long " << nom_nouvelle_taille << " = sizeof(char) *";
				generatrice.os << std::any_cast<dls::chaine>(enfant2->valeur_calculee);
				generatrice.os << ";\n";

				generatrice.os << acces_pointeur << " = (char *)(realloc(";
				generatrice.os << acces_pointeur << ", " << nom_nouvelle_taille<< "));\n";
				generatrice.os << acces_taille << " = " << nom_nouvelle_taille << ";\n";
			}
			else {
				auto flux_type = dls::flux_chaine();
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_pointeur.dereference(),
							flux_type);

				auto chn_type_deref = flux_type.chn();

				auto expr = "sizeof(" + chn_type_deref + ")";

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							nom_ancienne_taille,
							expr);

				generatrice.declare_variable(
							contexte.magasin_types[TYPE_Z64],
							nom_nouvelle_taille,
							nom_ancienne_taille);

				genere_code_C(enfant1, generatrice, contexte, true);
				nom_ptr_ret = std::any_cast<dls::chaine>(enfant1->valeur_calculee);

				flux_type.chn("");
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_pointeur.plage(),
							flux_type);

				generatrice.os << nom_ptr_ret;
				generatrice.os << " = (" << flux_type.chn();
				generatrice.os << ")(realloc(";
				generatrice.os << nom_ptr_ret;
				generatrice.os << ", sizeof(" << chn_type_deref << ")));\n";
			}

			auto bloc_sinon = static_cast<base *>(nullptr);

			if (nombre_enfant == 2) {
				bloc_sinon = *enfant++;
			}

			genere_code_echec_logement(
						contexte,
						generatrice,
						nom_ptr_ret,
						a_pointeur,
						b,
						bloc_sinon);

			generatrice.os << "__VG_memoire_utilisee__ += " << nom_nouvelle_taille
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
				auto enf0 = enfant->enfants.front();
				auto enf1 = enfant->enfants.back();
				auto nom_valeur = broye_chaine(b) + '_' + broye_chaine(enf0);

				genere_code_C(enf1, generatrice, contexte, false);

				generatrice.os << "static const ";
				generatrice.declare_variable(dt, nom_valeur, std::any_cast<dls::chaine>(enf1->valeur_calculee));
			}

			break;
		}
		case type_noeud::ASSOCIE:
		{
			/* le premier enfant est l'expression, les suivants les paires */

			auto nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_expr = std::any_cast<dls::chaine>(expression->valeur_calculee);

			for (auto i = 1l; i < nombre_enfants; ++i) {
				auto paire = *iter_enfant++;
				auto enf0 = paire->enfants.front();
				auto enf1 = paire->enfants.back();

				genere_code_C(enf0, generatrice, contexte, true);

				generatrice.os << "if (";
				generatrice.os << chaine_expr;
				generatrice.os << " == ";
				generatrice.os << std::any_cast<dls::chaine>(enf0->valeur_calculee);
				generatrice.os << ") ";
				genere_code_C(enf1, generatrice, contexte, false);

				if (i < nombre_enfants - 1) {
					generatrice.os << "else {\n";
				}
			}

			/* il faut fermer tous les blocs else */
			for (auto i = 1l; i < nombre_enfants - 1; ++i) {
				generatrice.os << "}\n";
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

				genere_code_C(f, generatrice, contexte, true);

				generatrice.os << "__etat->" << df->noms_retours[i] << " = ";
				generatrice.os << std::any_cast<dls::chaine>(f->valeur_calculee);
				generatrice.os << ";\n";
			}

			auto debut = contexte.debut_locales();
			auto fin   = contexte.fin_locales();

			for (; debut != fin; ++debut) {
				if (debut->second.est_argument) {
					continue;
				}

				auto nom_broye = broye_nom_simple(debut->first);
				generatrice.os << "__etat->" << nom_broye << " = " << nom_broye << ";\n";
			}

			generatrice.os << "__etat->__reprend_coro = " << donnees_coroutine.nombre_retenues << ";\n";
			generatrice.os << "return;\n";
			generatrice.os << "__reprend_coro" << donnees_coroutine.nombre_retenues << ":\n";

			debut = contexte.debut_locales();

			for (; debut != fin; ++debut) {
				if (debut->second.est_argument) {
					continue;
				}

				auto dt_enf = contexte.magasin_types.donnees_types[debut->second.index_type].plage();

				/* À FAIRE : trouve une manière de restaurer les tableaux fixes */
				if (est_type_tableau_fixe(dt_enf)) {
					continue;
				}

				auto nom_broye = broye_nom_simple(debut->first);
				generatrice.os << nom_broye << " = __etat->" << nom_broye << ";\n";
			}

			break;
		}
	}
}

void genere_code_C(
		base *b,
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os,
		dls::flux_chaine &os_init)
{
	if (b->type != type_noeud::RACINE) {
		return;
	}

	auto generatrice = GeneratriceCodeC(contexte, os);

	auto temps_validation = 0.0;
	auto temps_generation = 0.0;

	for (auto noeud : b->enfants) {
		auto debut_validation = dls::chrono::compte_seconde();
		performe_validation_semantique(noeud, contexte);
		contexte.magasin_chaines.efface();
		temps_validation += debut_validation.temps();
	}

	declare_structures_C(contexte, os);

	auto debut_generation = dls::chrono::compte_seconde();
	/* Crée les infos types pour tous les types connus.
	 * À FAIRE : évite de créer ceux qui ne sont pas utiles */
	for (auto &dt : contexte.magasin_types.donnees_types) {
		cree_info_type_C(contexte, generatrice, os, os_init, dt);
	}
	temps_generation += debut_generation.temps();

	/* génère le code */
	for (auto noeud : b->enfants) {
		debut_generation.commence();
		genere_code_C(noeud, generatrice, contexte, false);
		contexte.magasin_chaines.efface();
		temps_generation += debut_generation.temps();
	}

	contexte.temps_generation = temps_generation;
	contexte.temps_validation = temps_validation;
}

}  /* namespace noeud */
