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

#include "generatrice_code_c.hh"

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
		GeneratriceCodeC &generatrice,
		dls::chaine const &id_type,
		dls::chaine const &nom_info_type,
		unsigned taille_en_octet)
{
	generatrice << "static const InfoType " << nom_info_type << " = {\n";
	generatrice << "\t.id = " << id_type << ",\n";
	generatrice << "\t.taille_en_octet = " << taille_en_octet << '\n';
	generatrice << "};\n";
}

static auto cree_info_type_entier_C(
		GeneratriceCodeC &generatrice,
		unsigned taille_en_octet,
		bool est_signe,
		IDInfoType const &id_info_type,
		dls::chaine const &nom_info_type)
{
	generatrice << "static const InfoTypeEntier " << nom_info_type << " = {\n";
	generatrice << "\t.id = " << id_info_type.ENTIER << ",\n";
	generatrice << '\t' << broye_nom_simple(".est_signé = ") << est_signe << ",\n";
	generatrice << "\t.taille_en_octet = " << taille_en_octet << '\n';
	generatrice << "};\n";
}

static dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		Type *type,
		IDInfoType const &id_info_type);

static auto cree_info_type_structure_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::vue_chaine_compacte const &nom_struct,
		NoeudStruct *noeud_decl,
		Type *type,
		IDInfoType const &id_info_type,
		dls::chaine const &nom_info_type)
{
	auto const nom_broye = broye_nom_simple(nom_struct);

	/* prédéclare l'infotype au cas où la structure s'incluerait elle-même via
	 * un pointeur (p.e. listes chainées) */
	generatrice << "static const InfoTypeStructure " << nom_info_type << ";\n";

	/* met en place le 'pointeur' au cas où une structure s'incluerait elle-même
	 * via un pointeur */
	type->ptr_info_type = nom_info_type;

	if (noeud_decl->est_externe) {
		generatrice << "static const InfoTypeStructure " << nom_info_type << " = {\n";
		generatrice << "\t.id = " << id_info_type.STRUCTURE << ",\n";
		generatrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
		generatrice << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
		generatrice << "\t.membres = {.pointeur = 0, .taille = 0 },\n";
		generatrice << "};\n";
		return;
	}

	auto const nombre_membres = noeud_decl->bloc->membres.taille;

	POUR (noeud_decl->desc.membres) {
		if (it.type->ptr_info_type == "") {
			it.type->ptr_info_type = cree_info_type_C(
						contexte, generatrice, it.type, id_info_type);
		}
	}

	/* crée le tableau des données des membres */
	auto const nom_tableau_membre = "__info_type_membres" + nom_info_type;
	auto pointeurs = dls::tableau<dls::chaine>();

	POUR (noeud_decl->desc.membres) {
		auto nom_info_type_membre = "__info_type_membre" + dls::vers_chaine(index_info_type++);
		pointeurs.pousse(nom_info_type_membre);

		generatrice << "static const InfoTypeMembreStructure " << nom_info_type_membre << " = {\n";
		generatrice << "\t.nom = { .pointeur = \"" << it.nom << "\", .taille = " << it.nom.taille << " },\n";
		generatrice << "\t" << broye_nom_simple(".décalage = ") << it.decalage << ",\n";
		generatrice << "\t.id = (InfoType *)(&" << it.type->ptr_info_type << ")\n";
		generatrice << "};\n";
	}

	generatrice << "static const InfoTypeMembreStructure *" << nom_tableau_membre << "[] = {\n";
	for (auto &pointeur : pointeurs) {
		generatrice << "&" << pointeur << ",";
	}
	generatrice << "};\n";

	/* crée l'info pour la structure */
	generatrice << "static const InfoTypeStructure " << nom_info_type << " = {\n";
	generatrice << "\t.id = " << id_info_type.STRUCTURE << ",\n";
	generatrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
	generatrice << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
	generatrice << "\t.membres = {.pointeur = " << nom_tableau_membre << ", .taille = " << nombre_membres << "},\n";
	generatrice << "};\n";
}

static auto cree_info_type_enum_C(
		GeneratriceCodeC &generatrice,
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

	generatrice << "static const chaine " << nom_tableau_noms << "[] = {\n";

	POUR (noeud_decl->desc.noms) {
		generatrice << "\t{.pointeur=\"" << it << "\", .taille=" << it.taille << "},\n";
	}

	generatrice << "};\n";

	/* crée le tableau pour les valeurs */
	auto const nom_tableau_valeurs = "__tableau_valeurs_enum" + nom_info_type;

	generatrice << "static const int " << nom_tableau_valeurs << "[] = {\n\t";

	POUR (noeud_decl->desc.valeurs) {
		generatrice << it << ',';
	}

	generatrice << "\n};\n";

	/* crée l'info type pour l'énum */
	generatrice << "static const " << broye_nom_simple("InfoTypeÉnum ") << nom_info_type << " = {\n";
	generatrice << "\t.id = " << id_info_type.ENUM << ",\n";
	generatrice << "\t.taille_en_octet = " << taille_octet << ",\n";
	generatrice << "\t.est_drapeau = " << noeud_decl->desc.est_drapeau << ",\n";
	generatrice << "\t.nom = { .pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << " },\n";
	generatrice << "\t.noms = { .pointeur = " << nom_tableau_noms << ", .taille = " << nombre_valeurs << " },\n ";
	generatrice << "\t.valeurs = { .pointeur = " << nom_tableau_valeurs << ", .taille = " << nombre_valeurs << " },\n ";
	generatrice << "};\n";
}

dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		Type *type,
		IDInfoType const &id_info_type)
{
	if (type->ptr_info_type != "") {
		return type->ptr_info_type;
	}

	auto nom_info_type = "__info_type" + nom_broye_type(type, false) + dls::vers_chaine(type);

	if (type->genre == GenreType::ENTIER_CONSTANT) {
		type->ptr_info_type = cree_info_type_C(contexte, generatrice, contexte.typeuse[TypeBase::Z32], id_info_type);
	}
	else {
		type->ptr_info_type = nom_info_type;
	}

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			assert(false);
			break;
		}
		case GenreType::BOOL:
		{
			cree_info_type_defaul_C(generatrice, id_info_type.BOOLEEN, nom_info_type, 1);
			break;
		}
		case GenreType::OCTET:
		{
			cree_info_type_defaul_C(generatrice, id_info_type.OCTET, nom_info_type, 1);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			//cree_info_type_entier_C(generatrice, 4, true, id_info_type, nom_info_type);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			cree_info_type_entier_C(generatrice, type->taille_octet, false, id_info_type, nom_info_type);
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			cree_info_type_entier_C(generatrice, type->taille_octet, true, id_info_type, nom_info_type);
			break;
		}
		case GenreType::REEL:
		{
			cree_info_type_defaul_C(generatrice, id_info_type.REEL, nom_info_type, type->taille_octet);
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			if (type_deref && type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, generatrice, type_deref, id_info_type);
			}

			generatrice << "static const InfoTypePointeur " << nom_info_type << " = {\n";
			generatrice << ".id = " << id_info_type.POINTEUR << ",\n";
			generatrice << ".taille_en_octet = " << type->taille_octet << ",\n";
			if (type_deref) {
				generatrice << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			}
			else {
				generatrice << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
			}
			generatrice << '\t' << broye_nom_simple(".est_référence = ")
					<< (type->genre == GenreType::REFERENCE) << ",\n";
			generatrice << "};\n";

			break;
		}
		case GenreType::ENUM:
		{
			auto type_enum = static_cast<TypeEnum *>(type);

			cree_info_type_enum_C(
						generatrice,
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
				type_deref->ptr_info_type = cree_info_type_C(contexte, generatrice, type_deref, id_info_type);
			}

			generatrice << "static const InfoTypeTableau " << nom_info_type << " = {\n";
			generatrice << "\t.id = " << id_info_type.TABLEAU << ",\n";
			generatrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			generatrice << "\t.est_tableau_fixe = 1,\n";
			generatrice << "\t.taille_fixe = " << type_tabl->taille << ",\n";
			generatrice << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			generatrice << "};\n";
			break;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_var = static_cast<TypeVariadique *>(type);

			if (type_var->type_pointe == nullptr) {
				generatrice << "static const InfoTypeTableau " << nom_info_type << " = {\n";
				generatrice << "\t.id = " << id_info_type.TABLEAU << ",\n";
				generatrice << "\t.taille_en_octet = 0,\n";
				generatrice << "\t.est_tableau_fixe = 0,\n";
				generatrice << "\t.taille_fixe = 0,\n";
				generatrice << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
				generatrice << "};\n";
			}
			else {
				auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_var->type_pointe);
				type_var->ptr_info_type = cree_info_type_C(contexte, generatrice, type_tabl, id_info_type);
			}

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			/* dans le cas des arguments variadics des fonctions externes */
			if (type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, generatrice, type_deref, id_info_type);
			}

			generatrice << "static const InfoTypeTableau " << nom_info_type << " = {\n";
			generatrice << "\t.id = " << id_info_type.TABLEAU << ",\n";
			generatrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			generatrice << "\t.est_tableau_fixe = 0,\n";
			generatrice << "\t.taille_fixe = 0,\n";
			generatrice << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			generatrice << "};\n";

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

				cree_info_type_C(contexte, generatrice, it, id_info_type);
			}

			POUR (type_fonc->types_sorties) {
				if (it->ptr_info_type != "") {
					continue;
				}

				cree_info_type_C(contexte, generatrice, it, id_info_type);
			}

			/* crée tableau infos type pour les entrées */
			auto const nom_tableau_entrees = "__types_entree" + nom_info_type + dls::vers_chaine(index_info_type++);

			generatrice << "static const InfoType *" << nom_tableau_entrees << "[] = {\n";

			POUR (type_fonc->types_entrees) {
				generatrice << "(InfoType *)&" << it->ptr_info_type << ",\n";
			}

			generatrice << "};\n";

			/* crée tableau infos type pour les sorties */
			auto const nom_tableau_sorties = "__types_sortie" + nom_info_type + dls::vers_chaine(index_info_type++);

			generatrice << "static const InfoType *" << nom_tableau_sorties << "[] = {\n";

			POUR (type_fonc->types_sorties) {
				generatrice << "(InfoType *)&" << it->ptr_info_type << ",\n";
			}

			generatrice << "};\n";

			/* crée l'info type pour la fonction */
			generatrice << "static const InfoTypeFonction " << nom_info_type << " = {\n";
			generatrice << "\t.id = " << id_info_type.FONCTION << ",\n";
			generatrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			// À FAIRE : réusinage type
			//os_decl << "\t.est_coroutine = " << (type.type_base() == GenreLexeme::COROUT) << ",\n";
			generatrice << broye_nom_simple("\t.types_entrée = { .pointeur = ") << nom_tableau_entrees;
			generatrice << ", .taille = " << type_fonc->types_entrees.taille << " },\n";
			generatrice << broye_nom_simple("\t.types_sortie = { .pointeur = ") << nom_tableau_sorties;
			generatrice << ", .taille = " << type_fonc->types_sorties.taille << " },\n";
			generatrice << "};\n";

			break;
		}
		case GenreType::EINI:
		{
			cree_info_type_defaul_C(generatrice, id_info_type.EINI, nom_info_type, type->taille_octet);
			break;
		}
		case GenreType::RIEN:
		{
			cree_info_type_defaul_C(generatrice, id_info_type.RIEN, nom_info_type, 0);
			break;
		}
		case GenreType::CHAINE:
		{
			cree_info_type_defaul_C(generatrice, id_info_type.CHAINE, nom_info_type, type->taille_octet);
			break;
		}
	}

	return nom_info_type;
}

dls::chaine predeclare_info_type_C(
		GeneratriceCodeC &generatrice,
		Type *type)
{
	auto nom_info_type = "__info_type" + nom_broye_type(type, false) + dls::vers_chaine(type);

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			assert(false);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			break;
		}
		case GenreType::REEL:
		case GenreType::OCTET:
		case GenreType::EINI:
		case GenreType::RIEN:
		case GenreType::CHAINE:
		case GenreType::BOOL:
		{
			generatrice << "static const InfoType " << nom_info_type << ";\n";
			break;
		}
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		{
			generatrice << "static const InfoTypeEntier " << nom_info_type << ";\n";
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			generatrice << "static const InfoTypePointeur " << nom_info_type << ";\n";
			break;
		}
		case GenreType::ENUM:
		{
			generatrice << "static const InfoTypexC3x89num " << nom_info_type << ";\n";
			break;
		}
		case GenreType::STRUCTURE:
		case GenreType::UNION:
		{
			generatrice << "static const InfoTypeStructure " << nom_info_type << ";\n";
			break;
		}
		case GenreType::TABLEAU_FIXE:
		case GenreType::VARIADIQUE:
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			generatrice << "static const InfoTypeTableau " << nom_info_type << ";\n";
			break;
		}
		case GenreType::FONCTION:
		{
			generatrice << "static const InfoTypeFonction " << nom_info_type << ";\n";
			break;
		}
	}

	return nom_info_type;
}

dls::chaine cree_info_type_C(ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		Type *type)
{
	auto id_info_type = IDInfoType();
	return cree_info_type_C(contexte, generatrice, type, id_info_type);
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
