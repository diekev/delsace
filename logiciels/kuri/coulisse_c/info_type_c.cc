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

#include "info_type_c.hh"

#include "arbre_syntactic.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "typage.hh"

// À FAIRE : les types récursifs (p.e. listes chainées) générent plusieurs fois
// leurs info_types, donc nous utilisons un index pour les différentier
static int index_info_type = 0;

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
	dls::chaine OCTET     = "11";
};

static auto cree_info_type_defaul_C(
		dls::flux_chaine &os_decl,
		dls::chaine const &id_type,
		dls::chaine const &nom_info_type,
		unsigned taille_en_octet)
{
	os_decl << "static const InfoType " << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_type << ",\n";
	os_decl << "\t.taille_en_octet = " << taille_en_octet << '\n';
	os_decl << "};\n";
}

static auto cree_info_type_entier_C(
		dls::flux_chaine &os_decl,
		unsigned taille_en_octet,
		bool est_signe,
		IDInfoType const &id_info_type,
		dls::chaine const &nom_info_type)
{
	os_decl << "static const InfoTypeEntier " << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.ENTIER << ",\n";
	os_decl << '\t' << broye_nom_simple(".est_signé = ") << est_signe << ",\n";
	os_decl << "\t.taille_en_octet = " << taille_en_octet << '\n';
	os_decl << "};\n";
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		Type *type,
		IDInfoType const &id_info_type);

static auto cree_info_type_structure_C(
		dls::flux_chaine &os_decl,
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::vue_chaine_compacte const &nom_struct,
		NoeudStruct *noeud_decl,
		Type *type,
		IDInfoType const &id_info_type,
		dls::chaine const &nom_info_type)
{
	if (type->ptr_info_type != "") {
		return;
	}

	auto const nom_broye = broye_nom_simple(nom_struct);

	/* prédéclare l'infotype au cas où la structure s'incluerait elle-même via
	 * un pointeur (p.e. listes chainées) */
	os_decl << "static const InfoTypeStructure " << nom_info_type << ";\n";

	/* met en place le 'pointeur' au cas où une structure s'incluerait elle-même
	 * via un pointeur */
	type->ptr_info_type = nom_info_type;

	if (noeud_decl->est_externe) {
		os_decl << "static const InfoTypeStructure " << nom_info_type << " = {\n";
		os_decl << "\t.id = " << id_info_type.STRUCTURE << ",\n";
		os_decl << "\t.taille_en_octet = " << type->taille_octet << ",\n";
		os_decl << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
		os_decl << "\t.membres = {.pointeur = 0, .taille = 0 },\n";
		os_decl << "};\n";
		return;
	}

	auto const nombre_membres = noeud_decl->bloc->membres.taille;

	POUR (noeud_decl->desc.membres) {
		if (it.type->ptr_info_type == "") {
			it.type->ptr_info_type = cree_info_type_C(
						contexte, generatrice, os_decl, it.type, id_info_type);
		}
	}

	/* crée le tableau des données des membres */
	auto const nom_tableau_membre = "__info_type_membres" + nom_info_type;
	auto pointeurs = dls::tableau<dls::chaine>();

	POUR (noeud_decl->desc.membres) {
		auto nom_info_type_membre = "__info_type_membre" + dls::vers_chaine(index_info_type++);
		pointeurs.pousse(nom_info_type_membre);

		os_decl << "static const InfoTypeMembreStructure " << nom_info_type_membre << " = {\n";
		os_decl << "\t.nom = { .pointeur = \"" << it.nom << "\", .taille = " << it.nom.taille << " },\n";
		os_decl << "\t" << broye_nom_simple(".décalage = ") << it.decalage << ",\n";
		os_decl << "\t.id = (InfoType *)(&" << it.type->ptr_info_type << ")\n";
		os_decl << "};\n";
	}

	os_decl << "static const InfoTypeMembreStructure *" << nom_tableau_membre << "[] = {\n";
	for (auto &pointeur : pointeurs) {
		os_decl << "&" << pointeur << ",";
	}
	os_decl << "};\n";

	/* crée l'info pour la structure */
	os_decl << "static const InfoTypeStructure " << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.STRUCTURE << ",\n";
	os_decl << "\t.taille_en_octet = " << type->taille_octet << ",\n";
	os_decl << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
	os_decl << "\t.membres = {.pointeur = " << nom_tableau_membre << ", .taille = " << nombre_membres << "},\n";
	os_decl << "};\n";
}

static auto cree_info_type_enum_C(
		dls::flux_chaine &os_decl,
		dls::vue_chaine_compacte const &nom_struct,
		NoeudEnum *noeud_decl,
		IDInfoType const &id_info_type,
		dls::chaine const &nom_info_type,
		unsigned taille_octet)
{
	auto nom_broye = broye_nom_simple(nom_struct);
	auto nombre_valeurs = noeud_decl->desc.noms.taille;

	/* crée un tableau pour les noms des énumérations */
	auto const nom_tableau_noms = "__tableau_noms_enum" + nom_info_type;

	os_decl << "static const chaine " << nom_tableau_noms << "[] = {\n";

	POUR (noeud_decl->desc.noms) {
		os_decl << "\t{.pointeur=\"" << it << "\", .taille=" << it.taille << "},\n";
	}

	os_decl << "};\n";

	/* crée le tableau pour les valeurs */
	auto const nom_tableau_valeurs = "__tableau_valeurs_enum" + nom_info_type;

	os_decl << "static const int " << nom_tableau_valeurs << "[] = {\n\t";

	POUR (noeud_decl->desc.valeurs) {
		os_decl << it << ',';
	}

	os_decl << "\n};\n";

	/* crée l'info type pour l'énum */
	os_decl << "static const " << broye_nom_simple("InfoTypeÉnum ") << nom_info_type << " = {\n";
	os_decl << "\t.id = " << id_info_type.ENUM << ",\n";
	os_decl << "\t.taille_en_octet = " << taille_octet << ",\n";
	os_decl << "\t.est_drapeau = " << noeud_decl->desc.est_drapeau << ",\n";
	os_decl << "\t.nom = { .pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << " },\n";
	os_decl << "\t.noms = { .pointeur = " << nom_tableau_noms << ", .taille = " << nombre_valeurs << " },\n ";
	os_decl << "\t.valeurs = { .pointeur = " << nom_tableau_valeurs << ", .taille = " << nombre_valeurs << " },\n ";
	os_decl << "};\n";
}

dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		Type *type,
		IDInfoType const &id_info_type)
{
	auto nom_info_type = "__info_type" + nom_broye_type(type, true) + dls::vers_chaine(index_info_type++);

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			assert(false);
			break;
		}
		case GenreType::BOOL:
		{
			cree_info_type_defaul_C(os_decl, id_info_type.BOOLEEN, nom_info_type, 1);
			break;
		}
		case GenreType::OCTET:
		{
			cree_info_type_defaul_C(os_decl, id_info_type.OCTET, nom_info_type, 1);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			cree_info_type_entier_C(os_decl, 4, true, id_info_type, nom_info_type);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			cree_info_type_entier_C(os_decl, type->taille_octet, false, id_info_type, nom_info_type);
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			cree_info_type_entier_C(os_decl, type->taille_octet, true, id_info_type, nom_info_type);
			break;
		}
		case GenreType::REEL:
		{
			cree_info_type_defaul_C(os_decl, id_info_type.REEL, nom_info_type, type->taille_octet);
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			if (type_deref && type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, type_deref, id_info_type);
			}

			os_decl << "static const InfoTypePointeur " << nom_info_type << " = {\n";
			os_decl << ".id = " << id_info_type.POINTEUR << ",\n";
			os_decl << ".taille_en_octet = " << type->taille_octet << ",\n";
			if (type_deref) {
				os_decl << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			}
			else {
				os_decl << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
			}
			os_decl << '\t' << broye_nom_simple(".est_référence = ")
					<< (type->genre == GenreType::REFERENCE) << ",\n";
			os_decl << "};\n";

			break;
		}
		case GenreType::ENUM:
		{
			auto type_enum = static_cast<TypeEnum *>(type);

			cree_info_type_enum_C(
						os_decl,
						type_enum->nom,
						type_enum->decl,
						id_info_type,
						nom_info_type,
						type->taille_octet);

			break;
		}
		case GenreType::STRUCTURE:
		case GenreType::UNION:
		{
			auto type_struct = static_cast<TypeStructure *>(type);

			cree_info_type_structure_C(
						os_decl,
						contexte,
						generatrice,
						type_struct->nom,
						type_struct->decl,
						type,
						id_info_type,
						nom_info_type);

			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tabl = static_cast<TypeTableauFixe *>(type);
			auto type_deref = type_tabl->type_pointe;

			if (type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, type_deref, id_info_type);
			}

			os_decl << "static const InfoTypeTableau " << nom_info_type << " = {\n";
			os_decl << "\t.id = " << id_info_type.TABLEAU << ",\n";
			os_decl << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			os_decl << "\t.est_tableau_fixe = 1,\n";
			os_decl << "\t.taille_fixe = " << type_tabl->taille << ",\n";
			os_decl << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			os_decl << "};\n";
			break;
		}
		case GenreType::VARIADIQUE:
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			/* dans le cas des arguments variadics des fonctions externes */
			if (type_deref == nullptr) {
				os_decl << "static const InfoTypeTableau " << nom_info_type << " = {\n";
				os_decl << "\t.id = " << id_info_type.TABLEAU << ",\n";
				os_decl << "\t.taille_en_octet = 0,\n";
				os_decl << "\t.est_tableau_fixe = 0,\n";
				os_decl << "\t.taille_fixe = 0,\n";
				os_decl << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
				os_decl << "};\n";
			}
			else {
				if (type_deref->ptr_info_type == "") {
					type_deref->ptr_info_type = cree_info_type_C(contexte, generatrice, os_decl, type_deref, id_info_type);
				}

				os_decl << "static const InfoTypeTableau " << nom_info_type << " = {\n";
				os_decl << "\t.id = " << id_info_type.TABLEAU << ",\n";
				os_decl << "\t.taille_en_octet = " << type->taille_octet << ",\n";
				os_decl << "\t.est_tableau_fixe = 0,\n";
				os_decl << "\t.taille_fixe = 0,\n";
				os_decl << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
				os_decl << "};\n";
			}

			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = static_cast<TypeFonction *>(type);
			type->ptr_info_type = nom_info_type;

			POUR (type_fonc->types_entrees) {
				if (it->ptr_info_type != "") {
					continue;
				}

				cree_info_type_C(contexte, generatrice, os_decl, it, id_info_type);
			}

			POUR (type_fonc->types_sorties) {
				if (it->ptr_info_type != "") {
					continue;
				}

				cree_info_type_C(contexte, generatrice, os_decl, it, id_info_type);
			}

			/* crée tableau infos type pour les entrées */
			auto const nom_tableau_entrees = "__types_entree" + nom_info_type;

			os_decl << "static const InfoType *" << nom_tableau_entrees << "[] = {\n";

			POUR (type_fonc->types_entrees) {
				os_decl << "(InfoType *)&" << it->ptr_info_type << ",\n";
			}

			os_decl << "};\n";

			/* crée tableau infos type pour les sorties */
			auto const nom_tableau_sorties = "__types_sortie" + nom_info_type;

			os_decl << "static const InfoType *" << nom_tableau_sorties << "[] = {\n";

			POUR (type_fonc->types_sorties) {
				os_decl << "(InfoType *)&" << it->ptr_info_type << ",\n";
			}

			os_decl << "};\n";

			/* crée l'info type pour la fonction */
			os_decl << "static const InfoTypeFonction " << nom_info_type << " = {\n";
			os_decl << "\t.id = " << id_info_type.FONCTION << ",\n";
			os_decl << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			// À FAIRE : réusinage type
			//os_decl << "\t.est_coroutine = " << (type.type_base() == GenreLexeme::COROUT) << ",\n";
			os_decl << broye_nom_simple("\t.types_entrée = { .pointeur = ") << nom_tableau_entrees;
			os_decl << ", .taille = " << type_fonc->types_entrees.taille << " },\n";
			os_decl << broye_nom_simple("\t.types_sortie = { .pointeur = ") << nom_tableau_sorties;
			os_decl << ", .taille = " << type_fonc->types_sorties.taille << " },\n";
			os_decl << "};\n";

			break;
		}
		case GenreType::EINI:
		{
			cree_info_type_defaul_C(os_decl, id_info_type.EINI, nom_info_type, type->taille_octet);
			break;
		}
		case GenreType::RIEN:
		{
			cree_info_type_defaul_C(os_decl, id_info_type.RIEN, nom_info_type, 0);
			break;
		}
		case GenreType::CHAINE:
		{
			cree_info_type_defaul_C(os_decl, id_info_type.CHAINE, nom_info_type, type->taille_octet);
			break;
		}
	}

	if (type->ptr_info_type == "") {
		type->ptr_info_type = nom_info_type;
	}

	return nom_info_type;
}

dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::flux_chaine &os_decl,
		Type *type)
{
	auto id_info_type = IDInfoType();
	return cree_info_type_C(contexte, generatrice, os_decl, type, id_info_type);
}

/* À FAIRE : bouge ça d'ici */
dls::chaine chaine_valeur_enum(NoeudEnum *noeud_enum, const dls::vue_chaine_compacte &nom)
{
	for (auto i = 0; i < noeud_enum->desc.noms.taille; ++i) {
		if (noeud_enum->desc.noms[i] == kuri::chaine(nom)) {
			return dls::vers_chaine(noeud_enum->desc.valeurs[i]);
		}
	}

	return "0";
}
