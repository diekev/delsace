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
#include "conversion_type_c.hh"
#include "erreur.h"
#include "generatrice_code_c.hh"
#include "modules.hh"
#include "outils_morceaux.hh"
#include "typeuse.hh"
#include "validation_semantique.hh"

using denombreuse = lng::decoupeuse_nombre<id_morceau>;

namespace noeud {

/* ************************************************************************** */

/* À tenir synchronisé avec l'énum dans info_type.kuri
 * Nous utilisons ceci lors de la génération du code des infos types car nous ne
 * générons pas de code (ou symboles) pour les énums, mais prenons directements
 * leurs valeurs.
 */
struct IDInfoType {
	dls::chaine ENTIER    = "0";
	dls::chaine REEL      = "1";
	dls::chaine BOOLEEN   = "2";
	dls::chaine CHAINE    = "3";
	dls::chaine POINTEUR  = "4";
	dls::chaine STRUCTURE = "5";
	dls::chaine FONCTION  = "6";
	dls::chaine TABLEAU   = "7";
	dls::chaine EINI      = "8";
	dls::chaine RIEN      = "9";
	dls::chaine ENUM      = "10";
};

static auto chaine_valeur_enum(
		DonneesStructure const &ds,
		dls::vue_chaine_compacte const &nom)
{
	auto &dm = ds.donnees_membres.trouve(nom)->second;

	if (dm.resultat_expression.type == type_expression::ENTIER) {
		return dls::vers_chaine(dm.resultat_expression.entier);
	}

	return dls::vers_chaine(dm.resultat_expression.reel);
}

/* ************************************************************************** */

/* À FAIRE : trouve une bonne manière de générer des noms uniques. */
static int index = 0;

void genere_code_C(
		base *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche);

static void applique_transformation(
		base *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	genere_code_C(b, generatrice, contexte, expr_gauche);

	auto nom_courant = b->chaine_calculee();
	auto &dt = contexte.typeuse[b->index_type];

	auto &os = generatrice.os;

	auto nom_var_temp = "__var_temp_conv" + dls::vers_chaine(index++);

	switch (b->transformation.type) {
		default:
		case TypeTransformation::INUTILE:
		case TypeTransformation::PREND_PTR_RIEN:
		case TypeTransformation::AUGMENTE_TAILLE_TYPE:
		{
			nom_var_temp = nom_courant;
			break;
		}
		case TypeTransformation::CONSTRUIT_EINI:
		{
			/* dans le cas d'un nombre ou d'un tableau, etc. */
			if (b->type != type_noeud::VARIABLE) {
				generatrice.declare_variable(dt, nom_var_temp, b->chaine_calculee());
				nom_courant = nom_var_temp;
				nom_var_temp = "__var_temp_eini" + dls::vers_chaine(index++);
			}

			os << "Kseini " << nom_var_temp << " = {\n";
			os << "\t.pointeur = &" << nom_courant << ",\n";
			os << "\t.info = (InfoType *)(&" << dt.ptr_info_type << ")\n";
			os << "};\n";

			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			os << nom_broye_type(contexte, dt) << " " << nom_var_temp << " = *(";
			os << nom_broye_type(contexte, dt) << " *)(" << nom_courant << ".pointeur);\n";
			break;
		}
		case TypeTransformation::CONSTRUIT_TABL_OCTET:
		{
			auto type_base = dt.type_base();

			os << "KtKsoctet " << nom_var_temp << " = {\n";

			switch (type_base & 0xff) {
				default:
				{
					os << "\t.pointeur = (unsigned char *)(&" << nom_courant << "),\n";
					os << "\t.taille = sizeof(" << nom_broye_type(contexte, dt) << ")\n";
					break;
				}
				case id_morceau::POINTEUR:
				{
					os << "\t.pointeur = (unsigned char *)(" << nom_courant << "),\n";
					os << "\t.taille = sizeof(";
					auto index = contexte.typeuse.ajoute_type(dt.dereference());
					os << nom_broye_type(contexte, index) << ")\n";
					break;
				}
				case id_morceau::CHAINE:
				{
					os << "\t.pointeur = " << nom_courant << ".pointeur,\n";
					os << "\t.taille = " << nom_courant << ".taille,\n";
					break;
				}
				case id_morceau::TABLEAU:
				{
					auto taille = static_cast<int>(type_base >> 8);

					if (taille == 0) {
						os << "\t.pointeur = (unsigned char *)(&" << nom_courant << ".pointeur),\n";
						os << "\t.taille = " << nom_courant << ".taille * sizeof(";
						auto index = contexte.typeuse.ajoute_type(dt.dereference());
						os << nom_broye_type(contexte, index) << ")\n";
					}
					else {
						os << "\t.pointeur = " << nom_courant << ".pointeur,\n";
						os << "\t.taille = " << taille << " * sizeof(";
						auto index = contexte.typeuse.ajoute_type(dt.dereference());
						os << nom_broye_type(contexte, index) << ")\n";
					}

					break;
				}
			}

			os << "};\n";

			break;
		}
		case TypeTransformation::EXTRAIT_TABL_OCTET:
		{
			/* À FAIRE */
			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			auto index = contexte.typeuse.ajoute_type(dt.dereference());
			index = contexte.typeuse.type_tableau_pour(index);
			os << nom_broye_type(contexte, index) << ' ' << nom_var_temp << " = {\n";
			os << "\t.taille = " << static_cast<size_t>(dt.type_base() >> 8) << ",\n";
			os << "\t.pointeur = " << nom_courant << "\n";
			os << "};";

			break;
		}
		case TypeTransformation::CONSTRUIT_EINI_TABLEAU:
		{
			auto index = contexte.typeuse.ajoute_type(dt.dereference());
			index = contexte.typeuse.type_tableau_pour(index);
			os << nom_broye_type(contexte, index) << ' ' << nom_var_temp << " = {\n";
			os << "\t.taille = " << static_cast<size_t>(dt.type_base() >> 8) << ",\n";
			os << "\t.pointeur = " << nom_courant << "\n";
			os << "};";

			nom_courant = nom_var_temp;
			nom_var_temp = "__var_temp_tabl_eini" + dls::vers_chaine(index++);

			os << "Kseini " << nom_var_temp << " = {\n";
			os << "\t.pointeur = &" << nom_courant << ",\n";
			os << "\t.info = (InfoType *)(&" << dt.ptr_info_type << ")\n";
			os << "};\n";

			break;
		}
		case TypeTransformation::FONCTION:
		{
			/* À FAIRE : typage */
			os << nom_var_temp << " = " << b->transformation.nom_fonction << '(';
			os << nom_courant << ");n\n";

			break;
		}
		case TypeTransformation::PREND_REFERENCE:
		{
			nom_var_temp = "&(" + nom_courant + ")";
			break;
		}
		case TypeTransformation::DEREFERENCE:
		{
			if (expr_gauche) {
				nom_var_temp = "(*" + nom_courant + ")";
			}
			else {
				auto index = contexte.typeuse.ajoute_type(dt.dereference());
				os << nom_broye_type(contexte, index);
				os << ' ' << nom_var_temp << " = *(" << nom_courant << ");\n";
			}

			break;
		}
	}

	b->valeur_calculee = nom_var_temp;
}

static void genere_code_extra_pre_retour(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os)
{
	if (contexte.donnees_fonction->est_coroutine) {
		os << "__etat->__termine_coro = 1;\n";
		os << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		os << "pthread_cond_signal(&__etat->cond_boucle);\n";
		os << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
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

static auto cree_info_type_defaul_C(
		dls::flux_chaine &os_decl,
		dls::chaine const &id_type)
{
	auto nom_info_type = "__info_type" + id_type + dls::vers_chaine(index++);

	os_decl << "static const InfoType " << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_type << '\n';
	os_decl << "};\n";

	return nom_info_type;
}

static auto cree_info_type_entier_C(
		dls::flux_chaine &os_decl,
		int taille_en_octet,
		bool est_signe,
		IDInfoType const &id_info_type)
{
	auto nom_info_type = "__info_type_entier" + dls::vers_chaine(index++);

	os_decl << "static const InfoTypeEntier " << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.ENTIER << ",\n";
	os_decl << '\t' << broye_nom_simple(".est_signé = ") << est_signe << ",\n";
	os_decl << "\t.taille_en_octet = " << taille_en_octet << '\n';
	os_decl << "};\n";

	return nom_info_type;
}

static auto cree_info_type_reel_C(
		dls::flux_chaine &os_decl,
		int taille_en_octet,
		IDInfoType const &id_info_type)
{
	auto nom_info_type = "__info_type_reel" + dls::vers_chaine(index++);

	os_decl << "static const " << broye_nom_simple("InfoTypeRéel ") << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.REEL << ",\n";
	os_decl << "\t.taille_en_octet = " << taille_en_octet << '\n';
	os_decl << "};\n";

	return nom_info_type;
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		DonneesTypeFinal &donnees_type,
		IDInfoType const &id_info_type);

static auto cree_info_type_structure_C(
		dls::flux_chaine &os_decl,
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::vue_chaine_compacte const &nom_struct,
		DonneesStructure const &donnees_structure,
		DonneesTypeFinal &dt,
		IDInfoType const &id_info_type)
{
	if (dt.ptr_info_type != "") {
		return dt.ptr_info_type;
	}

	auto const nom_info_type = "__info_type_struct" + dls::vers_chaine(index++);

	auto const nom_broye = broye_nom_simple(nom_struct);

	/* prédéclare l'infotype au cas où la structure s'incluerait elle-même via
	 * un pointeur (p.e. listes chainées) */
	os_decl << "static const InfoTypeStructure " << nom_info_type << ";\n";

	/* met en place le 'pointeur' au cas où une structure s'incluerait elle-même
	 * via un pointeur */
	dt.ptr_info_type = nom_info_type;

	auto const nombre_membres = donnees_structure.index_types.taille();

	for (auto i = 0l; i < nombre_membres; ++i) {
		auto index_dt = donnees_structure.index_types[i];
		auto &dt_membre = contexte.typeuse[index_dt];

		for (auto paire_idx_mb : donnees_structure.donnees_membres) {
			if (paire_idx_mb.second.index_membre != i) {
				continue;
			}

			auto idx = contexte.typeuse.ajoute_type(dt_membre);
			auto &rderef = contexte.typeuse[idx];

			if (rderef.ptr_info_type == "") {
				rderef.ptr_info_type = cree_info_type_C(
							contexte, generatrice, os_decl, dt_membre, id_info_type);
			}

			break;
		}
	}

	/* crée le tableau des données des membres */
	auto const nom_tableau_membre = "__info_type_membres" + dls::vers_chaine(index++);
	os_decl << "static const InfoTypeMembreStructure " << nom_tableau_membre << "[] = {\n";

	for (auto i = 0l; i < nombre_membres; ++i) {
		auto index_dt = donnees_structure.index_types[i];
		auto &dt_membre = contexte.typeuse[index_dt];

		for (auto paire_idx_mb : donnees_structure.donnees_membres) {
			if (paire_idx_mb.second.index_membre != i) {
				continue;
			}

			auto idx = contexte.typeuse.ajoute_type(dt_membre);
			auto &rderef = contexte.typeuse[idx];

			os_decl << "\t{\n";
			os_decl << "\t\t.nom = { .pointeur = \"" << paire_idx_mb.first << "\", .taille = " << paire_idx_mb.first.taille() << " },\n";
			os_decl << "\t\t" << broye_nom_simple(".décalage = ") << paire_idx_mb.second.decalage << ",\n";
			os_decl << "\t\t.id = (InfoType *)(&" << rderef.ptr_info_type << ")\n";
			os_decl << "\t},\n";

			break;
		}
	}

	os_decl << "};\n";

	/* crée l'info pour la structure */
	os_decl << "static const InfoTypeStructure " << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.STRUCTURE << ",\n";
	os_decl << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
	os_decl << "\t.membres = {.pointeur = " << nom_tableau_membre << ", .taille = " << donnees_structure.index_types.taille() << "},\n";
	os_decl << "};\n";

	return nom_info_type;
}

static auto cree_info_type_enum_C(
		dls::flux_chaine &os_decl,
		dls::vue_chaine_compacte const &nom_struct,
		DonneesStructure const &donnees_structure,
		IDInfoType const &id_info_type)
{
	auto nom_broye = broye_nom_simple(nom_struct);

	/* crée un tableau pour les noms des énumérations */
	auto const nom_tableau_noms = "__tableau_noms_enum" + dls::vers_chaine(index++);

	os_decl << "static const chaine " << nom_tableau_noms << "[] = {\n";

	auto noeud_decl = donnees_structure.noeud_decl;
	auto nombre_enfants = noeud_decl->enfants.taille();
	for (auto enfant : noeud_decl->enfants) {
		auto enf0 = enfant;

		if (enf0->type == type_noeud::ASSIGNATION_VARIABLE) {
			enf0 = enf0->enfants.front();
		}

		os_decl << "\t{.pointeur=\"" << enf0->chaine() << "\", .taille=" << enf0->chaine().taille() << "},\n";
	}

	os_decl << "};\n";

	/* crée le tableau pour les valeurs */
	auto const nom_tableau_valeurs = "__tableau_valeurs_enum" + dls::vers_chaine(index++);

	os_decl << "static const int " << nom_tableau_valeurs << "[] = {\n\t";
	for (auto enfant : noeud_decl->enfants) {
		auto enf0 = enfant;

		if (enf0->type == type_noeud::ASSIGNATION_VARIABLE) {
			enf0 = enf0->enfants.front();
		}

		os_decl << chaine_valeur_enum(donnees_structure, enf0->chaine());
		os_decl << ',';
	}
	os_decl << "\n};\n";

	/* crée l'info type pour l'énum */
	auto const nom_info_type = "__info_type_enum" + dls::vers_chaine(index++);

	os_decl << "static const " << broye_nom_simple("InfoTypeÉnum ") << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.ENUM << ",\n";
	os_decl << "\t.nom = { .pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << " },\n";
	os_decl << "\t.noms = { .pointeur = " << nom_tableau_noms << ", .taille = " << nombre_enfants << " },\n ";
	os_decl << "\t.valeurs = { .pointeur = " << nom_tableau_valeurs << ", .taille = " << nombre_enfants << " },\n ";
	os_decl << "};\n";

	return nom_info_type;
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		DonneesTypeFinal &donnees_type,
		IDInfoType const &id_info_type)
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
			valeur = cree_info_type_defaul_C(os_decl, id_info_type.BOOLEEN);
			break;
		}
		case id_morceau::N8:
		{
			valeur = cree_info_type_entier_C(os_decl, 8, false, id_info_type);
			break;
		}
		case id_morceau::OCTET:
		case id_morceau::Z8:
		{
			valeur = cree_info_type_entier_C(os_decl, 8, true, id_info_type);
			break;
		}
		case id_morceau::N16:
		{
			valeur = cree_info_type_entier_C(os_decl, 16, false, id_info_type);
			break;
		}
		case id_morceau::Z16:
		{
			valeur = cree_info_type_entier_C(os_decl, 16, true, id_info_type);
			break;
		}
		case id_morceau::N32:
		{
			valeur = cree_info_type_entier_C(os_decl, 32, false, id_info_type);
			break;
		}
		case id_morceau::Z32:
		{
			valeur = cree_info_type_entier_C(os_decl, 32, true, id_info_type);
			break;
		}
		case id_morceau::N64:
		{
			valeur = cree_info_type_entier_C(os_decl, 64, false, id_info_type);
			break;
		}
		case id_morceau::N128:
		{
			valeur = cree_info_type_entier_C(os_decl, 128, false, id_info_type);
			break;
		}
		case id_morceau::Z64:
		{
			valeur = cree_info_type_entier_C(os_decl, 64, true, id_info_type);
			break;
		}
		case id_morceau::Z128:
		{
			valeur = cree_info_type_entier_C(os_decl, 128, true, id_info_type);
			break;
		}
		case id_morceau::R16:
		{
			valeur = cree_info_type_reel_C(os_decl, 16, id_info_type);
			break;
		}
		case id_morceau::R32:
		{
			valeur = cree_info_type_reel_C(os_decl, 32, id_info_type);
			break;
		}
		case id_morceau::R64:
		{
			valeur = cree_info_type_reel_C(os_decl, 64, id_info_type);
			break;
		}
		case id_morceau::R128:
		{
			valeur = cree_info_type_reel_C(os_decl, 128, id_info_type);
			break;
		}
		case id_morceau::REFERENCE:
		case id_morceau::POINTEUR:
		{
			auto deref = donnees_type.dereference();

			auto idx = contexte.typeuse.ajoute_type(deref);
			auto &rderef = contexte.typeuse[idx];

			if (rderef.ptr_info_type == "") {
				rderef.ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, rderef, id_info_type);
			}

			auto nom_info_type = "__info_type_pointeur" + dls::vers_chaine(index++);

			os_decl << "static const InfoTypePointeur " << nom_info_type << " = {\n";
			os_decl << ".id = " << id_info_type.POINTEUR << ",\n";
			os_decl << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << rderef.ptr_info_type << "),\n";
			os_decl << '\t' << broye_nom_simple(".est_référence = ")
					<< (donnees_type.type_base() == id_morceau::REFERENCE) << ",\n";
			os_decl << "};\n";

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
							contexte.nom_struct(id_structure),
							donnees_structure,
							id_info_type);

				donnees_type.ptr_info_type = valeur;
			}
			else {
				valeur = cree_info_type_structure_C(
							os_decl,
							contexte,
							generatrice,
							contexte.nom_struct(id_structure),
							donnees_structure,
							donnees_type,
							id_info_type);
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
				os_decl << "static const InfoTypeTableau " << nom_info_type << " = {\n";
				os_decl << "\t.id = " << id_info_type.TABLEAU << ",\n";
				os_decl << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
				os_decl << "};\n";
			}
			else {
				auto idx = contexte.typeuse.ajoute_type(deref);
				auto &rderef = contexte.typeuse[idx];

				if (rderef.ptr_info_type == "") {
					rderef.ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, rderef, id_info_type);
				}

				os_decl << "static const InfoTypeTableau " << nom_info_type << " = {\n";
				os_decl << "\t.id = " << id_info_type.TABLEAU << ",\n";
				os_decl << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << rderef.ptr_info_type << "),\n";
				os_decl << "};\n";
			}

			valeur = nom_info_type;
			break;
		}
		case id_morceau::COROUT:
		case id_morceau::FONC:
		{
			auto nom_info_type = "__info_type_FONCTION" + dls::vers_chaine(index++);

			donnees_type.ptr_info_type = nom_info_type;

			auto nombre_types_retour = 0l;
			auto dt_params = donnees_types_parametres(contexte.typeuse, donnees_type, nombre_types_retour);
			auto nombre_types_entree = dt_params.taille() - nombre_types_retour;

			for (auto i = 0; i < dt_params.taille(); ++i) {
				auto &dt_prm = contexte.typeuse[dt_params[i]];

				if (dt_prm.ptr_info_type != "") {
					continue;
				}

				cree_info_type_C(contexte, generatrice, os_decl, dt_prm, id_info_type);
			}

			/* crée tableau infos type pour les entrées */
			auto const nom_tableau_entrees = "__types_entree" + dls::vers_chaine(index++);

			os_decl << "static const InfoType *" << nom_tableau_entrees << "[] = {\n";

			for (auto i = 0; i < nombre_types_entree; ++i) {
				auto const &dt_prm = contexte.typeuse[dt_params[i]];
				os_decl << "(InfoType *)&" << dt_prm.ptr_info_type << ",\n";
			}

			os_decl << "};\n";

			/* crée tableau infos type pour les sorties */
			auto const nom_tableau_sorties = "__types_sortie" + dls::vers_chaine(index++);

			os_decl << "static const InfoType *" << nom_tableau_sorties << "[] = {\n";

			for (auto i = nombre_types_entree; i < dt_params.taille(); ++i) {
				auto const &dt_prm = contexte.typeuse[dt_params[i]];
				os_decl << "(InfoType *)&" << dt_prm.ptr_info_type << ",\n";
			}

			os_decl << "};\n";

			/* crée l'info type pour la fonction */
			os_decl << "static const InfoTypeFonction " << nom_info_type << " = {\n";
			os_decl << "\t.id = " << id_info_type.FONCTION << ",\n";
			os_decl << "\t.est_coroutine = " << (donnees_type.type_base() == id_morceau::COROUT) << ",\n";
			os_decl << broye_nom_simple("\t.types_entrée = { .pointeur = ") << nom_tableau_entrees;
			os_decl << ", .taille = " << nombre_types_entree << " },\n";
			os_decl << broye_nom_simple("\t.types_sortie = { .pointeur = ") << nom_tableau_sorties;
			os_decl << ", .taille = " << nombre_types_retour << " },\n";
			os_decl << "};\n";

			valeur = nom_info_type;
			break;
		}
		case id_morceau::EINI:
		{
			valeur = cree_info_type_defaul_C(os_decl, id_info_type.EINI);
			break;
		}
		case id_morceau::NUL: /* À FAIRE */
		case id_morceau::RIEN:
		{
			valeur = cree_info_type_defaul_C(os_decl, id_info_type.RIEN);
			break;
		}
		case id_morceau::CHAINE:
		{
			valeur = cree_info_type_defaul_C(os_decl, id_info_type.CHAINE);
			break;
		}
	}

	if (donnees_type.ptr_info_type == "") {
		donnees_type.ptr_info_type = valeur;
	}

	return valeur;
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
		applique_transformation(enf, generatrice, contexte, false);
	}

	auto &dt = contexte.typeuse[b->index_type];

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
		os << nom_broye_type(contexte, dt) << ' ' << nom_indirection << " = ";
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

		if (df->est_coroutine) {
			os << virgule << "&__etat" << dls::vers_chaine(b) << ");\n";
			return;
		}

		if (!df->est_externe && noeud_decl != nullptr && !dls::outils::possede_drapeau(noeud_decl->drapeaux, FORCE_NULCTX)) {
			os << virgule;
			os << "ctx";
			virgule = ',';
		}
	}
	else {
		if (!dls::outils::possede_drapeau(b->drapeaux, FORCE_NULCTX)) {
			os << virgule;
			os << "ctx";
			virgule = ',';
		}
	}

	for (auto enf : enfants) {
		os << virgule;
		os << enf->chaine_calculee();
		virgule = ',';
	}

	for (auto n : liste_var_retour) {
		os << virgule;
		os << "&(" << n->chaine_calculee() << ')';
		virgule = ',';
	}

	for (auto n : liste_noms_retour) {
		os << virgule << n;
		virgule = ',';
	}

	os << ");\n";
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
			auto dt_enf = contexte.typeuse[index_dt].plage();

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
					os << donnees_membre.noeud_decl->chaine_calculee();
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

		if (ds.est_union && !ds.est_nonsur) {
			os << chaine_parent << dls::chaine(accesseur) << "membre_actif = 0;\n";
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
		flux << membre->chaine_calculee();
	}
	else {
		auto const &index_type = structure->index_type;
		auto type_structure = contexte.typeuse[index_type].plage();
		auto est_pointeur = type_structure.front() == id_morceau::POINTEUR || type_structure.front() == id_morceau::REFERENCE;

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

				flux << structure->chaine_calculee();
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
				flux << chaine_valeur_enum(ds, membre->chaine());
			}
		}
		else {
			genere_code_C(structure, generatrice, contexte, expr_gauche);

			if (membre->type != type_noeud::VARIABLE) {
				genere_code_C(membre, generatrice, contexte, expr_gauche);
			}

			flux << structure->chaine_calculee();
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

			auto &dt = contexte.typeuse[b->index_type];
			generatrice.declare_variable(dt, nom_acces, membre->chaine_calculee());

			b->valeur_calculee = nom_acces;
		}
		else {
			auto nom_var = "__var_temp_acc" + dls::vers_chaine(index++);
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
		generatrice.os << pos.numero_ligne;
		generatrice.os << ");\n";
		generatrice.os << "}\n";
	}
}

static DonneesFonction *cherche_donnees_fonction(
		ContexteGenerationCode &contexte,
		base *b)
{
	auto module = contexte.fichier(static_cast<size_t>(b->morceau.fichier))->module;
	auto &vdf = module->donnees_fonction(b->morceau.chaine);

	for (auto &df : vdf) {
		if (df.noeud_decl == b) {
			return &df;
		}
	}

	return nullptr;
}

static void pousse_argument_fonction_pile(
		ContexteGenerationCode &contexte,
		DonneesArgument const &argument,
		dls::chaine const &nom_broye)
{
	auto donnees_var = DonneesVariable{};
	donnees_var.est_dynamique = argument.est_dynamic;
	donnees_var.est_variadic = argument.est_variadic;
	donnees_var.index_type = argument.index_type;
	donnees_var.est_argument = true;

	contexte.pousse_locale(argument.nom, donnees_var);

	if (argument.est_employe) {
		auto &dt_var = contexte.typeuse[argument.index_type];
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

static void genere_declaration_structure(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os,
		dls::vue_chaine_compacte const &nom_struct)
{
	auto &donnees = contexte.donnees_structure(nom_struct);

	if (donnees.est_externe) {
		return;
	}

	if (donnees.deja_genere) {
		return;
	}

	auto nom_broye = broye_nom_simple(contexte.nom_struct(donnees.id));

	if (donnees.est_union) {
		if (donnees.est_nonsur) {
			os << "typedef union " << nom_broye << "{\n";
		}
		else {
			os << "typedef struct " << nom_broye << "{\n";
			os << "int membre_actif;\n";
			os << "union {\n";
		}
	}
	else {
		os << "typedef struct " << nom_broye << "{\n";
	}

	for (auto i = 0l; i < donnees.index_types.taille(); ++i) {
		auto index_dt = donnees.index_types[i];
		auto &dt = contexte.typeuse[index_dt];

		for (auto paire_idx_mb : donnees.donnees_membres) {
			if (paire_idx_mb.second.index_membre == i) {
				auto nom = broye_nom_simple(paire_idx_mb.first);
				os << nom_broye_type(contexte, dt) << ' ' << nom << ";\n";
				break;
			}
		}
	}

	if (donnees.est_union && !donnees.est_nonsur) {
		os << "};\n";
	}

	os << "} " << nom_broye << ";\n\n";

	donnees.deja_genere = true;
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
		case type_noeud::SINON:
		case type_noeud::RACINE:
		{
			break;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			auto donnees_fonction = cherche_donnees_fonction(contexte, b);

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

			if (moult_retour) {
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

				auto &dt_ret = contexte.typeuse[b->index_type];
				generatrice.os << nom_broye_type(contexte, dt_ret) << ' ' << nom_fonction;
			}

			contexte.commence_fonction(donnees_fonction);

			/* Crée code pour les arguments */

			auto virgule = '(';

			if (donnees_fonction->args.taille() == 0 && !moult_retour) {
				generatrice.os << '(' << '\n';
				virgule = ' ';
			}

			if (!donnees_fonction->est_externe && !possede_drapeau(b->drapeaux, FORCE_NULCTX)) {
				generatrice.os << virgule << '\n';
				generatrice.os << "__contexte_global *ctx";
				virgule = ',';

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = true;
				donnees_var.est_variadic = false;
				donnees_var.index_type = contexte.index_type_ctx;
				donnees_var.est_argument = true;

				contexte.pousse_locale("ctx", donnees_var);
			}

			for (auto &argument : donnees_fonction->args) {
				auto &dt = contexte.typeuse[argument.index_type];
				auto nom_broye = broye_nom_simple(argument.nom);

				generatrice.os << virgule << '\n';
				generatrice.os << nom_broye_type(contexte, dt) << ' ' << nom_broye;

				virgule = ',';

				pousse_argument_fonction_pile(contexte, argument, nom_broye);
			}

			if (moult_retour) {
				auto idx_ret = 0l;
				for (auto idx : donnees_fonction->idx_types_retours) {
					generatrice.os << virgule << '\n';

					auto nom_ret = "*" + donnees_fonction->noms_retours[idx_ret++];

					auto &dt = contexte.typeuse[idx];
					generatrice.os << nom_broye_type(contexte, dt) << ' ' << nom_ret;
					virgule = ',';
				}
			}

			generatrice.os << ")\n";

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.back();

			genere_code_C(bloc, generatrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			}

			contexte.termine_fonction();

			break;
		}
		case type_noeud::DECLARATION_COROUTINE:
		{
			auto donnees_fonction = cherche_donnees_fonction(contexte, b);

			contexte.commence_fonction(donnees_fonction);

			/* Crée fonction */
			auto nom_fonction = donnees_fonction->nom_broye;
			auto nom_type_coro = "__etat_coro" + nom_fonction;

			/* Déclare la structure d'état de la coroutine. */
			generatrice.os << "typedef struct " << nom_type_coro << " {\n";
			generatrice.os << "pthread_mutex_t mutex_boucle;\n";
			generatrice.os << "pthread_cond_t cond_boucle;\n";
			generatrice.os << "pthread_mutex_t mutex_coro;\n";
			generatrice.os << "pthread_cond_t cond_coro;\n";
			generatrice.os << "bool __termine_coro;\n";
			generatrice.os << "__contexte_global *ctx;\n";

			auto idx_ret = 0l;
			for (auto idx : donnees_fonction->idx_types_retours) {
				auto &nom_ret = donnees_fonction->noms_retours[idx_ret++];
				auto &dt = contexte.typeuse[idx];
				generatrice.declare_variable(dt, nom_ret, "");
			}

			for (auto &argument : donnees_fonction->args) {
				auto &dt = contexte.typeuse[argument.index_type];
				auto nom_broye = broye_nom_simple(argument.nom);
				generatrice.declare_variable(dt, nom_broye, "");
			}

			generatrice.os << " } " << nom_type_coro << ";\n";

			/* Déclare la fonction. */
			generatrice.os << "static void *" << nom_fonction << "(\nvoid *data)\n";
			generatrice.os << "{\n";
			generatrice.os << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
			generatrice.os << "__contexte_global *ctx = __etat->ctx;\n";

			/* déclare les paramètres. */
			for (auto &argument : donnees_fonction->args) {
				auto &dt = contexte.typeuse[argument.index_type];
				auto nom_broye = broye_nom_simple(argument.nom);

				generatrice.declare_variable(dt, nom_broye, "__etat->" + nom_broye);

				pousse_argument_fonction_pile(contexte, argument, nom_broye);
			}

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.back();

			genere_code_C(bloc, generatrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			}

			generatrice.os << "}\n";

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
			auto flux = dls::flux_chaine();

			if (b->aide_generation_code == GENERE_CODE_DECL_VAR || b->aide_generation_code == GENERE_CODE_DECL_VAR_GLOBALE) {
				auto dt = contexte.typeuse[b->index_type];

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
				flux << nom_broye_type(contexte, dt) << ' ' << nom_broye;
				b->valeur_calculee = dls::chaine(flux.chn());

				if (contexte.donnees_fonction == nullptr) {
					auto donnees_var = DonneesVariable{};
					donnees_var.est_externe = (b->drapeaux & EST_EXTERNE) != 0;
					donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
					donnees_var.index_type = b->index_type;

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

				contexte.pousse_locale(b->chaine(), donnees_var);
			}
			/* désactive la vérification car les variables dans les
			 * constructions de structures n'ont pas cette aide */
			else /*if (b->aide_generation_code == GENERE_CODE_ACCES_VAR)*/ {
				if (b->nom_fonction_appel != "") {
					flux << b->nom_fonction_appel;
				}
				else {
					auto dv = contexte.donnees_variable(b->morceau.chaine);

					if (dv.est_membre_emploie) {
						flux << dv.structure;
					}

					flux << broye_chaine(b);
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
		case type_noeud::ACCES_MEMBRE_UNION:
		{
			auto structure = b->enfants.front();
			auto membre = b->enfants.back();

			auto index_membre = std::any_cast<long>(b->valeur_calculee);
			auto &dt = contexte.typeuse[structure->index_type];

			auto est_pointeur = (dt.type_base() == id_morceau::POINTEUR);
			est_pointeur |= (dt.type_base() == id_morceau::REFERENCE);

			auto flux = dls::flux_chaine();
			flux << broye_chaine(structure);
			flux << (est_pointeur ? "->" : ".");

			auto acces_structure = flux.chn();

			flux << broye_chaine(membre);

			auto expr_membre = acces_structure + "membre_actif";

			if (expr_gauche) {
				generatrice.os << expr_membre << " = " << index_membre + 1 << ";";
			}
			else {
				if (b->aide_generation_code != IGNORE_VERIFICATION) {
					auto const &morceau = b->morceau;
					auto module = contexte.fichier(static_cast<size_t>(morceau.fichier));
					auto pos = trouve_position(morceau, module);

					generatrice.os << "if (" << expr_membre << " != " << index_membre + 1 << ") {\n";
					generatrice.os << "KR__acces_membre_union(";
					generatrice.os << '"' << module->chemin << '"' << ',';
					generatrice.os << pos.numero_ligne;
					generatrice.os << ");\n";
					generatrice.os << "}\n";
				}
			}

			b->valeur_calculee = dls::chaine(flux.chn());

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
					if (f->aide_generation_code == GENERE_CODE_DECL_VAR || f->aide_generation_code == GENERE_CODE_DECL_VAR_GLOBALE) {
						genere_code_C(f, generatrice, contexte, true);
						generatrice.os << f->chaine_calculee();
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

			if (b->op != nullptr) {
				genere_code_C(expression, generatrice, contexte, false);

				variable->drapeaux |= POUR_ASSIGNATION;
				genere_code_C(variable, generatrice, contexte, true);

				generatrice.os << variable->chaine_calculee();
				generatrice.os << " = ";

				if (b->op->est_basique) {
					generatrice.os << expression->chaine_calculee();
				}
				else {
					generatrice.os << b->op->nom_fonction << "(";
					generatrice.os << expression->chaine_calculee();
					generatrice.os << ")";
				}

				/* pour les globales */
				generatrice.os << ";\n";

				return;
			}

			applique_transformation(expression, generatrice, contexte, expr_gauche);

			variable->drapeaux |= POUR_ASSIGNATION;
			genere_code_C(variable, generatrice, contexte, true);

			generatrice.os << variable->chaine_calculee();
			generatrice.os << " = ";
			generatrice.os << expression->chaine_calculee();

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

			/* À FAIRE : tests */
			auto flux = dls::flux_chaine();

			applique_transformation(enfant1, generatrice, contexte, expr_gauche);
			applique_transformation(enfant2, generatrice, contexte, expr_gauche);

			auto op = b->op;

			if (op->est_basique) {
				flux << enfant1->chaine_calculee();
				flux << b->morceau.chaine;
				flux << enfant2->chaine_calculee();
			}
			else {
				flux << op->nom_fonction << '(';
				flux << enfant1->chaine_calculee();
				flux << ',';
				flux << enfant2->chaine_calculee();
				flux << ')';
			}

			if (expr_gauche) {
				b->valeur_calculee = dls::chaine(flux.chn());
			}
			else {
				auto nom_var = "__var_temp_bin" + dls::vers_chaine(index++);
				generatrice.os << "const ";
				generatrice.declare_variable(b->index_type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case type_noeud::OPERATION_COMP_CHAINEE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			/* À FAIRE : tests */
			auto flux = dls::flux_chaine();

			genere_code_C(enfant1, generatrice, contexte, expr_gauche);
			genere_code_C(enfant2, generatrice, contexte, expr_gauche);
			genere_code_C(enfant1->enfants.back(), generatrice, contexte, expr_gauche);

			/* (a comp b) */
			flux << '(';
			flux << enfant1->chaine_calculee();
			flux << ')';

			flux << "&&";

			/* (b comp c) */
			flux << '(';
			flux << enfant1->enfants.back()->chaine_calculee();
			flux << b->morceau.chaine;
			flux << enfant2->chaine_calculee();
			flux << ')';

			auto nom_var = "__var_temp_cmp" + dls::vers_chaine(index++);
			generatrice.os << "const ";
			generatrice.declare_variable(b->index_type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
			break;
		}
		case type_noeud::ACCES_TABLEAU:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			auto const index_type1 = enfant1->index_type;
			auto type1 = contexte.typeuse[index_type1];

			if (type1.type_base() == id_morceau::REFERENCE) {
				type1 = type1.dereference();
			}

			/* À FAIRE : tests */
			auto flux = dls::flux_chaine();

			applique_transformation(enfant1, generatrice, contexte, expr_gauche);
			genere_code_C(enfant2, generatrice, contexte, expr_gauche);

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
					flux << enfant1->chaine_calculee();
					flux << '[';
					flux << enfant2->chaine_calculee();
					flux << ']';
					break;
				}
				case id_morceau::CHAINE:
				{
					generatrice.os << "if (";
					generatrice.os << enfant2->chaine_calculee();
					generatrice.os << " < 0 || ";
					generatrice.os << enfant2->chaine_calculee();
					generatrice.os << " >= ";
					generatrice.os << enfant1->chaine_calculee();
					generatrice.os << ".taille) {\n";
					generatrice.os << "KR__depassement_limites(";
					generatrice.os << '"' << module->chemin << '"' << ',';
					generatrice.os << pos.numero_ligne << ',';
					generatrice.os << "\"de la chaine\",";
					generatrice.os << enfant1->chaine_calculee();
					generatrice.os << ".taille,";
					generatrice.os << enfant2->chaine_calculee();
					generatrice.os << ");\n}\n";

					flux << enfant1->chaine_calculee();
					flux << ".pointeur";
					flux << '[';
					flux << enfant2->chaine_calculee();
					flux << ']';

					break;
				}
				case id_morceau::TABLEAU:
				{
					auto taille_tableau = static_cast<int>(type_base >> 8);

					if (b->aide_generation_code != IGNORE_VERIFICATION) {
						generatrice.os << "if (";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << " < 0 || ";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << " >= ";

						if (taille_tableau == 0) {
							generatrice.os << enfant1->chaine_calculee();
							generatrice.os << ".taille";
						}
						else {
							generatrice.os << taille_tableau;
						}

						generatrice.os << ") {\n";
						generatrice.os << "KR__depassement_limites(";
						generatrice.os << '"' << module->chemin << '"' << ',';
						generatrice.os << pos.numero_ligne << ',';
						generatrice.os << "\"du tableau\",";
						if (taille_tableau == 0) {
							generatrice.os << enfant1->chaine_calculee();
							generatrice.os << ".taille";
						}
						else {
							generatrice.os << taille_tableau;
						}
						generatrice.os << ",";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << ");\n}\n";
					}

					flux << enfant1->chaine_calculee();

					if (taille_tableau == 0) {
						flux << ".pointeur";
					}

					flux << '[';
					flux << enfant2->chaine_calculee();
					flux << ']';
					break;
				}
				default:
				{
					assert(false);
					break;
				}
			}

			/* pour les accès tableaux à gauche d'un '=', il ne faut pas passer
			 * par une variable temporaire */
			if (expr_gauche) {
				b->valeur_calculee = dls::chaine(flux.chn());
			}
			else {
				auto nom_var = "__var_temp_acctabl" + dls::vers_chaine(index++);
				generatrice.os << "const ";
				generatrice.declare_variable(b->index_type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto enfant = b->enfants.front();

			/* À FAIRE : tests */

			/* force une expression si l'opérateur est @, pour que les
			 * expressions du type @a[0] retourne le pointeur à a + 0 et non le
			 * pointeur de la variable temporaire du code généré */
			expr_gauche |= b->morceau.identifiant == id_morceau::AROBASE;
			applique_transformation(enfant, generatrice, contexte, expr_gauche);

			if (b->morceau.identifiant == id_morceau::AROBASE) {
				b->valeur_calculee = "&(" + enfant->chaine_calculee() + ")";
			}
			else {
				auto flux = dls::flux_chaine();
				auto op = b->op;

				if (op->est_basique) {
					flux << b->morceau.chaine;
				}
				else {
					flux << b->op->nom_fonction;
				}

				flux << '(';
				flux << enfant->chaine_calculee();
				flux << ')';

				b->valeur_calculee = dls::chaine(flux.chn());
			}

			break;
		}
		case type_noeud::RETOUR:
		{
			genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			generatrice.os << "return;\n";
			break;
		}
		case type_noeud::RETOUR_SIMPLE:
		{
			auto enfant = b->enfants.front();
			auto df = contexte.donnees_fonction;
			auto nom_variable = df->noms_retours[0];

			applique_transformation(enfant, generatrice, contexte, false);

			auto &dt = contexte.typeuse[enfant->index_type];

			generatrice.declare_variable(
						dt,
						nom_variable,
						enfant->chaine_calculee());

			/* NOTE : le code différé doit être créé après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			generatrice.os << "return " << nom_variable << ";\n";
			break;
		}
		case type_noeud::RETOUR_MULTIPLE:
		{
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
					applique_transformation(f, generatrice, contexte, false);

					generatrice.os << '*' << df->noms_retours[idx++] << " = ";
					generatrice.os << f->chaine_calculee();
					generatrice.os << ';';
				}
			}

			/* NOTE : le code différé doit être créé après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			generatrice.os << "return;\n";
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

				flux << enfant1->chaine_calculee();
				flux << " ? ";
				/* prenons les enfants des enfants pour ne mettre des accolades
				 * autour de l'expression vu qu'ils sont de type 'BLOC' */
				flux << enfant2->enfants.front()->chaine_calculee();
				flux << " : ";
				flux << enfant3->enfants.front()->chaine_calculee();

				auto nom_variable = "__var_temp_si" + dls::vers_chaine(index++);

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

			generatrice.os << enfant1->chaine_calculee();
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

				if (est_type_retour(enfant->type)) {
					break;
				}

				if (enfant->type == type_noeud::OPERATION_BINAIRE) {
					/* les assignations opérées (+=, etc) n'ont pas leurs codes
					 * générées via genere_code_C  */
					if (est_assignation_operee(enfant->identifiant())) {
						generatrice.os << enfant->chaine_calculee();
						generatrice.os << ";\n";
					}
				}
			}

			if (dernier_enfant != nullptr && !est_type_retour(dernier_enfant->type)) {
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
			auto &type_debut = contexte.typeuse[index_type];
			auto const type = type_debut.type_base();

			enfant1->index_type = index_type;

			contexte.empile_nombre_locales();

			auto genere_code_tableau_chaine = [&generatrice](
					dls::flux_chaine &os_loc,
					ContexteGenerationCode &contexte_loc,
					base *enfant_1,
					base *enfant_2,
					DonneesTypeFinal &dt,
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
				os_loc << enfant_2->chaine_calculee();
				os_loc << ".taille - 1; ++"<< nom_var <<") {\n";
				os_loc << nom_broye_type(contexte_loc, dt);
				os_loc << " " << broye_chaine(var) << " = &";
				os_loc << enfant_2->chaine_calculee();
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
					DonneesTypeFinal &dt,
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
				os_loc << nom_broye_type(contexte_loc, dt);
				os_loc << " *" << broye_chaine(var) << " = &";
				os_loc << enfant_2->chaine_calculee();
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
					generatrice.os << nom_broye_type(contexte, type_debut);
					generatrice.os << " " << nom_broye << " = ";
					generatrice.os << enfant2->enfants.front()->chaine_calculee();

					generatrice.os << "; "
					   << nom_broye << " <= ";
					generatrice.os << enfant2->enfants.back()->chaine_calculee();

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

					auto donnees_var = DonneesVariable{};

					if ((type & 0xff) == id_morceau::TABLEAU) {
						auto const taille_tableau = static_cast<uint64_t>(type >> 8);

						auto idx_type_deref = contexte.typeuse.type_dereference_pour(index_type);
						auto idx_type_ref = contexte.typeuse.type_reference_pour(idx_type_deref);
						auto &type_deref = contexte.typeuse[idx_type_ref];

						if (taille_tableau != 0) {
							genere_code_tableau_fixe(generatrice.os, contexte, enfant1, enfant2, type_deref, nom_var, taille_tableau);
						}
						else {
							genere_code_tableau_chaine(generatrice.os, contexte, enfant1, enfant2, type_deref, nom_var);
						}
					}
					else if (type == id_morceau::CHAINE) {
						index_type = contexte.typeuse[TypeBase::REF_Z8];
						auto &dt = contexte.typeuse[index_type];
						genere_code_tableau_chaine(generatrice.os, contexte, enfant1, enfant2, dt, nom_var);
					}

					if (b->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
						auto var = enfant1->enfants.front();
						auto idx = enfant1->enfants.back();

						donnees_var.index_type = index_type;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.index_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						donnees_var.index_type = index_type;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					auto df = enfant2->df;
					auto nom_etat = "__etat" + dls::vers_chaine(enfant2);
					auto nom_type_coro = "__etat_coro" + df->nom_broye;

					generatrice.os << nom_type_coro << " " << nom_etat << " = {\n";
					generatrice.os << ".mutex_boucle = PTHREAD_MUTEX_INITIALIZER,\n";
					generatrice.os << ".mutex_coro = PTHREAD_MUTEX_INITIALIZER,\n";
					generatrice.os << ".cond_coro = PTHREAD_COND_INITIALIZER,\n";
					generatrice.os << ".cond_boucle = PTHREAD_COND_INITIALIZER,\n";
					generatrice.os << ".ctx = ctx,\n";
					generatrice.os << ".__termine_coro = 0\n";
					generatrice.os << "};\n";

					/* intialise les arguments de la fonction. */
					for (auto enfs : enfant2->enfants) {
						genere_code_C(enfs, generatrice, contexte, false);
					}

					auto iter_enf = enfant2->enfants.debut();

					for (auto &arg : df->args) {
						auto nom_broye = broye_nom_simple(arg.nom);
						generatrice.os << nom_etat << '.' << nom_broye << " = ";
						generatrice.os << (*iter_enf)->chaine_calculee();
						generatrice.os << ";\n";
						++iter_enf;
					}

					generatrice.os << "pthread_t fil_coro;\n";
					generatrice.os << "pthread_create(&fil_coro, NULL, " << df->nom_broye << ", &" << nom_etat << ");\n";
					generatrice.os << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";
					generatrice.os << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
					generatrice.os << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

					/* À FAIRE : utilisation du type */
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

					generatrice.os << "while (" << nom_etat << ".__termine_coro == 0) {\n";
					generatrice.os << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";

					for (auto i = 0l; i < nombre_vars_ret; ++i) {
						auto f = feuilles[i];
						auto nom_var_broye = broye_chaine(f);
						generatrice.declare_variable(type_debut, nom_var_broye, "");
						generatrice.os << nom_var_broye << " = "
									   << nom_etat << '.' << df->noms_retours[i]
									   << ";\n";

						auto donnees_var = DonneesVariable{};
						donnees_var.index_type = df->idx_types_retours[i];
						contexte.pousse_locale(f->chaine(), donnees_var);
					}

					generatrice.os << "pthread_mutex_lock(&" << nom_etat << ".mutex_coro);\n";
					generatrice.os << "pthread_cond_signal(&" << nom_etat << ".cond_coro);\n";
					generatrice.os << "pthread_mutex_unlock(&" << nom_etat << ".mutex_coro);\n";
					generatrice.os << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
					generatrice.os << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

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

			if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE || b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
				generatrice.os << "pthread_join(fil_coro, NULL);\n";
			}

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
			generatrice.os << enfant2->chaine_calculee();
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
			generatrice.os << enfant1->chaine_calculee();
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

			auto &dt = contexte.typeuse[b->index_type];

			auto flux = dls::flux_chaine();

			flux << "(";
			flux << nom_broye_type(contexte, dt);
			flux << ")(";
			flux << enfant->chaine_calculee();
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
			auto const &donnees = contexte.typeuse[index_dt];
			b->valeur_calculee = dls::vers_chaine(taille_octet_type(contexte, donnees));
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

			auto &type = contexte.typeuse[b->index_type];

			for (auto enfant : b->enfants) {
				applique_transformation(enfant, generatrice, contexte, false);
			}

			/* alloue un tableau fixe */
			auto dt_tfixe = DonneesTypeFinal{};
			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(taille_tableau << 8));
			dt_tfixe.pousse(type);

			auto nom_tableau_fixe = dls::chaine("__tabl_fix")
					.append(dls::vers_chaine(reinterpret_cast<long>(b)));

			generatrice.os << nom_broye_type(contexte, dt_tfixe) << ' ' << nom_tableau_fixe;
			generatrice.os << " = ";

			auto virgule = '{';

			for (auto enfant : b->enfants) {
				generatrice.os << virgule;
				generatrice.os << enfant->chaine_calculee();
				virgule = ',';
			}

			generatrice.os << "};\n";

			/* alloue un tableau dynamique */
			auto dt_tdyn = DonneesTypeFinal{};
			dt_tdyn.pousse(id_morceau::TABLEAU);
			dt_tdyn.pousse(type);

			auto nom_tableau_dyn = dls::chaine("__tabl_dyn")
					.append(dls::vers_chaine(b));

			generatrice.os << nom_broye_type(contexte, dt_tdyn) << ' ' << nom_tableau_dyn << ";\n";
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
				flux << (*enfant)->chaine_calculee();
				++enfant;
				++nom_param;

				virgule = ',';
			}

			if (liste_params->taille() == 0) {
				flux << '{';
			}

			flux << '}';

			b->valeur_calculee = dls::chaine(flux.chn());

			break;
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			auto nom_tableau = "__tabl" + dls::vers_chaine(b);
			auto &dt = contexte.typeuse[b->index_type];

			dls::tableau<base *> feuilles;
			rassemble_feuilles(b->enfants.front(), feuilles);

			for (auto f : feuilles) {
				genere_code_C(f, generatrice, contexte, false);
			}

			generatrice.os << nom_broye_type(contexte, dt) << ' ' << nom_tableau << " = ";

			auto virgule = '{';

			for (auto f : feuilles) {
				generatrice.os << virgule;
				generatrice.os << f->chaine_calculee();
				virgule = ',';
			}

			generatrice.os << "};\n";

			b->valeur_calculee = nom_tableau;
			break;
		}
		case type_noeud::INFO_DE:
		{
			auto enfant = b->enfants.front();
			auto &dt = contexte.typeuse[enfant->index_type];
			b->valeur_calculee = "&" + dt.ptr_info_type;
			break;
		}
		case type_noeud::MEMOIRE:
		{
			genere_code_C(b->enfants.front(), generatrice, contexte, false);
			b->valeur_calculee = "*(" + b->enfants.front()->chaine_calculee() + ")";
			break;
		}
		case type_noeud::LOGE:
		{
			auto &dt = contexte.typeuse[b->index_type];
			auto enfant = b->enfants.debut();
			auto nombre_enfant = b->enfants.taille();
			auto a_pointeur = false;
			auto nom_ptr_ret = dls::chaine("");
			auto nom_taille = "__taille_allouee" + dls::vers_chaine(index++);

			if (dt.type_base() == id_morceau::TABLEAU) {
				auto expr = b->type_declare.expressions[0];

				a_pointeur = true;
				auto nom_ptr = "__ptr" + dls::vers_chaine(b);
				auto nom_tabl = "__tabl" + dls::vers_chaine(b);
				auto taille_tabl = "__taille_tabl" + dls::vers_chaine(b);

				genere_code_C(expr, generatrice, contexte, false);

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							taille_tabl,
							expr->chaine_calculee());

				auto flux = dls::flux_chaine();
				auto index_dt = contexte.typeuse.ajoute_type(dt.dereference());
				auto &dt_deref = contexte.typeuse[index_dt];

				auto expr_sizeof = "sizeof(" + nom_broye_type(contexte, dt_deref) + ") * " + taille_tabl;

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							nom_taille,
							expr_sizeof);

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
							contexte.typeuse[TypeBase::Z64],
							nom_taille,
							enf->chaine_calculee());

				nombre_enfant -= 1;

				generatrice.os << "char *" << nom_ptr << " = (char *)(malloc(sizeof(char) * (";
				generatrice.os << nom_taille << ")));\n";

				generatrice.os << "chaine " << nom_chaine << ";\n";
				generatrice.os << nom_chaine << ".pointeur = " << nom_ptr << ";\n";
				generatrice.os << nom_chaine << ".taille = " << nom_taille << ";\n";

				nom_ptr_ret = nom_chaine;
			}
			else {
				auto index_dt = contexte.typeuse.ajoute_type(dt.dereference());
				auto &dt_deref = contexte.typeuse[index_dt];
				generatrice.os << "long " << nom_taille << " = sizeof(";
				generatrice.os << nom_broye_type(contexte, dt_deref) << ");\n";

				auto nom_ptr = "__ptr" + dls::vers_chaine(b);
				generatrice.os << nom_broye_type(contexte, dt);
				generatrice.os << ' ' << nom_ptr << " = (";
				generatrice.os << nom_broye_type(contexte, dt);
				generatrice.os << ")(malloc(" << nom_taille << "));\n";
				/* À FAIRE : évite de mettre à zéro quand non-nécessaire */
				generatrice.os << "memset(" << nom_ptr << ",0," << nom_taille << ");\n";

				/* initialise la structure */
				if ((dt_deref.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
					cree_initialisation(
								contexte,
								generatrice,
								dt_deref.plage(),
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
			generatrice.os << "__VG_memoire_consommee__ = (__VG_memoire_consommee__ >= __VG_memoire_utilisee__) ? __VG_memoire_consommee__ : __VG_memoire_utilisee__;\n";
			generatrice.os << "__VG_nombre_allocations__ += 1;\n";
			b->valeur_calculee = nom_ptr_ret;

			break;
		}
		case type_noeud::DELOGE:
		{
			auto enfant = b->enfants.front();
			auto &dt = contexte.typeuse[enfant->index_type];
			auto nom_taille = "__taille_allouee" + dls::vers_chaine(index++);

			genere_code_C(enfant, generatrice, contexte, true);
			auto chn_enfant = enfant->chaine_calculee();

			if (dt.type_base() == id_morceau::TABLEAU || dt.type_base() == id_morceau::CHAINE) {
				generatrice.os << "long " << nom_taille << " = ";
				generatrice.os << chn_enfant << ".taille";

				if (dt.type_base() == id_morceau::TABLEAU) {
					auto index_dt = contexte.typeuse.ajoute_type(dt.dereference());
					auto &dt_deref = contexte.typeuse[index_dt];
					generatrice.os << " * sizeof(" << nom_broye_type(contexte, dt_deref) << ")";
				}

				generatrice.os << ";\n";

				generatrice.os << "free(" << chn_enfant << ".pointeur);\n";
				generatrice.os << chn_enfant << ".pointeur = 0;\n";
				generatrice.os << chn_enfant << ".taille = 0;\n";
			}
			else {
				auto index_dt = contexte.typeuse.ajoute_type(dt.dereference());
				auto &dt_deref = contexte.typeuse[index_dt];

				generatrice.os << "long " << nom_taille << " = sizeof(";
				generatrice.os << nom_broye_type(contexte, dt_deref) << ");\n";
				generatrice.os << "free(" << chn_enfant << ");\n";
				generatrice.os << chn_enfant << " = 0;\n";
			}

			generatrice.os << "__VG_memoire_utilisee__ -= " << nom_taille << ";\n";
			generatrice.os << "__VG_nombre_deallocations__ += 1;\n";

			break;
		}
		case type_noeud::RELOGE:
		{
			auto &dt_pointeur = contexte.typeuse[b->index_type];
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
				auto taille_tabl = "__taille_tabl" + dls::vers_chaine(b);

				genere_code_C(expr, generatrice, contexte, false);
				applique_transformation(enfant1, generatrice, contexte, true);

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							taille_tabl,
							expr->chaine_calculee());

				nom_ptr_ret = enfant1->chaine_calculee();
				auto acces_taille = nom_ptr_ret + ".taille";
				auto acces_pointeur = nom_ptr_ret + ".pointeur";

				auto index_dt = contexte.typeuse.ajoute_type(dt_pointeur.dereference());
				auto &dt_deref = contexte.typeuse[index_dt];
				auto const &chn_type_deref = nom_broye_type(contexte, dt_deref);

				auto expr_sizeof = "sizeof(" + chn_type_deref + ")";

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							nom_ancienne_taille,
							expr_sizeof + " * " + acces_taille);

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							nom_nouvelle_taille,
							expr_sizeof + " * " + taille_tabl);

				generatrice.os << acces_pointeur << " = (" << chn_type_deref << " *)(realloc(";
				generatrice.os << acces_pointeur << ", " << nom_nouvelle_taille << "));\n";
				generatrice.os << acces_taille << " = " << taille_tabl << ";\n";
			}
			else if (dt_pointeur.type_base() == id_morceau::CHAINE) {
				auto enfant2 = *enfant++;
				nombre_enfant -= 1;
				a_pointeur = true;

				applique_transformation(enfant1, generatrice, contexte, true);
				nom_ptr_ret = enfant1->chaine_calculee();
				genere_code_C(enfant2, generatrice, contexte, true);

				auto acces_taille = nom_ptr_ret + ".taille";
				auto acces_pointeur = nom_ptr_ret + ".pointeur";

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							nom_ancienne_taille,
							acces_taille);

				generatrice.os << "long " << nom_nouvelle_taille << " = sizeof(char) *";
				generatrice.os << enfant2->chaine_calculee();
				generatrice.os << ";\n";

				generatrice.os << acces_pointeur << " = (char *)(realloc(";
				generatrice.os << acces_pointeur << ", " << nom_nouvelle_taille<< "));\n";
				generatrice.os << acces_taille << " = " << nom_nouvelle_taille << ";\n";
			}
			else {
				auto index_dt = contexte.typeuse.ajoute_type(dt_pointeur.dereference());
				auto &dt_deref = contexte.typeuse[index_dt];

				auto const &chn_type_deref = nom_broye_type(contexte, dt_deref);

				auto expr = "sizeof(" + chn_type_deref + ")";

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							nom_ancienne_taille,
							expr);

				generatrice.declare_variable(
							contexte.typeuse[TypeBase::Z64],
							nom_nouvelle_taille,
							nom_ancienne_taille);

				applique_transformation(enfant1, generatrice, contexte, true);
				nom_ptr_ret = enfant1->chaine_calculee();

				generatrice.os << nom_ptr_ret;
				generatrice.os << " = (" << nom_broye_type(contexte, dt_pointeur);
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
			generatrice.os << "__VG_memoire_consommee__ = (__VG_memoire_consommee__ >= __VG_memoire_utilisee__) ? __VG_memoire_consommee__ : __VG_memoire_utilisee__;\n";
			generatrice.os << "__VG_nombre_allocations__ += 1;\n";
			generatrice.os << "__VG_nombre_reallocations__ += 1;\n";

			break;
		}
		case type_noeud::DECLARATION_STRUCTURE:
		{
			genere_declaration_structure(contexte, generatrice.os, b->chaine());
			break;
		}
		case type_noeud::DECLARATION_ENUM:
		{
			/* nous ne générons pas de code pour les énums, nous prenons leurs
			 * valeurs directement */
			break;
		}
		case type_noeud::DISCR:
		{
			/* le premier enfant est l'expression, les suivants les paires */

			auto nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			for (auto i = 1l; i < nombre_enfants; ++i) {
				auto paire = *iter_enfant++;
				auto enf0 = paire->enfants.front();
				auto enf1 = paire->enfants.back();

				if (enf0->type != type_noeud::SINON) {
					genere_code_C(enf0, generatrice, contexte, true);

					generatrice.os << "if (";
					generatrice.os << chaine_expr;
					generatrice.os << " == ";
					generatrice.os << enf0->chaine_calculee();
					generatrice.os << ") ";
				}

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
		case type_noeud::PAIRE_DISCR:
		{
			/* RAF : pris en charge dans type_noeud::DISCR, ce noeud n'est que
			 * pour ajouter un niveau d'indirection et faciliter la compilation
			 * des discriminations. */
			assert(false);
			break;
		}
		case type_noeud::DISCR_UNION:
		{
			/* switch (union.membre_actif) {
			 * case index membre + 1: {
			 *	bloc
			 *	break;
			 * }
			 */

			auto nombre_enfant = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			auto const &dt = contexte.typeuse[expression->index_type];
			auto const est_pointeur = dt.type_base() == id_morceau::POINTEUR;
			auto id = static_cast<long>(dt.type_base() >> 8);
			auto &ds = contexte.donnees_structure(id);

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_calculee = expression->chaine_calculee();
			chaine_calculee += (est_pointeur ? "->" : ".");

			generatrice.os << "switch (" << chaine_calculee << "membre_actif) {\n";

			for (auto i = 1; i < nombre_enfant; ++i) {
				auto enfant = *iter_enfant++;
				auto expr_paire = enfant->enfants.front();
				auto bloc_paire = enfant->enfants.back();

				if (expr_paire->type == type_noeud::SINON) {
					generatrice.os << "default";
				}
				else {
					auto iter_dm = ds.donnees_membres.trouve(expr_paire->chaine());
					auto const &dm = iter_dm->second;
					generatrice.os << "case " << dm.index_membre + 1;
				}

				auto iter_membre = ds.donnees_membres.trouve(expr_paire->chaine());

				auto dv = DonneesVariable{};
				dv.index_type = ds.index_types[iter_membre->second.index_membre];
				dv.est_argument = true;
				dv.est_membre_emploie = true;
				dv.structure = chaine_calculee;

				contexte.empile_nombre_locales();
				contexte.pousse_locale(iter_membre->first, dv);

				generatrice.os << ": {\n";
				genere_code_C(bloc_paire, generatrice, contexte, true);
				generatrice.os << "break;\n}\n";
				contexte.depile_nombre_locales();
			}

			generatrice.os << "}\n";

			break;
		}
		case type_noeud::RETIENS:
		{
			auto df = contexte.donnees_fonction;
			auto enfant = b->enfants.front();

			generatrice.os << "pthread_mutex_lock(&__etat->mutex_coro);\n";

			auto feuilles = dls::tableau<base *>{};
			rassemble_feuilles(enfant, feuilles);

			for (auto i = 0l; i < feuilles.taille(); ++i) {
				auto f = feuilles[i];

				genere_code_C(f, generatrice, contexte, true);

				generatrice.os << "__etat->" << df->noms_retours[i] << " = ";
				generatrice.os << f->chaine_calculee();
				generatrice.os << ";\n";
			}

			generatrice.os << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
			generatrice.os << "pthread_cond_signal(&__etat->cond_boucle);\n";
			generatrice.os << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
			generatrice.os << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
			generatrice.os << "pthread_mutex_unlock(&__etat->mutex_coro);\n";

			break;
		}
	}
}

// CHERCHE (noeud_principale : FONCTION { nom = "principale" })
// CHERCHE (noeud_principale) -[:UTILISE_FONCTION|UTILISE_TYPE*]-> (noeud)
// RETOURNE DISTINCT noeud_principale, noeud
static void traverse_graphe_pour_generation_code(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		NoeudDependance *noeud,
		bool genere_info_type = true)
{
	noeud->fut_visite = true;

	for (auto const &relation : noeud->relations) {
		auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
		accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
		accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

		if (!accepte) {
			continue;
		}

		/* À FAIRE : dépendances cycliques :
		 * - types qui s'incluent indirectement (listes chainées intrusives)
		 * - fonctions recursives
		 */
		if (relation.noeud_fin->fut_visite) {
			continue;
		}

		traverse_graphe_pour_generation_code(contexte, generatrice, relation.noeud_fin, genere_info_type);
	}

	if (noeud->type == TypeNoeudDependance::TYPE) {
		if (noeud->noeud_syntactique != nullptr) {
			genere_code_C(noeud->noeud_syntactique, generatrice, contexte, false);
		}

		if (noeud->index == -1) {
			return;
		}

		auto &dt = contexte.typeuse[noeud->index];
		cree_typedef(contexte, dt, generatrice.os);

		if (!genere_info_type) {
			return;
		}

		/* Suppression des avertissements pour les conversions dites
		 * « imcompatibles » alors qu'elles sont bonnes.
		 * Elles surviennent dans les assignations des pointeurs, par exemple pour
		 * ceux des tableaux des membres des fonctions.
		 */
		generatrice.os << "#pragma GCC diagnostic push\n";
		generatrice.os << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

		auto id_info_type = IDInfoType();

		cree_info_type_C(contexte, generatrice, generatrice.os, dt, id_info_type);

		generatrice.os << "#pragma GCC diagnostic pop\n";
	}
	else {
		genere_code_C(noeud->noeud_syntactique, generatrice, contexte, false);
	}
}

void genere_code_C(
		base *b,
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	if (b->type != type_noeud::RACINE) {
		return;
	}

	auto generatrice = GeneratriceCodeC(contexte, os);

	auto temps_validation = 0.0;
	auto temps_generation = 0.0;

	for (auto noeud : b->enfants) {
		auto debut_validation = dls::chrono::compte_seconde();
		performe_validation_semantique(noeud, contexte, true);
		temps_validation += debut_validation.temps();
	}

	/* déclaration des types de bases */
	os << "typedef struct chaine { char *pointeur; long taille; } chaine;\n";
	os << "typedef struct eini { void *pointeur; struct InfoType *info; } eini;\n";
	os << "typedef unsigned char bool;\n";
	os << "typedef unsigned char octet;\n";
	os << "typedef void Ksnul;\n";
	os << "typedef struct __contexte_global Ks__contexte_global;\n";
	/* À FAIRE : pas beau, mais un pointeur de fonction peut être un pointeur
	 * vers une fonction de LibC dont les arguments variadiques ne sont pas
	 * typés */
	os << "#define Kv ...\n";

	auto debut_generation = dls::chrono::compte_seconde();
	auto &graphe_dependance = contexte.graphe_dependance;

	/* il faut d'abord créer le code pour les structures InfoType */
	const char *noms_structs_infos_types[] = {
		"InfoType",
		"InfoTypeEntier",
		"InfoTypeRéel",
		"InfoTypePointeur",
		"InfoTypeÉnum",
		"InfoTypeStructure",
		"InfoTypeTableau",
		"InfoTypeFonction",
		"InfoTypeMembreStructure",
	};

	for (auto nom_struct : noms_structs_infos_types) {
		auto const &ds = contexte.donnees_structure(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(ds.index_type);
		traverse_graphe_pour_generation_code(contexte, generatrice, noeud, false);
		/* restaure le drapeaux pour la génération des infos des types */
		noeud->fut_visite = false;
	}

	for (auto nom_struct : noms_structs_infos_types) {
		auto const &ds = contexte.donnees_structure(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(ds.index_type);
		traverse_graphe_pour_generation_code(contexte, generatrice, noeud, true);
	}

	temps_generation += debut_generation.temps();

	reduction_transitive(graphe_dependance);

	auto noeud = graphe_dependance.cherche_noeud_fonction("principale");

	if (noeud == nullptr) {
		erreur::fonction_principale_manquante();
	}

	debut_generation.commence();
	traverse_graphe_pour_generation_code(contexte, generatrice, noeud);
	temps_generation += debut_generation.temps();

	contexte.temps_generation = temps_generation;
	contexte.temps_validation = temps_validation;
}

}  /* namespace noeud */
