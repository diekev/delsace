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

#include "constructrice_code_c.hh"

/* À tenir synchronisé avec l'énum dans info_type.kuri
 * Nous utilisons ceci lors de la génération du code des infos types car nous ne
 * générons pas de code (ou symboles) pour les énums, mais prenons directements
 * leurs valeurs.
 */
struct IDInfoType {
	static constexpr const char *ENTIER    = "0";
	static constexpr const char *REEL      = "1";
	static constexpr const char *BOOLEEN   = "2";
	static constexpr const char *CHAINE    = "3";
	static constexpr const char *POINTEUR  = "4";
	static constexpr const char *STRUCTURE = "5";
	static constexpr const char *FONCTION  = "6";
	static constexpr const char *TABLEAU   = "7";
	static constexpr const char *EINI      = "8";
	static constexpr const char *RIEN      = "9";
	static constexpr const char *ENUM      = "10";
	static constexpr const char *OCTET     = "11";
};

static auto cree_info_type_defaut_C(
		ConstructriceCodeC &constructrice,
		const char *id_type,
		dls::chaine const &nom_info_type,
		unsigned taille_en_octet)
{
	constructrice << "static const InfoType " << nom_info_type << " = {\n";
	constructrice << "\t.id = " << id_type << ",\n";
	constructrice << "\t.taille_en_octet = " << taille_en_octet << '\n';
	constructrice << "};\n";
}

static auto cree_info_type_entier_C(
		ConstructriceCodeC &constructrice,
		unsigned taille_en_octet,
		bool est_signe,
		dls::chaine const &nom_info_type)
{
	constructrice << "static const InfoTypeEntier " << nom_info_type << " = {\n";
	constructrice << "\t.id = " << IDInfoType::ENTIER << ",\n";
	constructrice << '\t' << broye_nom_simple(".est_signé = ") << est_signe << ",\n";
	constructrice << "\t.taille_en_octet = " << taille_en_octet << '\n';
	constructrice << "};\n";
}

static auto cree_info_type_structure_C(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		dls::vue_chaine_compacte const &nom_struct,
		NoeudStruct *noeud_decl,
		Type *type,
		dls::chaine const &nom_info_type)
{
	auto const nom_broye = broye_nom_simple(nom_struct);

	/* prédéclare l'infotype au cas où la structure s'incluerait elle-même via
	 * un pointeur (p.e. listes chainées) */
	constructrice << "static const InfoTypeStructure " << nom_info_type << ";\n";

	/* met en place le 'pointeur' au cas où une structure s'incluerait elle-même
	 * via un pointeur */
	type->ptr_info_type = nom_info_type;

	if (noeud_decl->est_externe) {
		constructrice << "static const InfoTypeStructure " << nom_info_type << " = {\n";
		constructrice << "\t.id = " << IDInfoType::STRUCTURE << ",\n";
		constructrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
		constructrice << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
		constructrice << "\t.membres = {.pointeur = 0, .taille = 0 },\n";
		constructrice << "};\n";
		return;
	}

	auto const nombre_membres = noeud_decl->bloc->membres.taille;

	POUR (noeud_decl->desc.membres) {
		if (it.type->ptr_info_type == "") {
			it.type->ptr_info_type = cree_info_type_C(
						contexte, constructrice, it.type);
		}
	}

	/* crée le tableau des données des membres */
	auto const nom_tableau_membre = "__info_type_membres" + nom_info_type;
	auto pointeurs = dls::tableau<dls::chaine>();

	auto index_membre = 0;

	POUR (noeud_decl->desc.membres) {
		auto nom_info_type_membre = "__info_type_membre" + nom_info_type + dls::vers_chaine(index_membre++);
		pointeurs.pousse(nom_info_type_membre);

		constructrice << "static const InfoTypeMembreStructure " << nom_info_type_membre << " = {\n";
		constructrice << "\t.nom = { .pointeur = \"" << it.nom << "\", .taille = " << it.nom.taille << " },\n";
		constructrice << "\t" << broye_nom_simple(".décalage = ") << it.decalage << ",\n";
		constructrice << "\t.id = (InfoType *)(&" << it.type->ptr_info_type << ")\n";
		constructrice << "};\n";
	}

	constructrice << "static const InfoTypeMembreStructure *" << nom_tableau_membre << "[] = {\n";
	for (auto &pointeur : pointeurs) {
		constructrice << "&" << pointeur << ",";
	}
	constructrice << "};\n";

	/* crée l'info pour la structure */
	constructrice << "static const InfoTypeStructure " << nom_info_type << " = {\n";
	constructrice << "\t.id = " << IDInfoType::STRUCTURE << ",\n";
	constructrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
	constructrice << "\t.nom = {.pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << "},\n";
	constructrice << "\t.membres = {.pointeur = " << nom_tableau_membre << ", .taille = " << nombre_membres << "},\n";
	constructrice << "};\n";
}

static auto cree_info_type_enum_C(
		ConstructriceCodeC &constructrice,
		dls::vue_chaine_compacte const &nom_struct,
		NoeudEnum *noeud_decl,
		dls::chaine const &nom_info_type,
		unsigned taille_octet)
{
	auto nom_broye = broye_nom_simple(nom_struct);
	auto nombre_valeurs = noeud_decl->desc.noms.taille;

	/* crée un tableau pour les noms des énumérations */
	auto const nom_tableau_noms = "__tableau_noms_enum" + nom_info_type;

	constructrice << "static const chaine " << nom_tableau_noms << "[] = {\n";

	POUR (noeud_decl->desc.noms) {
		constructrice << "\t{.pointeur=\"" << it << "\", .taille=" << it.taille << "},\n";
	}

	constructrice << "};\n";

	/* crée le tableau pour les valeurs */
	auto const nom_tableau_valeurs = "__tableau_valeurs_enum" + nom_info_type;

	constructrice << "static const int " << nom_tableau_valeurs << "[] = {\n\t";

	POUR (noeud_decl->desc.valeurs) {
		constructrice << it << ',';
	}

	constructrice << "\n};\n";

	/* crée l'info type pour l'énum */
	constructrice << "static const " << broye_nom_simple("InfoTypeÉnum ") << nom_info_type << " = {\n";
	constructrice << "\t.id = " << IDInfoType::ENUM << ",\n";
	constructrice << "\t.taille_en_octet = " << taille_octet << ",\n";
	constructrice << "\t.est_drapeau = " << noeud_decl->desc.est_drapeau << ",\n";
	constructrice << "\t.nom = { .pointeur = \"" << nom_struct << "\", .taille = " << nom_struct.taille() << " },\n";
	constructrice << "\t.noms = { .pointeur = " << nom_tableau_noms << ", .taille = " << nombre_valeurs << " },\n ";
	constructrice << "\t.valeurs = { .pointeur = " << nom_tableau_valeurs << ", .taille = " << nombre_valeurs << " },\n ";
	constructrice << "};\n";
}

dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		Type *type)
{
	if (type->ptr_info_type != "") {
		return type->ptr_info_type;
	}

	auto nom_info_type = "__info_type" + nom_broye_type(type) + dls::vers_chaine(type);

	if (type->genre == GenreType::ENTIER_CONSTANT) {
		type->ptr_info_type = cree_info_type_C(contexte, constructrice, contexte.typeuse[TypeBase::Z32]);
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
			cree_info_type_defaut_C(constructrice, IDInfoType::BOOLEEN, nom_info_type, 1);
			break;
		}
		case GenreType::OCTET:
		{
			cree_info_type_defaut_C(constructrice, IDInfoType::OCTET, nom_info_type, 1);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			//cree_info_type_entier_C(constructrice, 4, true, id_info_type, nom_info_type);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			cree_info_type_entier_C(constructrice, type->taille_octet, false, nom_info_type);
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			cree_info_type_entier_C(constructrice, type->taille_octet, true, nom_info_type);
			break;
		}
		case GenreType::REEL:
		{
			cree_info_type_defaut_C(constructrice, IDInfoType::REEL, nom_info_type, type->taille_octet);
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			if (type_deref && type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, constructrice, type_deref);
			}

			constructrice << "static const InfoTypePointeur " << nom_info_type << " = {\n";
			constructrice << ".id = " << IDInfoType::POINTEUR << ",\n";
			constructrice << ".taille_en_octet = " << type->taille_octet << ",\n";
			if (type_deref) {
				constructrice << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			}
			else {
				constructrice << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
			}
			constructrice << '\t' << broye_nom_simple(".est_référence = ")
					<< (type->genre == GenreType::REFERENCE) << ",\n";
			constructrice << "};\n";

			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);

			cree_info_type_enum_C(
						constructrice,
						type_enum->nom,
						type_enum->decl,
						nom_info_type,
						type->taille_octet);

			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);

			cree_info_type_structure_C(
						contexte,
						constructrice,
						type_struct->nom,
						type_struct->decl,
						type,
						nom_info_type);

			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = static_cast<TypeUnion *>(type);

			cree_info_type_structure_C(
						contexte,
						constructrice,
						type_struct->nom,
						type_struct->decl,
						type,
						nom_info_type);

			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tabl = static_cast<TypeTableauFixe *>(type);
			auto type_deref = type_tabl->type_pointe;

			if (type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, constructrice, type_deref);
			}

			constructrice << "static const InfoTypeTableau " << nom_info_type << " = {\n";
			constructrice << "\t.id = " << IDInfoType::TABLEAU << ",\n";
			constructrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			constructrice << "\t.est_tableau_fixe = 1,\n";
			constructrice << "\t.taille_fixe = " << type_tabl->taille << ",\n";
			constructrice << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			constructrice << "};\n";
			break;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_var = static_cast<TypeVariadique *>(type);

			if (type_var->type_pointe == nullptr) {
				constructrice << "static const InfoTypeTableau " << nom_info_type << " = {\n";
				constructrice << "\t.id = " << IDInfoType::TABLEAU << ",\n";
				constructrice << "\t.taille_en_octet = 0,\n";
				constructrice << "\t.est_tableau_fixe = 0,\n";
				constructrice << "\t.taille_fixe = 0,\n";
				constructrice << '\t' << broye_nom_simple(".type_pointé") << " = 0,\n";
				constructrice << "};\n";
			}
			else {
				auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_var->type_pointe);
				type_var->ptr_info_type = cree_info_type_C(contexte, constructrice, type_tabl);
			}

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			/* dans le cas des arguments variadics des fonctions externes */
			if (type_deref->ptr_info_type == "") {
				type_deref->ptr_info_type = cree_info_type_C(contexte, constructrice, type_deref);
			}

			constructrice << "static const InfoTypeTableau " << nom_info_type << " = {\n";
			constructrice << "\t.id = " << IDInfoType::TABLEAU << ",\n";
			constructrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			constructrice << "\t.est_tableau_fixe = 0,\n";
			constructrice << "\t.taille_fixe = 0,\n";
			constructrice << '\t' << broye_nom_simple(".type_pointé") << " = (InfoType *)(&" << type_deref->ptr_info_type << "),\n";
			constructrice << "};\n";

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

				cree_info_type_C(contexte, constructrice, it);
			}

			POUR (type_fonc->types_sorties) {
				if (it->ptr_info_type != "") {
					continue;
				}

				cree_info_type_C(contexte, constructrice, it);
			}

			/* crée tableau infos type pour les entrées */
			auto const nom_tableau_entrees = "__types_entree" + nom_info_type;

			constructrice << "static const InfoType *" << nom_tableau_entrees << "[] = {\n";

			POUR (type_fonc->types_entrees) {
				constructrice << "(InfoType *)&" << it->ptr_info_type << ",\n";
			}

			constructrice << "};\n";

			/* crée tableau infos type pour les sorties */
			auto const nom_tableau_sorties = "__types_sortie" + nom_info_type;

			constructrice << "static const InfoType *" << nom_tableau_sorties << "[] = {\n";

			POUR (type_fonc->types_sorties) {
				constructrice << "(InfoType *)&" << it->ptr_info_type << ",\n";
			}

			constructrice << "};\n";

			/* crée l'info type pour la fonction */
			constructrice << "static const InfoTypeFonction " << nom_info_type << " = {\n";
			constructrice << "\t.id = " << IDInfoType::FONCTION << ",\n";
			constructrice << "\t.taille_en_octet = " << type->taille_octet << ",\n";
			// À FAIRE : réusinage type
			//os_decl << "\t.est_coroutine = " << (type.type_base() == GenreLexeme::COROUT) << ",\n";
			constructrice << broye_nom_simple("\t.types_entrée = { .pointeur = ") << nom_tableau_entrees;
			constructrice << ", .taille = " << type_fonc->types_entrees.taille << " },\n";
			constructrice << broye_nom_simple("\t.types_sortie = { .pointeur = ") << nom_tableau_sorties;
			constructrice << ", .taille = " << type_fonc->types_sorties.taille << " },\n";
			constructrice << "};\n";

			break;
		}
		case GenreType::EINI:
		{
			cree_info_type_defaut_C(constructrice, IDInfoType::EINI, nom_info_type, type->taille_octet);
			break;
		}
		case GenreType::RIEN:
		{
			cree_info_type_defaut_C(constructrice, IDInfoType::RIEN, nom_info_type, 0);
			break;
		}
		case GenreType::CHAINE:
		{
			cree_info_type_defaut_C(constructrice, IDInfoType::CHAINE, nom_info_type, type->taille_octet);
			break;
		}
	}

	return nom_info_type;
}

dls::chaine predeclare_info_type_C(
		ConstructriceCodeC &constructrice,
		Type *type)
{
	auto nom_info_type = "__info_type" + nom_broye_type(type) + dls::vers_chaine(type);

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
			constructrice << "static const InfoType " << nom_info_type << ";\n";
			break;
		}
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		{
			constructrice << "static const InfoTypeEntier " << nom_info_type << ";\n";
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			constructrice << "static const InfoTypePointeur " << nom_info_type << ";\n";
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			constructrice << "static const InfoTypexC3x89num " << nom_info_type << ";\n";
			break;
		}
		case GenreType::STRUCTURE:
		case GenreType::UNION:
		{
			constructrice << "static const InfoTypeStructure " << nom_info_type << ";\n";
			break;
		}
		case GenreType::TABLEAU_FIXE:
		case GenreType::VARIADIQUE:
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			constructrice << "static const InfoTypeTableau " << nom_info_type << ";\n";
			break;
		}
		case GenreType::FONCTION:
		{
			constructrice << "static const InfoTypeFonction " << nom_info_type << ";\n";
			break;
		}
	}

	return nom_info_type;
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
