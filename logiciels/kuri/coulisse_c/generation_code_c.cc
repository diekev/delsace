﻿/*
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

#include "generation_code_c.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/conditions.h"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "conversion_type_c.hh"
#include "erreur.h"
#include "constructrice_code_c.hh"
#include "info_type_c.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
#include "portee.hh"
#include "typage.hh"

#include "representation_intermediaire/constructrice_ri.hh"

namespace noeud {

/* ************************************************************************** */

static inline auto broye_chaine(NoeudExpression *b)
{
	return broye_nom_simple(b->ident->nom);
}

static inline auto broye_chaine(dls::vue_chaine_compacte const &chn)
{
	return broye_nom_simple(chn);
}

static inline auto broye_chaine(kuri::chaine const &chn)
{
	return broye_nom_simple(dls::vue_chaine_compacte(chn.pointeur, chn.taille));
}

/* ************************************************************************** */

/* À FAIRE : trouve une bonne manière de générer des noms uniques. */
static int index = 0;

static void debute_record_trace_appel(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		NoeudExpression *expr)
{
	if (dls::outils::possede_drapeau(contexte.donnees_fonction->drapeaux, FORCE_SANSTRACE)) {
		return;
	}

	auto const &lexeme = expr->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = position_lexeme(*lexeme);

	constructrice << "DEBUTE_RECORD_TRACE_APPEL(";
	constructrice << pos.numero_ligne << ",";
	constructrice << pos.pos << ",";
	constructrice << "\"";

	auto ligne = fichier->tampon[pos.index_ligne];

	POUR (ligne) {
		if (it == '\n') {
			continue;
		}

		if (it == '"') {
			constructrice.m_enchaineuse.pousse_caractere('\\');
		}

		constructrice.m_enchaineuse.pousse_caractere(it);
	}

	constructrice << "\", " << ligne.taille();
	constructrice << ");\n";
}

static void termine_record_trace_appel(ConstructriceCodeC &constructrice)
{
	constructrice << "TERMINE_RECORD_TRACE_APPEL;\n";
}

void genere_code_C(
		NoeudExpression *b,
		ConstructriceCodeC &constructrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche);

static void applique_transformation(
		NoeudExpression *b,
		ConstructriceCodeC &constructrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	/* force une expression gauche afin de ne pas prendre la référence d'une
	 * variable temporaire */
	expr_gauche |= b->transformation.type == TypeTransformation::PREND_REFERENCE;
	expr_gauche |= b->transformation.type == TypeTransformation::CONSTRUIT_EINI;
	genere_code_C(b, constructrice, contexte, expr_gauche);

	auto nom_courant = b->chaine_calculee();
	auto type = b->type;

	auto nom_var_temp = "__var_temp_conv" + dls::vers_chaine(index++);

	switch (b->transformation.type) {
		case TypeTransformation::IMPOSSIBLE:
		{
			break;
		}
		case TypeTransformation::CONSTRUIT_UNION:
		{
			// KsUnion __var_temp;
			// __var_temp.membre = b->chaine_calculee();
			// __var_temp.membre_actif = idx + 1;

			auto index_membre = b->transformation.index_membre;
			auto type_union = static_cast<TypeUnion *>(b->transformation.type_cible);
			auto &membre = type_union->membres[index_membre];

			constructrice.declare_variable(type_union, nom_var_temp, "");
			constructrice << nom_var_temp << "." << broye_chaine(membre.nom);
			constructrice << " = " << nom_courant << ";\n";

			if (!type_union->est_nonsure) {
				constructrice << nom_var_temp << ".membre_actif = " << index_membre + 1 << ";\n";
			}

			break;
		}
		case TypeTransformation::EXTRAIT_UNION:
		{
			auto type_cible = b->transformation.type_cible;
			auto index_membre = b->transformation.index_membre;

			auto type_union = static_cast<TypeUnion *>(type);
			auto &membre = type_union->membres[index_membre];

			// À FAIRE : nous pourrions avoir une erreur différente ici.
			constructrice << "if (" << nom_courant << ".membre_actif != " << index_membre + 1 << ") {\n";
			debute_record_trace_appel(contexte, constructrice, b);
			constructrice << contexte.interface_kuri.decl_panique_membre_union->nom_broye << "(contexte);\n";
			constructrice << "}\n";

			constructrice << nom_broye_type(type_cible) << " " << nom_var_temp
						<< " = " << nom_courant << "." << broye_chaine(membre.nom)
						<< ";\n";

			break;
		}
		case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
		case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
		case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
		case TypeTransformation::INUTILE:
		case TypeTransformation::AUGMENTE_TAILLE_TYPE:
		{
			nom_var_temp = nom_courant;
			break;
		}
		case TypeTransformation::CONSTRUIT_EINI:
		{
			/* dans le cas d'un nombre ou d'une chaine, etc. */
			if (!est_valeur_gauche(b->genre_valeur)) {
				constructrice.declare_variable(type, nom_var_temp, b->chaine_calculee());
				nom_courant = nom_var_temp;
				nom_var_temp = "__var_temp_eini" + dls::vers_chaine(index++);
			}

			constructrice << "Kseini " << nom_var_temp << " = {\n";
			constructrice << "\t.pointeur = &" << nom_courant << ",\n";
			constructrice << "\t.info = (InfoType *)(&" << type->ptr_info_type << ")\n";
			constructrice << "};\n";

			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			constructrice << nom_broye_type(b->transformation.type_cible) << " " << nom_var_temp << " = *(";
			constructrice << nom_broye_type(b->transformation.type_cible) << " *)(" << nom_courant << ".pointeur);\n";
			break;
		}
		case TypeTransformation::CONSTRUIT_TABL_OCTET:
		{
			/* dans le cas d'un nombre ou d'un tableau, etc. */
			if (!est_valeur_gauche(b->genre_valeur)) {
				constructrice.declare_variable(type, nom_var_temp, b->chaine_calculee());
				nom_courant = nom_var_temp;
				nom_var_temp = "__var_temp_eini" + dls::vers_chaine(index++);
			}

			constructrice << "KtKsoctet " << nom_var_temp << " = {\n";

			switch (type->genre) {
				default:
				{
					constructrice << "\t.pointeur = (unsigned char *)(&" << nom_courant << "),\n";
					constructrice << "\t.taille = sizeof(" << nom_broye_type(type) << ")\n";
					break;
				}
				case GenreType::POINTEUR:
				{
					constructrice << "\t.pointeur = (unsigned char *)(" << nom_courant << "),\n";
					constructrice << "\t.taille = sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					constructrice << nom_broye_type(type_deref) << ")\n";
					break;
				}
				case GenreType::CHAINE:
				{
					constructrice << "\t.pointeur = " << nom_courant << ".pointeur,\n";
					constructrice << "\t.taille = " << nom_courant << ".taille,\n";
					break;
				}
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					constructrice << "\t.pointeur = (unsigned char *)(&" << nom_courant << ".pointeur),\n";
					constructrice << "\t.taille = " << nom_courant << ".taille * sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					constructrice << nom_broye_type(type_deref) << ")\n";
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					constructrice << "\t.pointeur = " << nom_courant << ",\n";
					constructrice << "\t.taille = " << static_cast<TypeTableauFixe *>(type)->taille << " * sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					constructrice << nom_broye_type(type_deref) << ")\n";
					break;
				}
			}

			constructrice << "};\n";

			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);
			auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_deref);
			constructrice << nom_broye_type(type_tabl) << ' ' << nom_var_temp << " = {\n";
			constructrice << "\t.taille = " << static_cast<TypeTableauFixe *>(type)->taille << ",\n";
			constructrice << "\t.pointeur = " << nom_courant << "\n";
			constructrice << "};";

			break;
		}
		case TypeTransformation::FONCTION:
		{
			constructrice << nom_broye_type(b->transformation.type_cible) << ' ';
			constructrice << nom_var_temp << " = " << b->transformation.nom_fonction << '(';
			constructrice << nom_courant << ");\n";

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
				auto type_deref = contexte.typeuse.type_dereference_pour(type);
				constructrice << nom_broye_type(type_deref);
				constructrice << ' ' << nom_var_temp << " = *(" << nom_courant << ");\n";
			}

			break;
		}
		case TypeTransformation::CONVERTI_VERS_BASE:
		{
			//constructrice.declare_variable(b->transformation.type_cible, nom_var_temp, b->chaine_calculee());
			nom_var_temp = nom_courant;
			break;
		}
	}

	b->valeur_calculee = nom_var_temp;
}

static void genere_code_extra_pre_retour(
		NoeudBloc *bloc,
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice)
{
	if (contexte.donnees_fonction->est_coroutine) {
		constructrice << "__etat->__termine_coro = 1;\n";
		constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
		constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	}

	/* génère le code pour les blocs déférés */
	while (bloc != nullptr) {
		for (auto i = bloc->noeuds_differes.taille - 1; i >= 0; --i) {
			auto bloc_differe = bloc->noeuds_differes[i];
			bloc_differe->est_differe = false;
			genere_code_C(bloc_differe, constructrice, contexte, false);
			bloc_differe->est_differe = true;
		}

		bloc = bloc->parent;
	}
}

/* ************************************************************************** */

static void cree_appel(
		NoeudExpression *b,
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice)
{
	auto expr_appel = static_cast<NoeudExpressionAppel *>(b);
	auto decl_fonction_appelee = static_cast<NoeudDeclarationFonction const *>(expr_appel->noeud_fonction_appelee);

	auto pour_appel_precedent = contexte.pour_appel;
	contexte.pour_appel = static_cast<NoeudExpressionAppel *>(b);
	POUR (expr_appel->exprs) {
		applique_transformation(it, constructrice, contexte, false);
	}
	contexte.pour_appel = pour_appel_precedent;

	auto type = b->type;

	dls::liste<NoeudExpression *> liste_var_retour{};
	dls::tableau<dls::chaine> liste_noms_retour{};

	debute_record_trace_appel(contexte, constructrice, expr_appel);

	if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET) {
		liste_var_retour = std::any_cast<dls::liste<NoeudExpression *>>(b->valeur_calculee);
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");

		for (auto n : liste_var_retour) {
			genere_code_C(n, constructrice, contexte, false);
		}
	}
	else if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET2) {
		liste_noms_retour = std::any_cast<dls::tableau<dls::chaine>>(b->valeur_calculee);
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}
	else if (type->genre != GenreType::RIEN && (b->aide_generation_code == APPEL_POINTEUR_FONCTION || ((decl_fonction_appelee != nullptr) && !decl_fonction_appelee->est_coroutine))) {
		auto nom_indirection = "__ret" + dls::vers_chaine(b);
		constructrice << nom_broye_type(type) << ' ' << nom_indirection << " = ";
		b->valeur_calculee = nom_indirection;
	}
	else {
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}

	if (b->aide_generation_code == APPEL_POINTEUR_FONCTION) {
		genere_code_C(expr_appel->appelee, constructrice, contexte, true);
		constructrice << expr_appel->appelee->chaine_calculee();
	}
	else {
		constructrice << decl_fonction_appelee->nom_broye;
	}

	auto virgule = '(';

	if (expr_appel->params.taille == 0 && liste_var_retour.est_vide() && liste_noms_retour.est_vide()) {
		constructrice << virgule;
		virgule = ' ';
	}

	if (decl_fonction_appelee != nullptr) {
		if (decl_fonction_appelee->est_coroutine) {
			constructrice << virgule << "&__etat" << dls::vers_chaine(b) << ");\n";
			return;
		}

		if (!decl_fonction_appelee->est_externe && !dls::outils::possede_drapeau(decl_fonction_appelee->drapeaux, FORCE_NULCTX)) {
			constructrice << virgule;
			constructrice << "contexte";
			virgule = ',';
		}
	}
	else {
		if (!dls::outils::possede_drapeau(expr_appel->drapeaux, FORCE_NULCTX)) {
			constructrice << virgule;
			constructrice << "contexte";
			virgule = ',';
		}
	}

	POUR (expr_appel->exprs) {
		constructrice << virgule;
		constructrice << it->chaine_calculee();
		virgule = ',';
	}

	for (auto n : liste_var_retour) {
		constructrice << virgule;
		constructrice << "&(" << n->chaine_calculee() << ')';
		virgule = ',';
	}

	for (auto n : liste_noms_retour) {
		constructrice << virgule << n;
		virgule = ',';
	}

	constructrice << ");\n";

	if (!dls::outils::possede_drapeau(contexte.donnees_fonction->drapeaux, FORCE_SANSTRACE)) {
		termine_record_trace_appel(constructrice);
	}
}

static void genere_code_acces_membre(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		NoeudExpression *b,
		NoeudExpression *structure,
		NoeudExpression *membre,
		bool expr_gauche)
{
	auto flux = dls::flux_chaine();

	auto nom_acces = "__acces" + dls::vers_chaine(b);
	b->valeur_calculee = nom_acces;

	if (b->aide_generation_code == ACCEDE_MODULE) {
		genere_code_C(membre, constructrice, contexte, false);
		flux << membre->chaine_calculee();
	}
	else {
		auto type_structure = structure->type;
		auto est_reference = type_structure->genre == GenreType::REFERENCE;

		if (est_reference) {
			type_structure = contexte.typeuse.type_dereference_pour(type_structure);
		}

		auto est_pointeur = type_structure->genre == GenreType::POINTEUR;

		if (est_pointeur) {
			type_structure = contexte.typeuse.type_dereference_pour(type_structure);
		}

		if (type_structure->genre == GenreType::TABLEAU_DYNAMIQUE) {
			genere_code_C(structure, constructrice, contexte, expr_gauche);

			if (membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				genere_code_C(membre, constructrice, contexte, expr_gauche);
			}

			if (est_reference) {
				flux << "(*" << structure->chaine_calculee() << ')';
			}
			else {
				flux << structure->chaine_calculee();
			}

			flux << ((est_pointeur) ? "->" : ".");
			flux << broye_chaine(membre);
		}
		else if (type_structure->genre == GenreType::TABLEAU_FIXE) {
			auto taille_tableau = static_cast<TypeTableauFixe *>(type_structure)->taille;
			constructrice << "long " << nom_acces << " = " << taille_tableau << ";\n";
			flux << nom_acces;
		}
		else if (type_structure->genre == GenreType::ENUM || type_structure->genre == GenreType::ERREUR) {
			auto type_enum = static_cast<TypeEnum *>(type_structure);
			flux << chaine_valeur_enum(type_enum->decl, membre->ident->nom);
		}
		else if (type_structure->genre == GenreType::STRUCTURE) {
			genere_code_C(structure, constructrice, contexte, expr_gauche);

			if (est_reference) {
				flux << "(*" << structure->chaine_calculee() << ')';
			}
			else {
				flux << structure->chaine_calculee();
			}

			flux << ((est_pointeur) ? "->" : ".");

			if (membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				genere_code_C(membre, constructrice, contexte, expr_gauche);
			}

			flux << broye_chaine(membre);
		}
		else {
			genere_code_C(structure, constructrice, contexte, expr_gauche);

			if (membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				genere_code_C(membre, constructrice, contexte, expr_gauche);
			}

			if (est_reference) {
				flux << "(*" << structure->chaine_calculee() << ')';
			}
			else {
				flux << structure->chaine_calculee();
			}

			flux << ((est_pointeur) ? "->" : ".");
			flux << broye_chaine(membre->ident->nom);
		}
	}

	if (expr_gauche) {
		b->valeur_calculee = dls::chaine(flux.chn());
	}
	else {
		auto nom_var = "__var_temp_acc" + dls::vers_chaine(index++);
		constructrice << "const ";
		constructrice.declare_variable(b->type, nom_var, flux.chn());

		b->valeur_calculee = nom_var;
	}
}

static void genere_code_echec_logement(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		dls::chaine const &nom_ptr,
		NoeudExpression *b,
		NoeudExpression *bloc)
{
	constructrice << "if (" << nom_ptr  << " == 0)";

	if (bloc) {
		genere_code_C(bloc, constructrice, contexte, true);
	}
	else {
		constructrice << " {\n";
		debute_record_trace_appel(contexte, constructrice, b);
		constructrice << contexte.interface_kuri.decl_panique_memoire->nom_broye << "(contexte);\n";
		constructrice << "}\n";
	}
}

static void genere_declaration_structure(ConstructriceCodeC &constructrice, NoeudStruct *decl)
{
	if (decl->est_externe) {
		return;
	}

	auto nom_broye = broye_nom_simple(decl->ident->nom);

	if (decl->est_union) {
		if (decl->est_nonsure) {
			constructrice << "typedef union " << nom_broye << "{\n";
		}
		else {
			constructrice << "typedef struct " << nom_broye << "{\n";
			constructrice << "int membre_actif;\n";
			constructrice << "union {\n";
		}
	}
	else {
		constructrice << "typedef struct " << nom_broye << "{\n";
	}

	auto type_compose = static_cast<TypeCompose *>(decl->type);

	POUR (type_compose->membres) {
		auto nom = broye_chaine(it.nom);
		constructrice << nom_broye_type(it.type) << ' ' << nom << ";\n";
	}

	if (decl->est_union && !decl->est_nonsure) {
		constructrice << "};\n";
	}

	constructrice << "} " << nom_broye << ";\n\n";
}

static void cree_initialisation_structure(ContexteGenerationCode &contexte, ConstructriceCodeC &constructrice, Type *type, NoeudStruct *decl)
{
	constructrice << '\n';
	constructrice << "void initialise_" << type->nom_broye << "("
				   << type->nom_broye << " *pointeur)\n";
	constructrice << "{\n";

	auto type_compose = static_cast<TypeCompose *>(type);

	POUR (type_compose->membres) {
		auto type_membre = it.type;
		auto nom_broye_membre = broye_chaine(it.nom);

		if (it.expression_valeur_defaut != nullptr) {
			genere_code_C(it.expression_valeur_defaut, constructrice, contexte, false);
			constructrice << "pointeur->" << nom_broye_membre << " = ";
			constructrice << it.expression_valeur_defaut->chaine_calculee() << ";\n";
			continue;
		}

		if (type_membre->genre == GenreType::CHAINE) {
			constructrice << "pointeur->" << nom_broye_membre << ".pointeur = 0;\n";
			constructrice << "pointeur->" << nom_broye_membre << ".taille = 0;\n";
		}
		else if (type_membre->genre == GenreType::TABLEAU_DYNAMIQUE) {
			constructrice << "pointeur->" << nom_broye_membre << ".pointeur = 0;\n";
			constructrice << "pointeur->" << nom_broye_membre << ".taille = 0;\n";
			constructrice << "pointeur->" << nom_broye_membre << broye_nom_simple(".capacité") << " = 0;\n";
		}
		else if (type_membre->genre == GenreType::EINI) {
			constructrice << "pointeur->" << nom_broye_membre << ".pointeur = 0\n;";
			constructrice << "pointeur->" << nom_broye_membre << ".info = 0\n;";
		}
		else if (type_membre->genre == GenreType::ENUM) {
			constructrice << "pointeur->" << nom_broye_membre << " = 0;\n";
		}
		else if (type_membre->genre == GenreType::ERREUR) {
			constructrice << "pointeur->" << nom_broye_membre << " = 0;\n";
		}
		else if (type_membre->genre == GenreType::UNION) {
			auto type_union = static_cast<TypeUnion *>(type_membre);

			/* À FAIRE: initialise le type le plus large */
			if (!type_union->decl->est_nonsure) {
				constructrice << "pointeur->" << nom_broye_membre << ".membre_actif = 0;\n";
			}
		}
		else if (type_membre->genre == GenreType::STRUCTURE) {
			constructrice << "initialise_" << type_membre->nom_broye << "(&pointeur->" << nom_broye_membre << ");\n";
		}
		else if (type_membre->genre == GenreType::TABLEAU_FIXE) {
			/* À FAIRE */
		}
		else {
			constructrice << "pointeur->" << nom_broye_membre << " = 0;\n";
		}
	}

	constructrice << "}\n";
}

static void cree_initialisation_defaut_pour_constante(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		dls::flux_chaine &flux,
		Type *type)
{
	switch (type->genre) {
		case GenreType::CHAINE:
		{
			flux << "{ .pointeur = 0, .taille = 0 }";
			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			flux << "{ .pointeur = 0, .taille = 0, " << broye_nom_simple(".capacité") << " = 0 }";
			break;
		}
		case GenreType::BOOL:
		case GenreType::OCTET:
		case GenreType::ENUM:
		case GenreType::ERREUR:
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		case GenreType::ENTIER_CONSTANT:
		case GenreType::REEL:
		case GenreType::FONCTION:
		case GenreType::POINTEUR:
		case GenreType::REFERENCE: // À FAIRE : une référence ne peut être nulle
		{
			flux << "0";
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			flux << "{ 0 }";
			break;
		}
		case GenreType::UNION:
		{
			auto type_union = static_cast<TypeUnion *>(type);

			if (type_union->est_nonsure) {
				flux << "{ 0 }";
			}
			else {
				flux << "{ { 0 }, 0 }";
			}

			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto virgule = '{';

			POUR (type_struct->membres) {
				flux << virgule << '.' << broye_chaine(it.nom) << " = ";

				if (it.expression_valeur_defaut != nullptr) {
					genere_code_C(it.expression_valeur_defaut, constructrice, contexte, true);
					flux << it.expression_valeur_defaut->chaine_calculee();
				}
				else {
					cree_initialisation_defaut_pour_constante(contexte, constructrice, flux, it.type);
				}

				virgule = ',';
			}

			if (type_struct->membres.taille == 0) {
				flux << "{ 0 ";
			}

			flux << '}';

			break;
		}
		case GenreType::EINI:
		{
			flux << "{ .pointeur = 0, .info = 0 }";
			break;
		}
		case GenreType::VARIADIQUE:
		case GenreType::RIEN:
		case GenreType::INVALIDE:
		{
			break;
		}
	}
}

static void genere_code_position_source(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &flux,
		NoeudExpression *b)
{
	if (contexte.pour_appel) {
		b = contexte.pour_appel;
	}

	auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme->fichier));

	auto fonction_courante = contexte.donnees_fonction;
	auto nom_fonction = dls::vue_chaine_compacte("");

	if (fonction_courante != nullptr) {
		nom_fonction = fonction_courante->lexeme->chaine;
	}

	auto pos = position_lexeme(*b->lexeme);

	flux << "{ ";
	flux << ".fichier = { .pointeur = \"" << fichier->nom << ".kuri\", .taille = " << fichier->nom.taille() + 5 << " },";
	flux << ".fonction = { .pointeur = \"" << nom_fonction << "\", .taille = " << nom_fonction.taille() << " },";
	flux << ".ligne = " << pos.numero_ligne << " ,";
	flux << ".colonne = " << pos.pos;
	flux << " };";
}

static void genere_code_allocation(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		Type *type,
		int mode,
		NoeudExpression *b,
		NoeudExpression *variable,
		NoeudExpression *expression,
		NoeudExpression *bloc_sinon)
{
	auto expr_nouvelle_taille_octet = dls::chaine("");
	auto expr_ancienne_taille_octet = dls::chaine("");
	auto expr_nouvelle_taille = dls::chaine("");
	auto expr_acces_capacite = dls::chaine("");
	auto expr_pointeur = dls::chaine("");
	auto chn_enfant = dls::chaine("");

	/* variable n'est nul que pour les allocations simples */
	if (variable != nullptr) {
		assert(mode == 1 || mode == 2);
		applique_transformation(variable, constructrice, contexte, true);
		chn_enfant = variable->chaine_calculee();
	}
	else {
		assert(mode == 0);
		chn_enfant = constructrice.declare_variable_temp(type, index++);

		if (type->genre == GenreType::TABLEAU_DYNAMIQUE) {
			constructrice << chn_enfant << ".taille = 0;\n";
		}
	}

	switch (type->genre) {
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			expr_pointeur = chn_enfant + ".pointeur";
			expr_acces_capacite = chn_enfant + broye_nom_simple(".capacité");
			expr_ancienne_taille_octet = expr_acces_capacite;
			expr_ancienne_taille_octet += " * sizeof(" + nom_broye_type(type_deref) + ")";

			/* allocation ou réallocation */
			if (!b->type_declare.expressions.est_vide()) {
				expression = b->type_declare.expressions[0];
				genere_code_C(expression, constructrice, contexte, false);
				expr_nouvelle_taille = expression->chaine_calculee();
				expr_nouvelle_taille_octet = expr_nouvelle_taille;
				expr_nouvelle_taille_octet += " * sizeof(" + nom_broye_type(type_deref) + ")";
			}
			/* désallocation */
			else {
				expr_nouvelle_taille = "0";
				expr_nouvelle_taille_octet = "0";
			}

			break;
		}
		case GenreType::CHAINE:
		{
			expr_pointeur = chn_enfant + ".pointeur";
			expr_acces_capacite = chn_enfant + ".taille";
			expr_ancienne_taille_octet = expr_acces_capacite;

			/* allocation ou réallocation */
			if (expression != nullptr) {
				genere_code_C(expression, constructrice, contexte, false);
				expr_nouvelle_taille = expression->chaine_calculee();
				expr_nouvelle_taille_octet = expr_nouvelle_taille;
			}
			/* désallocation */
			else {
				expr_nouvelle_taille = "0";
				expr_nouvelle_taille_octet = "0";
			}

			break;
		}
		default:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);
			expr_pointeur = chn_enfant;
			expr_ancienne_taille_octet = "sizeof(" + nom_broye_type(type_deref) + ")";

			/* allocation ou réallocation */
			expr_nouvelle_taille_octet = expr_ancienne_taille_octet;
		}
	}

	// int mode = ...;
	// long nouvelle_taille_octet = ...;
	// long ancienne_taille_octet = ...;
	// void *pointeur = ...;
	// void *données = contexte.données_allocatrice;
	// InfoType *info_type = ...;
	// PositionSourceCode pos = ...;
	// contexte.allocatrice(contexte, mode, nouvelle_taille_octet, ancienne_taille_octet, pointeur, données, info_type, pos);
	auto const chn_index = dls::vers_chaine(index++);
	auto const nom_nouvelle_taille = "nouvelle_taille" + chn_index;
	auto const nom_ancienne_taille = "ancienne_taille" + chn_index;
	auto const nom_ancien_pointeur = "ancien_pointeur" + chn_index;
	auto const nom_nouveu_pointeur = "nouveu_pointeur" + chn_index;
	auto const nom_info_type = "info_type" + chn_index;

	constructrice << "long " << nom_nouvelle_taille << " = " << expr_nouvelle_taille_octet << ";\n";
	constructrice << "long " << nom_ancienne_taille << " = " << expr_ancienne_taille_octet << ";\n";
	constructrice << "InfoType *" << nom_info_type << " = (InfoType *)(&" << type->ptr_info_type << ");\n";
	constructrice << "void *" << nom_ancien_pointeur << " = " << expr_pointeur << ";\n";

	auto nom_pos = "__var_temp_pos" + dls::vers_chaine(index++);
	constructrice << "PositionCodeSource " << nom_pos << " = ";

	dls::flux_chaine flux;
	genere_code_position_source(contexte, flux, b);
	constructrice << flux.chn() << "\n;";

	constructrice << "void *" << nom_nouveu_pointeur << " = ";
	constructrice << "contexte.allocatrice(contexte, ";
	constructrice << mode << ',';
	constructrice << nom_nouvelle_taille << ',';
	constructrice << nom_ancienne_taille << ',';
	constructrice << nom_ancien_pointeur << ',';
	constructrice << broye_nom_simple("contexte.données_allocatrice") << ',';
	constructrice << nom_info_type << ',';
	constructrice << nom_pos << ");\n";
	constructrice << expr_pointeur << " = " << nom_nouveu_pointeur << ";\n";

	switch (mode) {
		case 0:
		{
			genere_code_echec_logement(
						contexte,
						constructrice,
						expr_pointeur,
						b,
						bloc_sinon);

			if (type->genre != GenreType::CHAINE && type->genre != GenreType::TABLEAU_DYNAMIQUE) {
				auto type_deref = contexte.typeuse.type_dereference_pour(type);

				if (type_deref->genre == GenreType::STRUCTURE) {
					constructrice << "initialise_" << type_deref->nom_broye << '(' << expr_pointeur << ");\n";
				}
			}

			if (expr_acces_capacite.taille() != 0) {
				b->valeur_calculee = chn_enfant;
			}
			else {
				b->valeur_calculee = expr_pointeur;
			}

			break;
		}
		case 1:
		{
			genere_code_echec_logement(
						contexte,
						constructrice,
						expr_pointeur,
						b,
						bloc_sinon);

			break;
		}
	}

	/* Il faut attendre d'avoir généré le code d'ajournement de la mémoire avant
	 * de modifier la taille. */
	if (expr_acces_capacite.taille() != 0) {
		constructrice << expr_acces_capacite << " = " << expr_nouvelle_taille <<  ";\n";
	}
}

static void genere_declaration_fonction(
		NoeudExpression *b,
		ConstructriceCodeC &constructrice)
{
	using dls::outils::possede_drapeau;

	auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

	if (est_externe) {
		return;
	}

	auto decl = static_cast<NoeudDeclarationFonction *>(b);

	/* Code pour le type et le nom */

	auto nom_fonction = decl->nom_broye;

	auto moult_retour = decl->type_fonc->types_sorties.taille > 1;

	if (moult_retour) {
		if (possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
			constructrice << "static inline void ";
		}
		else if (possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
			constructrice << "static void __attribute__ ((noinline)) ";
		}
		else {
			constructrice << "static void ";
		}

		constructrice << nom_fonction;
	}
	else {
		if (possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
			constructrice << "static inline ";
		}
		else if (possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
			constructrice << "__attribute__ ((noinline)) ";
		}

		auto type_fonc = static_cast<TypeFonction *>(b->type);
		constructrice << nom_broye_type(type_fonc->types_sorties[0]) << ' ' << nom_fonction;
	}

	/* Code pour les arguments */

	auto virgule = '(';

	if (decl->params.taille == 0 && !moult_retour) {
		constructrice << '(' << '\n';
		virgule = ' ';
	}

	if (!decl->est_externe && !possede_drapeau(decl->drapeaux, FORCE_NULCTX)) {
		constructrice << virgule << '\n';
		constructrice << "KsContexteProgramme contexte";
		virgule = ',';
	}

	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident->nom);

		constructrice << virgule << '\n';
		constructrice << nom_broye_type(it->type) << ' ' << nom_broye;

		virgule = ',';
	}

	if (moult_retour) {
		auto idx_ret = 0l;
		POUR (decl->type_fonc->types_sorties) {
			constructrice << virgule << '\n';

			auto nom_ret = "*" + decl->noms_retours[idx_ret++];

			constructrice << nom_broye_type(it) << ' ' << nom_ret;
			virgule = ',';
		}
	}

	constructrice << ")";
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
		NoeudExpression *b,
		ConstructriceCodeC &constructrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	switch (b->genre) {
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			break;
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		case GenreNoeud::DECLARATION_FONCTION:
		{
			using dls::outils::possede_drapeau;

			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			if (est_externe) {
				return;
			}

			genere_declaration_fonction(b, constructrice);
			constructrice << '\n';

			auto decl = static_cast<NoeudDeclarationFonction *>(b);
			contexte.commence_fonction(decl);

			auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme->fichier));

			constructrice << "{\n";

			if (!dls::outils::possede_drapeau(contexte.donnees_fonction->drapeaux, FORCE_SANSTRACE)) {
				constructrice << "INITIALISE_TRACE_APPEL(\""
							  << b->lexeme->chaine << "\", "
							  << b->lexeme->chaine.taille() << ", \""
							  << fichier->nom << ".kuri\", "
							  << fichier->nom.taille() + 5 << ", "
							  << decl->nom_broye << ");\n";
			}

			/* Crée code pour le bloc. */
			decl->bloc->est_bloc_fonction = true;
			genere_code_C(decl->bloc, constructrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(decl->bloc, contexte, constructrice);
			}

			constructrice << "}\n";

			contexte.termine_fonction();

			break;
		}
		case GenreNoeud::DECLARATION_COROUTINE:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(b);

			contexte.commence_fonction(decl);

			/* Crée fonction */
			auto nom_fonction = decl->nom_broye;
			auto nom_type_coro = "__etat_coro" + nom_fonction;

			/* Déclare la structure d'état de la coroutine. */
			constructrice << "typedef struct " << nom_type_coro << " {\n";
			constructrice << "pthread_mutex_t mutex_boucle;\n";
			constructrice << "pthread_cond_t cond_boucle;\n";
			constructrice << "pthread_mutex_t mutex_coro;\n";
			constructrice << "pthread_cond_t cond_coro;\n";
			constructrice << "bool __termine_coro;\n";
			constructrice << "ContexteProgramme contexte;\n";

			auto idx_ret = 0l;
			POUR (decl->type_fonc->types_sorties) {
				auto &nom_ret = decl->noms_retours[idx_ret++];
				constructrice.declare_variable(it, nom_ret, "");
			}

			POUR (decl->params) {
				auto nom_broye = broye_nom_simple(it->ident->nom);
				constructrice.declare_variable(it->type, nom_broye, "");
			}

			constructrice << " } " << nom_type_coro << ";\n";

			/* Déclare la fonction. */
			constructrice << "static void *" << nom_fonction << "(\nvoid *data)\n";
			constructrice << "{\n";
			constructrice << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
			constructrice << "ContexteProgramme contexte = __etat->contexte;\n";

			/* déclare les paramètres. */
			POUR (decl->params) {
				auto nom_broye = broye_nom_simple(it->ident->nom);
				constructrice.declare_variable(it->type, nom_broye, "__etat->" + nom_broye);
			}

			/* Crée code pour le bloc. */
			genere_code_C(decl->bloc, constructrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(decl->bloc, contexte, constructrice);
			}

			constructrice << "}\n";

			contexte.termine_fonction();

			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			cree_appel(b, contexte, constructrice);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto expr = static_cast<NoeudExpressionReference *>(b);
			auto decl = expr->decl;
			auto flux = dls::flux_chaine();

			// À FAIRE : decl peut être nulle pour les appels de pointeurs de fonctions

			if (decl != nullptr && decl->drapeaux & EST_CONSTANTE) {
				auto decl_const = static_cast<NoeudDeclarationVariable *>(decl);
				flux << decl_const->valeur_expression.entier;
			}
			else if (decl != nullptr && decl->genre == GenreNoeud::DECLARATION_FONCTION) {
				auto decl_fonc = static_cast<NoeudDeclarationFonction *>(decl);
				flux << decl_fonc->nom_broye;
			}
			else {
				// À FAIRE(réusinage arbre)
				//	if (dv.est_membre_emploie) {
				//		flux << dv.structure;
				//	}

				if ((b->drapeaux & EST_VAR_BOUCLE) != 0) {
					flux << "(*";
				}

				flux << broye_chaine(b);

				if ((b->drapeaux & EST_VAR_BOUCLE) != 0) {
					flux << ")";
				}
			}

			b->valeur_calculee = dls::chaine(flux.chn());

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto inst = static_cast<NoeudExpressionMembre *>(b);
			auto structure = inst->accede;
			auto membre = inst->membre;

			genere_code_acces_membre(contexte, constructrice, b, structure, membre, expr_gauche);

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto inst = static_cast<NoeudExpressionMembre *>(b);
			auto structure = inst->accede;
			auto membre = inst->membre;

			auto index_membre = inst->index_membre;
			auto type = structure->type;

			auto est_pointeur = (type->genre == GenreType::POINTEUR);
			est_pointeur |= (type->genre == GenreType::REFERENCE);

			auto flux = dls::flux_chaine();
			flux << broye_chaine(structure->ident->nom);
			flux << (est_pointeur ? "->" : ".");

			auto acces_structure = flux.chn();

			flux << broye_chaine(membre->ident->nom);

			auto expr_membre = acces_structure + "membre_actif";

			if (expr_gauche) {
				constructrice << expr_membre << " = " << index_membre + 1 << ";";
			}
			else {
				if (b->aide_generation_code != IGNORE_VERIFICATION) {
					constructrice << "if (" << expr_membre << " != " << index_membre + 1 << ") {\n";
					debute_record_trace_appel(contexte, constructrice, b);
					constructrice << contexte.interface_kuri.decl_panique_membre_union->nom_broye << "(contexte);\n";
					constructrice << "}\n";
				}
			}

			b->valeur_calculee = dls::chaine(flux.chn());

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto variable = expr->expr1;
			auto expression = expr->expr2;

			/* a, b = foo(); -> foo(&a, &b); */
			if (variable->lexeme->genre == GenreLexeme::VIRGULE) {
				dls::tablet<NoeudExpression *, 10> feuilles;
				rassemble_feuilles(variable, feuilles);

				dls::liste<NoeudExpression *> noeuds;

				for (auto f : feuilles) {
					noeuds.pousse(f);
				}

				expression->aide_generation_code = APPEL_FONCTION_MOULT_RET;
				expression->valeur_calculee = noeuds;

				genere_code_C(expression,
							  constructrice,
							  contexte,
							  true);
				return;
			}

			applique_transformation(expression, constructrice, contexte, false);

			genere_code_C(variable, constructrice, contexte, true);

			constructrice << variable->chaine_calculee();
			constructrice << " = ";
			constructrice << expression->chaine_calculee();

			/* pour les globales */
			constructrice << ";\n";

			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(b);
			auto variable = expr->valeur;
			auto expression = expr->expression;

			if (dls::outils::possede_drapeau(expr->drapeaux, EST_CONSTANTE)) {
				return;
			}

			if (dls::outils::possede_drapeau(variable->drapeaux, EST_EXTERNE)) {
				return;
			}

			auto flux = dls::flux_chaine();
			auto type = variable->type;

			if (expression != nullptr) {
				/* pour les assignations de tableaux fixes, remplace les crochets
				 * par des pointeurs pour la déclaration */
				if (expression->genre == GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU) {
					type = contexte.typeuse.type_dereference_pour(type);
					type = contexte.typeuse.type_pointeur_pour(type);
				}
			}

			auto nom_broye = broye_chaine(variable->ident->nom);
			flux << nom_broye_type(type) << ' ' << nom_broye;

			/* nous avons une déclaration, initialise à zéro */
			if (expression == nullptr) {
				if (contexte.donnees_fonction != nullptr) {
					constructrice << flux.chn() << ";\n";

					if (variable->type->genre == GenreType::STRUCTURE || variable->type->genre == GenreType::UNION) {
						constructrice << "initialise_" << type->nom_broye << "(&" << nom_broye << ");\n";
					}
					else if (variable->type->genre == GenreType::CHAINE) {
						constructrice << nom_broye << ".pointeur = 0;\n";
						constructrice << nom_broye << ".taille = 0;\n";
					}
					else if (variable->type->genre == GenreType::TABLEAU_DYNAMIQUE) {
						constructrice << nom_broye << ".pointeur = 0;\n";
						constructrice << nom_broye << ".taille = 0;\n";
						constructrice << nom_broye << broye_nom_simple(".capacité") << " = 0;\n";
					}
					else if (variable->type->genre == GenreType::EINI) {
						constructrice << nom_broye << ".pointeur = 0\n;";
						constructrice << nom_broye << ".info = 0\n;";
					}
					else if (variable->type->genre != GenreType::TABLEAU_FIXE) {
						constructrice << nom_broye << " = 0;\n";
					}
				}
				else {
					flux << " = ";
					cree_initialisation_defaut_pour_constante(contexte, constructrice, flux, type);
					constructrice << flux.chn();
					constructrice << ";\n";
				}
			}
			else {
				if (expression->genre != GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
					applique_transformation(expression, constructrice, contexte, contexte.donnees_fonction == nullptr);
					constructrice << flux.chn();
					constructrice << " = ";
					constructrice << expression->chaine_calculee();
					constructrice << ";\n";
				}
				else {
					constructrice << flux.chn() << ";\n";
				}
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			b->valeur_calculee = dls::vers_chaine(b->lexeme->valeur_reelle);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			b->valeur_calculee = dls::vers_chaine(b->lexeme->valeur_entiere);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			auto flux = dls::flux_chaine();

			applique_transformation(enfant1, constructrice, contexte, expr_gauche);
			applique_transformation(enfant2, constructrice, contexte, expr_gauche);

			auto op = expr->op;

			if (op->est_basique) {
				flux << enfant1->chaine_calculee();
				flux << b->lexeme->chaine;
				flux << enfant2->chaine_calculee();
			}
			else {
				/* À FAIRE: gestion du contexte. */
				flux << op->decl->nom_broye << '(';
				flux << "contexte,";
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
				constructrice << "const ";
				constructrice.declare_variable(b->type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = static_cast<NoeudExpressionBinaire *>(inst->expr1);
			auto enfant2 = inst->expr2;

			auto flux = dls::flux_chaine();

			genere_code_C(enfant1, constructrice, contexte, expr_gauche);
			genere_code_C(enfant2, constructrice, contexte, expr_gauche);

			/* (a comp b) */
			flux << '(';
			flux << enfant1->chaine_calculee();
			flux << ')';

			flux << "&&";

			/* (b comp c) */
			flux << '(';
			flux << enfant1->expr2->chaine_calculee();
			flux << b->lexeme->chaine;
			flux << enfant2->chaine_calculee();
			flux << ')';

			auto nom_var = "__var_temp_cmp" + dls::vers_chaine(index++);
			constructrice << "const ";
			constructrice.declare_variable(b->type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			auto type1 = enfant1->type;

			if (type1->genre == GenreType::REFERENCE) {
				type1 = contexte.typeuse.type_dereference_pour(type1);
			}

			auto flux = dls::flux_chaine();

			// force une expression gauche pour les types tableau fixe car une
			// expression droite pourrait créer une variable temporarire T x[N] = pointeur
			// ce qui est invalide en C.
			applique_transformation(enfant1, constructrice, contexte, expr_gauche || type1->genre == GenreType::TABLEAU_FIXE);
			genere_code_C(enfant2, constructrice, contexte, expr_gauche);

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

			switch (type1->genre) {
				case GenreType::POINTEUR:
				{
					flux << enfant1->chaine_calculee();
					flux << '[';
					flux << enfant2->chaine_calculee();
					flux << ']';
					break;
				}
				case GenreType::CHAINE:
				{
					constructrice << "if (";
					constructrice << enfant2->chaine_calculee();
					constructrice << " < 0 || ";
					constructrice << enfant2->chaine_calculee();
					constructrice << " >= ";
					constructrice << enfant1->chaine_calculee();
					constructrice << ".taille) {\n";
					debute_record_trace_appel(contexte, constructrice, b);
					constructrice << contexte.interface_kuri.decl_panique_chaine->nom_broye << "(contexte, ";
					constructrice << enfant1->chaine_calculee();
					constructrice << ".taille,";
					constructrice << enfant2->chaine_calculee();
					constructrice << ");\n}\n";

					flux << enfant1->chaine_calculee();
					flux << ".pointeur";
					flux << '[';
					flux << enfant2->chaine_calculee();
					flux << ']';

					break;
				}
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					if (b->aide_generation_code != IGNORE_VERIFICATION) {
						constructrice << "if (";
						constructrice << enfant2->chaine_calculee();
						constructrice << " < 0 || ";
						constructrice << enfant2->chaine_calculee();
						constructrice << " >= ";
						constructrice << enfant1->chaine_calculee();
						constructrice << ".taille";
						constructrice << ") {\n";
						debute_record_trace_appel(contexte, constructrice, b);
						constructrice << contexte.interface_kuri.decl_panique_tableau->nom_broye << "(contexte, ";
						constructrice << enfant1->chaine_calculee();
						constructrice << ".taille";
						constructrice << ",";
						constructrice << enfant2->chaine_calculee();
						constructrice << ");\n}\n";
					}

					flux << enfant1->chaine_calculee() << ".pointeur";
					flux << '[';
					flux << enfant2->chaine_calculee();
					flux << ']';
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					auto taille_tableau = static_cast<TypeTableauFixe *>(type1)->taille;

					if (b->aide_generation_code != IGNORE_VERIFICATION) {
						constructrice << "if (";
						constructrice << enfant2->chaine_calculee();
						constructrice << " < 0 || ";
						constructrice << enfant2->chaine_calculee();
						constructrice << " >= ";
						constructrice << taille_tableau;

						constructrice << ") {\n";
						debute_record_trace_appel(contexte, constructrice, b);
						constructrice << contexte.interface_kuri.decl_panique_tableau->nom_broye << "(contexte, ";
						constructrice << taille_tableau;
						constructrice << ",";
						constructrice << enfant2->chaine_calculee();
						constructrice << ");\n}\n";
					}

					flux << enfant1->chaine_calculee();
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
				constructrice << "const ";
				constructrice.declare_variable(b->type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = expr->expr;

			/* force une expression si l'opérateur est @, pour que les
			 * expressions du type @a[0] retourne le pointeur à a + 0 et non le
			 * pointeur de la variable temporaire du code généré */
			expr_gauche |= b->lexeme->genre == GenreLexeme::AROBASE;
			applique_transformation(enfant, constructrice, contexte, expr_gauche);

			if (b->lexeme->genre == GenreLexeme::AROBASE) {
				b->valeur_calculee = "&(" + enfant->chaine_calculee() + ")";
			}
			else {
				auto flux = dls::flux_chaine();
				auto op = expr->op;

				if (op->est_basique) {
					flux << b->lexeme->chaine;
				}
				else {
					flux << expr->op->decl->nom_broye;
				}

				flux << '(';
				flux << enfant->chaine_calculee();
				flux << ')';

				b->valeur_calculee = dls::chaine(flux.chn());
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			genere_code_extra_pre_retour(b->bloc_parent, contexte, constructrice);
			constructrice << "return;\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = inst->expr;
			auto df = contexte.donnees_fonction;
			auto nom_variable = df->noms_retours[0];

			/* utilisation d'une valeur gauche (donc sans temporaire) pour le
			 * retour de références */
			applique_transformation(enfant, constructrice, contexte, false);

			constructrice.declare_variable(
						b->type,
						nom_variable,
						enfant->chaine_calculee());

			/* NOTE : le code différé doit être créé après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(b->bloc_parent, contexte, constructrice);
			constructrice << "return " << nom_variable << ";\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = inst->expr;
			auto df = contexte.donnees_fonction;

			if (enfant->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
				/* retourne foo() -> foo(__ret...); return; */
				enfant->aide_generation_code = APPEL_FONCTION_MOULT_RET2;
				enfant->valeur_calculee = df->noms_retours;
				genere_code_C(enfant, constructrice, contexte, false);
			}
			else if (enfant->lexeme->genre == GenreLexeme::VIRGULE) {
				/* retourne a, b; -> *__ret1 = a; *__ret2 = b; return; */
				dls::tablet<NoeudExpression *, 10> feuilles;
				rassemble_feuilles(enfant, feuilles);

				auto idx = 0l;
				for (auto f : feuilles) {
					applique_transformation(f, constructrice, contexte, false);

					constructrice << '*' << df->noms_retours[idx++] << " = ";
					constructrice << f->chaine_calculee();
					constructrice << ';';
				}
			}

			/* NOTE : le code différé doit être créé après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(b->bloc_parent, contexte, constructrice);
			constructrice << "return;\n";
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			auto flux = dls::flux_chaine();

			flux << "{ .pointeur = " << '"';

			auto char_depuis_hex = [](char hex)
			{
				return "0123456789ABCDEF"[static_cast<int>(hex)];
			};

			auto chaine = kuri::chaine();
			chaine.pointeur = b->lexeme->pointeur;
			chaine.taille = b->lexeme->taille;

			POUR (chaine) {
				flux << "\\x" << char_depuis_hex((it & 0xf0) >> 4) << char_depuis_hex(it & 0x0f);
			}

			flux << '"';
			flux << ", .taille = " << chaine.taille << " }";

			if (expr_gauche) {
				b->valeur_calculee = dls::chaine(flux.chn());
			}
			else {
				auto nom_chaine = "__chaine_tmp" + dls::vers_chaine(b);
				constructrice << "chaine " << nom_chaine << " = " << flux.chn() << ";\n";
				b->valeur_calculee = nom_chaine;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
										   : (b->lexeme->chaine == "vrai");
			b->valeur_calculee = valeur ? dls::chaine("1") : dls::chaine("0");
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			auto c = static_cast<unsigned char>(b->lexeme->valeur_entiere);

			auto flux = dls::flux_chaine();

			auto char_depuis_hex = [](unsigned char hex)
			{
				return "0123456789ABCDEF"[static_cast<int>(hex)];
			};

			flux << '\'';
			flux << "\\x" << char_depuis_hex((c & 0xf0) >> 4) << char_depuis_hex(c & 0x0f);
			flux << '\'';

			b->valeur_calculee = dls::chaine(flux.chn());
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst = static_cast<NoeudSi *>(b);

			if (!expr_gauche) {
				auto enfant1 = inst->condition;
				auto enfant2 = inst->bloc_si_vrai;
				auto enfant3 = inst->bloc_si_vrai;

				genere_code_C(enfant1, constructrice, contexte, false);
				genere_code_C(enfant2->expressions[0], constructrice, contexte, false);
				genere_code_C(enfant3->expressions[0], constructrice, contexte, false);

				auto flux = dls::flux_chaine();

				if (b->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
					flux << '!';
				}

				flux << enfant1->chaine_calculee();
				flux << " ? ";
				/* prenons les enfants des enfants pour ne mettre des accolades
				 * autour de l'expression vu qu'ils sont de type 'BLOC' */
				flux << enfant2->expressions[0]->chaine_calculee();
				flux << " : ";
				flux << enfant3->expressions[0]->chaine_calculee();

				auto nom_variable = "__var_temp_si" + dls::vers_chaine(index++);

				constructrice.declare_variable(
							enfant2->expressions[0]->type,
							nom_variable,
							flux.chn());

				enfant1->valeur_calculee = nom_variable;

				return;
			}

			auto condition = inst->condition;

			genere_code_C(condition, constructrice, contexte, false);

			constructrice << "if (";

			if (b->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
				constructrice << '!';
			}

			constructrice << condition->chaine_calculee();
			constructrice << ")";

			genere_code_C(inst->bloc_si_vrai, constructrice, contexte, false);

			if (inst->bloc_si_faux) {
				constructrice << "else ";
				genere_code_C(inst->bloc_si_faux, constructrice, contexte, false);
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto inst = static_cast<NoeudBloc *>(b);
			auto dernier_enfant = static_cast<NoeudExpression *>(nullptr);

			if (inst->est_differe) {
				inst->bloc_parent->noeuds_differes.pousse(inst);
				return;
			}

			if (!inst->est_bloc_fonction) {
				constructrice << "{\n";
			}

			POUR (inst->expressions) {
				genere_code_C(it, constructrice, contexte, true);

				dernier_enfant = it;

				if (est_instruction_retour(it->genre)) {
					break;
				}

				if (it->genre == GenreNoeud::OPERATEUR_BINAIRE) {
					/* les assignations composées (+=, etc) n'ont pas leurs codes
					 * générées via genere_code_C  */
					if (est_assignation_composee(it->lexeme->genre)) {
						constructrice << it->chaine_calculee();
						constructrice << ";\n";
					}
				}
			}

			if (dernier_enfant != nullptr && !est_instruction_retour(dernier_enfant->genre)) {
				/* génère le code pour tous les noeuds différés de ce bloc */
				for (auto i = inst->noeuds_differes.taille - 1; i >= 0; --i) {
					auto bloc_differe = inst->noeuds_differes[i];
					bloc_differe->est_differe = false;
					genere_code_C(bloc_differe, constructrice, contexte, false);
					bloc_differe->est_differe = true;
				}
			}

			if (!inst->est_bloc_fonction) {
				constructrice << "}\n";
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto inst = static_cast<NoeudPour *>(b);

			/* on génère d'abord le type de la variable */
			auto enfant1 = inst->variable;
			auto enfant2 = inst->expression;
			auto enfant3 = inst->bloc;
			auto enfant_sans_arret = inst->bloc_sansarret;
			auto enfant_sinon = inst->bloc_sinon;

			auto type = enfant2->type;
			enfant1->type = type;

			auto genere_code_tableau_chaine = [&constructrice](
					ConstructriceCodeC &gen_loc,
					ContexteGenerationCode &contexte_loc,
					NoeudExpression *enfant_1,
					NoeudExpression *enfant_2,
					Type *type_loc,
					dls::chaine const &nom_var)
			{
				auto var = enfant_1;
				auto idx = static_cast<NoeudExpression *>(nullptr);

				if (enfant_1->lexeme->genre == GenreLexeme::VIRGULE) {
					auto expr_bin = static_cast<NoeudExpressionBinaire *>(var);
					var = expr_bin->expr1;
					idx = expr_bin->expr2;
				}

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, constructrice, contexte_loc, true);

				gen_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= ";
				gen_loc << enfant_2->chaine_calculee();
				gen_loc << ".taille - 1; ++"<< nom_var <<") {\n";
				gen_loc << nom_broye_type(type_loc);
				gen_loc << " *" << broye_chaine(var) << " = &";
				gen_loc << enfant_2->chaine_calculee();
				gen_loc << ".pointeur["<< nom_var <<"];\n";

				if (idx) {
					gen_loc << "int " << broye_chaine(idx) << " = " << nom_var << ";\n";
				}
			};

			auto ident_boucle = static_cast<IdentifiantCode *>(nullptr);

			auto genere_code_tableau_fixe = [&constructrice, &ident_boucle](
					ConstructriceCodeC &gen_loc,
					ContexteGenerationCode &contexte_loc,
					NoeudExpression *enfant_1,
					NoeudExpression *enfant_2,
					Type *type_loc,
					dls::chaine const &nom_var,
					long taille_tableau)
			{
				auto var = enfant_1;
				auto idx = static_cast<NoeudExpression *>(nullptr);

				if (enfant_1->lexeme->genre == GenreLexeme::VIRGULE) {
					auto expr_bin = static_cast<NoeudExpressionBinaire *>(var);
					var = expr_bin->expr1;
					idx = expr_bin->expr2;
				}

				ident_boucle = var->ident;

				gen_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= "
				   << taille_tableau << "-1; ++"<< nom_var <<") {\n";

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, constructrice, contexte_loc, true);
				gen_loc << nom_broye_type(type_loc);
				gen_loc << " *" << broye_chaine(var) << " = &";
				gen_loc << enfant_2->chaine_calculee();
				gen_loc << "["<< nom_var <<"];\n";

				if (idx) {
					gen_loc << "int " << broye_chaine(idx) << " = " << nom_var << ";\n";
				}
			};

			switch (b->aide_generation_code) {
				case GENERE_BOUCLE_PLAGE:
				case GENERE_BOUCLE_PLAGE_INDEX:
				{
					auto plage = static_cast<NoeudExpressionBinaire *>(enfant2);
					auto var = enfant1;
					auto idx = static_cast<NoeudExpression *>(nullptr);

					if (enfant1->lexeme->genre == GenreLexeme::VIRGULE) {
						auto expr_bin = static_cast<NoeudExpressionBinaire *>(var);
						var = expr_bin->expr1;
						idx = expr_bin->expr2;
					}

					ident_boucle = var->ident;

					auto nom_broye = broye_chaine(var);

					if (idx != nullptr) {
						constructrice << "int " << broye_chaine(idx) << " = 0;\n";
					}

					genere_code_C(plage->expr1, constructrice, contexte, false);
					genere_code_C(plage->expr2, constructrice, contexte, false);

					constructrice << "for (";
					constructrice << nom_broye_type(type);
					constructrice << " " << nom_broye << " = ";
					constructrice << plage->expr1->chaine_calculee();

					constructrice << "; "
					   << nom_broye << " <= ";
					constructrice << plage->expr2->chaine_calculee();

					constructrice <<"; ++" << nom_broye;

					if (idx != nullptr) {
						constructrice << ", ++" << broye_chaine(idx);
					}

					constructrice  << ") {\n";

					break;
				}
				case GENERE_BOUCLE_TABLEAU:
				case GENERE_BOUCLE_TABLEAU_INDEX:
				{
					auto nom_var = "__i" + dls::vers_chaine(b);

					if (type->genre == GenreType::TABLEAU_FIXE) {
						auto type_tabl = static_cast<TypeTableauFixe *>(type);
						auto type_deref = type_tabl->type_pointe;
						genere_code_tableau_fixe(constructrice, contexte, enfant1, enfant2, type_deref, nom_var, type_tabl->taille);
					}
					else if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::VARIADIQUE) {
						auto type_deref = contexte.typeuse.type_dereference_pour(type);
						genere_code_tableau_chaine(constructrice, contexte, enfant1, enfant2, type_deref, nom_var);
					}
					else if (type->genre == GenreType::CHAINE) {
						type = contexte.typeuse[TypeBase::Z8];
						genere_code_tableau_chaine(constructrice, contexte, enfant1, enfant2, type, nom_var);
					}

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					auto expr_appel = static_cast<NoeudExpressionAppel *>(enfant2);
					auto decl_fonc = static_cast<NoeudDeclarationFonction const *>(expr_appel->noeud_fonction_appelee);
					auto nom_etat = "__etat" + dls::vers_chaine(enfant2);
					auto nom_type_coro = "__etat_coro" + decl_fonc->nom_broye;

					constructrice << nom_type_coro << " " << nom_etat << " = {\n";
					constructrice << ".mutex_boucle = PTHREAD_MUTEX_INITIALIZER,\n";
					constructrice << ".mutex_coro = PTHREAD_MUTEX_INITIALIZER,\n";
					constructrice << ".cond_coro = PTHREAD_COND_INITIALIZER,\n";
					constructrice << ".cond_boucle = PTHREAD_COND_INITIALIZER,\n";
					constructrice << ".contexte = contexte,\n";
					constructrice << ".__termine_coro = 0\n";
					constructrice << "};\n";

					/* intialise les arguments de la fonction. */
					POUR (expr_appel->params) {
						genere_code_C(it, constructrice, contexte, false);
					}

					auto iter_enf = expr_appel->params.begin();

					POUR (decl_fonc->params) {
						auto nom_broye = broye_nom_simple(it->ident->nom);
						constructrice << nom_etat << '.' << nom_broye << " = ";
						constructrice << (*iter_enf)->chaine_calculee();
						constructrice << ";\n";
						++iter_enf;
					}

					constructrice << "pthread_t fil_coro;\n";
					constructrice << "pthread_create(&fil_coro, NULL, " << decl_fonc->nom_broye << ", &" << nom_etat << ");\n";
					constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";
					constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
					constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

					/* À FAIRE : utilisation du type */
					auto nombre_vars_ret = decl_fonc->type_fonc->types_sorties.taille;

					auto feuilles = dls::tablet<NoeudExpression *, 10>{};
					rassemble_feuilles(enfant1, feuilles);

					auto idx = static_cast<NoeudExpression *>(nullptr);
					auto nom_idx = dls::chaine{};

					if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
						idx = feuilles.back();
						nom_idx = "__idx" + dls::vers_chaine(b);
						constructrice << "int " << nom_idx << " = 0;";
					}

					constructrice << "while (" << nom_etat << ".__termine_coro == 0) {\n";
					constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";

					for (auto i = 0l; i < nombre_vars_ret; ++i) {
						auto f = feuilles[i];
						auto nom_var_broye = broye_chaine(f);
						constructrice.declare_variable(type, nom_var_broye, "");
						constructrice << nom_var_broye << " = "
									   << nom_etat << '.' << decl_fonc->noms_retours[i]
									   << ";\n";
					}

					constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_coro);\n";
					constructrice << "pthread_cond_signal(&" << nom_etat << ".cond_coro);\n";
					constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_coro);\n";
					constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
					constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

					if (idx) {
						constructrice << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
						constructrice << nom_idx << " += 1;";
					}
				}
			}

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);
			auto goto_brise = "__boucle_pour_brise" + dls::vers_chaine(b);

			contexte.empile_goto_continue(ident_boucle, goto_continue);
			contexte.empile_goto_arrete(ident_boucle, (enfant_sinon != nullptr) ? goto_brise : goto_apres);

			genere_code_C(enfant3, constructrice, contexte, false);

			constructrice << goto_continue << ":;\n";
			constructrice << "}\n";

			if (enfant_sans_arret) {
				genere_code_C(enfant_sans_arret, constructrice, contexte, false);
				constructrice << "goto " << goto_apres << ";";
			}

			if (enfant_sinon) {
				constructrice << goto_brise << ":;\n";
				genere_code_C(enfant_sinon, constructrice, contexte, false);
			}

			constructrice << goto_apres << ":;\n";

			contexte.depile_goto_arrete();
			contexte.depile_goto_continue();

			if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE || b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
				constructrice << "pthread_join(fil_coro, NULL);\n";
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto chaine_var = inst->expr == nullptr ? nullptr : inst->expr->ident;

			auto label_goto = (b->lexeme->genre == GenreLexeme::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			constructrice << "goto " << label_goto << ";\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue(nullptr, goto_continue);
			contexte.empile_goto_arrete(nullptr, goto_apres);

			constructrice << "while (1) {\n";

			genere_code_C(inst->bloc, constructrice, contexte, false);

			constructrice << goto_continue << ":;\n";
			constructrice << "}\n";
			constructrice << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue(nullptr, goto_continue);
			contexte.empile_goto_arrete(nullptr, goto_apres);

			constructrice << "while (1) {\n";
			genere_code_C(inst->bloc, constructrice, contexte, false);
			constructrice << goto_continue << ":;\n";
			genere_code_C(inst->condition, constructrice, contexte, false);
			constructrice << "if (!";
			constructrice << inst->condition->chaine_calculee();
			constructrice << ") {\nbreak;\n}\n";
			constructrice << "}\n";
			constructrice << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue(nullptr, goto_continue);
			contexte.empile_goto_arrete(nullptr, goto_apres);

			/* NOTE : la prépasse est susceptible de générer le code d'un appel
			 * de fonction, donc nous utilisons
			 * while (1) { if (!cond) { break; } }
			 * au lieu de
			 * while (cond) {}
			 * pour être sûr que la fonction est appelée à chaque boucle.
			 */
			constructrice << "while (1) {";
			genere_code_C(inst->condition, constructrice, contexte, false);
			constructrice << "if (!";
			constructrice << inst->condition->chaine_calculee();
			constructrice << ") { break; }\n";

			genere_code_C(inst->bloc, constructrice, contexte, false);

			constructrice << goto_continue << ":;\n";
			constructrice << "}\n";
			constructrice << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant = inst->expr1;
			auto const &type_de = enfant->type;

			genere_code_C(enfant, constructrice, contexte, false);

			if (type_de == b->type) {
				b->valeur_calculee = enfant->valeur_calculee;
				return;
			}

			auto flux = dls::flux_chaine();

			flux << "(";
			flux << nom_broye_type(b->type);
			flux << ")(";
			flux << enfant->chaine_calculee();
			flux << ")";

			b->valeur_calculee = dls::chaine(flux.chn());

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			b->valeur_calculee = dls::chaine("0");
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto type = std::any_cast<Type *>(b->valeur_calculee);
			b->valeur_calculee = dls::vers_chaine(type->taille_octet);
			break;
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			/* pris en charge dans type_noeud::POUR */
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			/* utilisé principalement pour convertir les listes d'arguments
			 * variadics en un tableau */
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(b);
			auto taille_tableau = noeud_tableau->exprs.taille;
			auto nom_tableau_fixe = dls::chaine("0");

			// si la liste d'arguments est vide, nous passons un tableau vide,
			// sinon nous créons d'abord un tableau fixe, qui sera converti en
			// un tableau dynamique
			if (taille_tableau != 0) {
				POUR (noeud_tableau->exprs) {
					applique_transformation(it, constructrice, contexte, false);
				}

				/* alloue un tableau fixe */
				auto type_tfixe = contexte.typeuse.type_tableau_fixe(b->type, taille_tableau);

				nom_tableau_fixe = dls::chaine("__tabl_fix")
						.append(dls::vers_chaine(reinterpret_cast<long>(b)));

				constructrice << nom_broye_type(type_tfixe) << ' ' << nom_tableau_fixe;
				constructrice << " = ";

				auto virgule = '{';

				POUR (noeud_tableau->exprs) {
					constructrice << virgule;
					constructrice << it->chaine_calculee();
					virgule = ',';
				}

				constructrice << "};\n";
			}

			/* alloue un tableau dynamique */
			auto type_tdyn = contexte.typeuse.type_tableau_dynamique(b->type);

			auto nom_tableau_dyn = dls::chaine("__tabl_dyn")
					.append(dls::vers_chaine(b));

			constructrice << nom_broye_type(type_tdyn) << ' ' << nom_tableau_dyn << ";\n";
			constructrice << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			constructrice << nom_tableau_dyn << ".taille = " << taille_tableau << ";\n";

			b->valeur_calculee = nom_tableau_dyn;
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(b);
			auto flux = dls::flux_chaine();

			if (expr->type->genre == GenreType::UNION) {
				POUR (expr->exprs) {
					if (it != nullptr) {
						applique_transformation(it, constructrice, contexte, false);
					}
				}

				auto index_membre = 0;
				auto expression = static_cast<NoeudExpression *>(nullptr);

				POUR (expr->exprs) {
					if (it != nullptr) {
						expression = it;
						break;
					}

					index_membre += 1;
				}

				auto type_union = static_cast<TypeUnion *>(expr->type);
				auto &membre = type_union->membres[index_membre];

				if (type_union->est_nonsure) {
					flux << "{ ." << broye_chaine(membre.nom) << " = " << expression->chaine_calculee() << " }";
				}
				else {
					flux << "{ { ." << broye_chaine(membre.nom) << " = " << expression->chaine_calculee() << " }, .membre_actif = " << index_membre + 1 << " }";
				}
			}
			else {
				auto type_struct = static_cast<TypeStructure *>(expr->type);

				if (type_struct->nom == "PositionCodeSource") {
					genere_code_position_source(contexte, flux, expr);
				}
				else {
					POUR (expr->exprs) {
						if (it != nullptr) {
							applique_transformation(it, constructrice, contexte, false);
						}
					}

					auto index_membre = 0;
					auto virgule = '{';

					POUR (expr->exprs) {
						flux << virgule;

						auto &membre = type_struct->membres[index_membre];
						index_membre += 1;

						flux << '.' << broye_chaine(membre.nom) << '=';

						if (it == nullptr) {
							cree_initialisation_defaut_pour_constante(contexte, constructrice, flux, membre.type);
						}
						else {
							flux << it->chaine_calculee();
						}

						virgule = ',';
					}

					flux << '}';
				}
			}

			if (expr_gauche) {
				expr->valeur_calculee = dls::chaine(flux.chn());
			}
			else {
				auto nom_temp = "__var_temp_struct" + dls::vers_chaine(index++);
				constructrice.declare_variable(expr->type, nom_temp, flux.chn());

				expr->valeur_calculee = nom_temp;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto nom_tableau = "__tabl" + dls::vers_chaine(b);
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			b->genre_valeur = GenreValeur::DROITE;

			dls::tablet<NoeudExpression *, 10> feuilles;
			rassemble_feuilles(expr->expr, feuilles);

			for (auto f : feuilles) {
				genere_code_C(f, constructrice, contexte, false);
			}

			constructrice << nom_broye_type(b->type) << ' ' << nom_tableau << " = ";

			auto virgule = '{';

			for (auto f : feuilles) {
				constructrice << virgule;
				constructrice << f->chaine_calculee();
				virgule = ',';
			}

			constructrice << "};\n";

			b->valeur_calculee = nom_tableau;
			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = inst->expr;
			b->valeur_calculee = "&" + enfant->type->ptr_info_type;
			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			genere_code_C(expr->expr, constructrice, contexte, false);
			b->valeur_calculee = "(*(" + expr->expr->chaine_calculee() + "))";
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			genere_code_allocation(contexte, constructrice, b->type, 0, b, expr->expr, expr->expr_chaine, expr->bloc);
			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			genere_code_allocation(contexte, constructrice, expr->expr->type, 2, expr->expr, expr->expr, expr->expr_chaine, expr->bloc);
			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			genere_code_allocation(contexte, constructrice, expr->expr->type, 1, b, expr->expr, expr->expr_chaine, expr->bloc);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			if (b->type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(b->type);

				if (type_union->deja_genere) {
					return;
				}

				genere_declaration_structure(constructrice, type_union->decl);
				cree_initialisation_structure(contexte, constructrice, type_union, type_union->decl);

				type_union->deja_genere = true;
			}
			else if (b->type->genre == GenreType::STRUCTURE) {
				auto type_struct = static_cast<TypeStructure *>(b->type);

				if (type_struct->deja_genere) {
					return;
				}

				genere_declaration_structure(constructrice, type_struct->decl);
				cree_initialisation_structure(contexte, constructrice, type_struct, type_struct->decl);

				type_struct->deja_genere = true;
			}

			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			/* nous ne générons pas de code pour les énums, nous prenons leurs
			 * valeurs directement */
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			/* le premier enfant est l'expression, les suivants les paires */
			auto inst = static_cast<NoeudDiscr *>(b);
			auto expression = inst->expr;

			genere_code_C(expression, constructrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			auto op = inst->op;

			POUR (inst->paires_discr) {
				auto enf0 = it.first;
				auto enf1 = it.second;

				auto feuilles = dls::tablet<NoeudExpression *, 10>();
				rassemble_feuilles(enf0, feuilles);

				for (auto f : feuilles) {
					genere_code_C(f, constructrice, contexte, true);
				}

				auto prefixe = "if (";

				for (auto f : feuilles) {
					constructrice << prefixe;

					if (op->est_basique) {
						constructrice << chaine_expr;
						constructrice << " == ";
						constructrice << f->chaine_calculee();
					}
					else {
						constructrice << op->decl->nom_broye;
						constructrice << "(contexte,";
						constructrice << chaine_expr;
						constructrice << ',';
						constructrice << f->chaine_calculee();
						constructrice << ')';
					}

					prefixe = " || ";
				}

				constructrice << ") ";

				genere_code_C(enf1, constructrice, contexte, false);

				if (it != inst->paires_discr[inst->paires_discr.taille - 1]) {
					constructrice << "else {\n";
				}
			}

			if (inst->bloc_sinon) {
				constructrice << "else {\n";
				genere_code_C(inst->bloc_sinon, constructrice, contexte, false);
			}

			/* il faut fermer tous les blocs else */
			for (auto i = 0; i < inst->paires_discr.taille - 1; ++i) {
				constructrice << "}\n";
			}

			if (inst->bloc_sinon) {
				constructrice << "}\n";
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		{
			/* le premier enfant est l'expression, les suivants les paires */
			auto inst = static_cast<NoeudDiscr *>(b);
			auto expression = inst->expr;
			auto type_enum = static_cast<TypeEnum *>(expression->type);

			genere_code_C(expression, constructrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			constructrice << "switch (" << chaine_expr << ") {\n";

			POUR (inst->paires_discr) {
				auto enf0 = it.first;
				auto enf1 = it.second;

				auto feuilles = dls::tablet<NoeudExpression *, 10>();
				rassemble_feuilles(enf0, feuilles);

				for (auto f : feuilles) {
					constructrice << "case " << chaine_valeur_enum(type_enum->decl, f->ident->nom) << ":\n";
				}

				constructrice << " {\n";
				genere_code_C(enf1, constructrice, contexte, false);
				constructrice << "break;\n}\n";
			}

			if (inst->bloc_sinon) {
				constructrice << "default:";
				constructrice << " {\n";
				genere_code_C(inst->bloc_sinon, constructrice, contexte, false);
				constructrice << "break;\n}\n";
			}

			constructrice << "}\n";

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			/* switch (union.membre_actif) {
			 * case index membre + 1: {
			 *	bloc
			 *	break;
			 * }
			 */

			auto inst = static_cast<NoeudDiscr *>(b);
			auto expression = inst->expr;
			auto type = expression->type;

			auto const est_pointeur = type->genre == GenreType::POINTEUR;

			if (est_pointeur) {
				type = static_cast<TypePointeur *>(type)->type_pointe;
			}

			auto type_struct = static_cast<TypeStructure *>(type);
			auto decl = type_struct->decl;

			genere_code_C(expression, constructrice, contexte, true);
			auto chaine_calculee = expression->chaine_calculee();
			chaine_calculee += (est_pointeur ? "->" : ".");

			constructrice << "switch (" << chaine_calculee << "membre_actif) {\n";

			POUR (inst->paires_discr) {
				auto expr_paire = it.first;
				auto bloc_paire = it.second;

				auto index_membre = 0;

				for (auto i = 0; i < decl->bloc->membres.taille; ++i) {
					if (decl->bloc->membres[i]->ident == expr_paire->ident) {
						index_membre = i;
						break;
					}
				}

				constructrice << "case " << index_membre + 1;

				constructrice << ": {\n";
				genere_code_C(bloc_paire, constructrice, contexte, true);
				constructrice << "break;\n}\n";
			}

			if (inst->bloc_sinon) {
				constructrice << "default:";
				constructrice << " {\n";
				genere_code_C(inst->bloc_sinon, constructrice, contexte, false);
				constructrice << "break;\n}\n";
			}

			constructrice << "}\n";

			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto df = contexte.donnees_fonction;
			auto enfant = inst->expr;

			constructrice << "pthread_mutex_lock(&__etat->mutex_coro);\n";

			auto feuilles = dls::tablet<NoeudExpression *, 10>{};
			rassemble_feuilles(enfant, feuilles);

			for (auto i = 0l; i < feuilles.taille(); ++i) {
				auto f = feuilles[i];

				genere_code_C(f, constructrice, contexte, true);

				constructrice << "__etat->" << df->noms_retours[i] << " = ";
				constructrice << f->chaine_calculee();
				constructrice << ";\n";
			}

			constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
			constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
			constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
			constructrice << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
			constructrice << "pthread_mutex_unlock(&__etat->mutex_coro);\n";

			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionParenthese *>(b);
			genere_code_C(expr->expr, constructrice, contexte, expr_gauche);
			b->valeur_calculee = '(' + expr->expr->chaine_calculee() + ')';
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto inst = static_cast<NoeudPousseContexte *>(b);
			auto variable = inst->expr;

			// Ajout le pointeur au nom de l'ancien_contexte pour avoir une variable unique.
			constructrice << "KsContexteProgramme ancien_contexte" << b << " = contexte;\n";
			constructrice << "contexte = " << broye_chaine(variable) << ";\n";
			genere_code_C(inst->bloc, constructrice, contexte, true);
			constructrice << "contexte = ancien_contexte" << b << ";\n";

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			applique_transformation(expr->expr, constructrice, contexte, expr_gauche);
			b->valeur_calculee = expr->expr->valeur_calculee;
			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			// À FAIRE(retours multiples)
			auto inst = static_cast<NoeudTente *>(b);

			genere_code_C(inst->expr_appel, constructrice, contexte, false);

			struct DonneesGenerationCodeTente {
				dls::chaine acces_variable{};
				dls::chaine acces_erreur{};
				dls::chaine acces_erreur_pour_test{};

				Type *type_piege = nullptr;
				Type *type_variable = nullptr;
			};

			DonneesGenerationCodeTente gen_tente;

			if (inst->expr_appel->type->genre == GenreType::ERREUR) {
				gen_tente.type_piege = inst->expr_appel->type;
				gen_tente.type_variable = gen_tente.type_piege;
				gen_tente.acces_erreur = inst->expr_appel->chaine_calculee();
				gen_tente.acces_variable = gen_tente.acces_erreur;
				gen_tente.acces_erreur_pour_test = gen_tente.acces_erreur;
			}
			else if (inst->expr_appel->type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(inst->expr_appel->type);
				auto index_membre_variable = 0;
				auto index_membre_erreur = 0;

				if (type_union->membres.taille == 2) {
					if (type_union->membres[0].type->genre == GenreType::ERREUR) {
						gen_tente.type_piege = type_union->membres[0].type;
						gen_tente.type_variable = type_union->membres[1].type;
						index_membre_variable = 1;
					}
					else {
						gen_tente.type_piege = type_union->membres[1].type;
						gen_tente.type_variable = type_union->membres[0].type;
						index_membre_erreur = 1;
					}
				}
				else {
					// À FAIRE(tente) : extraction des valeurs de l'union
				}

				gen_tente.acces_erreur = inst->expr_appel->chaine_calculee() + "." + broye_chaine(type_union->membres[index_membre_erreur].nom);
				gen_tente.acces_erreur_pour_test = inst->expr_appel->chaine_calculee() + ".membre_actif == " + dls::vers_chaine(index_membre_erreur + 1);
				gen_tente.acces_erreur_pour_test += " && ";
				gen_tente.acces_erreur_pour_test += gen_tente.acces_erreur;
				gen_tente.acces_variable = inst->expr_appel->chaine_calculee() + "." + broye_chaine(type_union->membres[index_membre_variable].nom);
			}

			constructrice << "if (" << gen_tente.acces_erreur_pour_test << " != 0) {\n";

			if (inst->expr_piege == nullptr) {
				debute_record_trace_appel(contexte, constructrice, inst->expr_appel);
				constructrice << contexte.interface_kuri.decl_panique_erreur->nom_broye << "(contexte);\n";
			}
			else {
				constructrice << nom_broye_type(gen_tente.type_piege) << " " << broye_chaine(inst->expr_piege) << " = " << gen_tente.acces_erreur << ";\n";
				genere_code_C(inst->bloc, constructrice, contexte, true);
			}

			constructrice << "}\n";

			auto nom_var_temp = "__var_temp_tent" + dls::vers_chaine(index++);
			inst->valeur_calculee = nom_var_temp;

			constructrice << nom_broye_type(gen_tente.type_variable) << ' ' << nom_var_temp;
			constructrice << " = " << gen_tente.acces_variable << ";\n";

			break;
		}
	}
}

// CHERCHE (noeud_principale : FONCTION { nom = "principale" })
// CHERCHE (noeud_principale) -[:UTILISE_FONCTION|UTILISE_TYPE*]-> (noeud)
// RETOURNE DISTINCT noeud_principale, noeud
static void traverse_graphe_pour_generation_code(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		NoeudDependance *noeud,
		bool genere_fonctions,
		bool genere_info_type)
{
	noeud->fut_visite = true;

	for (auto const &relation : noeud->relations) {
		auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
		accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
		accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

		if (!accepte) {
			continue;
		}

		if (relation.noeud_fin->fut_visite) {
			continue;
		}

		traverse_graphe_pour_generation_code(contexte, constructrice, relation.noeud_fin, genere_fonctions, genere_info_type);
	}

	if (noeud->type == TypeNoeudDependance::TYPE) {
		if (noeud->noeud_syntactique != nullptr) {
			genere_code_C(noeud->noeud_syntactique, constructrice, contexte, false);
		}

//		if (noeud->type_ == nullptr || !genere_info_type) {
//			return;
//		}

//		/* Suppression des avertissements pour les conversions dites
//		 * « imcompatibles » alors qu'elles sont bonnes.
//		 * Elles surviennent dans les assignations des pointeurs, par exemple pour
//		 * ceux des tableaux des membres des fonctions.
//		 */
//		constructrice << "#pragma GCC diagnostic push\n";
//		constructrice << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

//		cree_info_type_C(contexte, constructrice, constructrice, noeud->type_);

//		constructrice << "#pragma GCC diagnostic pop\n";
	}
	else {
		if (noeud->type == TypeNoeudDependance::FONCTION && !genere_fonctions) {
			genere_declaration_fonction(noeud->noeud_syntactique, constructrice);
			constructrice << ";\n";
			return;
		}

		genere_code_C(noeud->noeud_syntactique, constructrice, contexte, false);
	}
}

static bool peut_etre_dereference(Type *type)
{
	return type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::REFERENCE || type->genre == GenreType::POINTEUR;
}

static void genere_typedefs_recursifs(
		ContexteGenerationCode &contexte,
		Type *type,
		ConstructriceCodeC &constructrice)
{
	if ((type->drapeaux & TYPEDEF_FUT_GENERE) != 0) {
		return;
	}

	if (peut_etre_dereference(type)) {
		auto type_deref = contexte.typeuse.type_dereference_pour(type);

		/* argument variadique fonction externe */
		if (type_deref == nullptr) {
			return;
		}

		if ((type_deref->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
			genere_typedefs_recursifs(contexte, type_deref, constructrice);
		}

		type_deref->drapeaux |= TYPEDEF_FUT_GENERE;
	}
	/* ajoute les types des paramètres et de retour des fonctions */
	else if (type->genre == GenreType::FONCTION) {
		auto type_fonc = static_cast<TypeFonction *>(type);

		POUR (type_fonc->types_entrees) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(contexte, it, constructrice);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}

		POUR (type_fonc->types_sorties) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(contexte, it, constructrice);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}
	}

	cree_typedef(type, constructrice);
	type->drapeaux |= TYPEDEF_FUT_GENERE;
}

static void genere_typedefs_pour_tous_les_types(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice)
{
	POUR (contexte.typeuse.types_simples) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_pointeurs) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_references) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_structures) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_enums) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_tableaux_fixes) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_tableaux_dynamiques) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_fonctions) genere_typedefs_recursifs(contexte, it, constructrice);
	POUR (contexte.typeuse.types_unions) genere_typedefs_recursifs(contexte, it, constructrice);
}

static void genere_infos_pour_tous_les_types(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice)
{
	/* Suppression des avertissements pour les conversions dites
	 * « imcompatibles » alors qu'elles sont bonnes.
	 * Elles surviennent dans les assignations des pointeurs, par exemple pour
	 * ceux des tableaux des membres des fonctions.
	 */
	constructrice << "#pragma GCC diagnostic push\n";
	constructrice << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

	POUR (contexte.typeuse.types_simples) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_pointeurs) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_references) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_structures) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_enums) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_tableaux_fixes) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_tableaux_dynamiques) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_fonctions) predeclare_info_type_C(constructrice, it);
	POUR (contexte.typeuse.types_unions) predeclare_info_type_C(constructrice, it);

	POUR (contexte.typeuse.types_simples) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_pointeurs) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_references) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_structures) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_enums) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_tableaux_fixes) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_tableaux_dynamiques) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_fonctions) cree_info_type_C(contexte, constructrice, it);
	POUR (contexte.typeuse.types_unions) cree_info_type_C(contexte, constructrice, it);

	constructrice << "#pragma GCC diagnostic pop\n";
}

// ----------------------------------------------

static void genere_code_debut_fichier(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		assembleuse_arbre const &arbre,
		dls::chaine const &racine_kuri)
{
	for (auto const &inc : arbre.inclusions) {
		constructrice << "#include <" << inc << ">\n";
	}

	constructrice << "\n";

	constructrice << "#include <" << racine_kuri << "/fichiers/r16_c.h>\n";

	constructrice <<
R"(
#define INITIALISE_TRACE_APPEL(_nom_fonction, _taille_nom, _fichier, _taille_fichier, _pointeur_fonction) \
	static KsInfoFonctionTraceAppel mon_info = { { .pointeur = _nom_fonction, .taille = _taille_nom }, { .pointeur = _fichier, .taille = _taille_fichier }, _pointeur_fonction }; \
	KsTraceAppel ma_trace = { 0 }; \
	ma_trace.info_fonction = &mon_info; \
	ma_trace.prxC3xA9cxC3xA9dente = contexte.trace_appel; \
	ma_trace.profondeur = contexte.trace_appel->profondeur + 1;

#define DEBUTE_RECORD_TRACE_APPEL(_ligne, _colonne, _ligne_appel, _taille_ligne) \
	static KsInfoAppelTraceAppel info_appel##_ligne##_colonne = { _ligne, _colonne, { .pointeur = _ligne_appel, .taille = _taille_ligne } }; \
	ma_trace.info_appel = &info_appel##_ligne##_colonne; \
	contexte.trace_appel = &ma_trace;

#define TERMINE_RECORD_TRACE_APPEL \
   contexte.trace_appel = ma_trace.prxC3xA9cxC3xA9dente;
	)";

//	constructrice << "#include <signal.h>\n";
//	constructrice << "static void gere_erreur_segmentation(int s)\n";
//	constructrice << "{\n";
//	constructrice << "    if (s == SIGSEGV) {\n";
//	constructrice << "        " << contexte.interface_kuri.decl_panique->nom_broye << "(\"erreur de ségmentation dans une fonction\");\n";
//	constructrice << "    }\n";
//	constructrice << "    " << contexte.interface_kuri.decl_panique->nom_broye << "(\"erreur inconnue\");\n";
//	constructrice << "}\n";

	/* déclaration des types de bases */
	constructrice << "typedef struct chaine { char *pointeur; long taille; } chaine;\n";
	constructrice << "typedef struct eini { void *pointeur; struct InfoType *info; } eini;\n";
	constructrice << "#ifndef bool // bool est défini dans stdbool.h\n";
	constructrice << "typedef unsigned char bool;\n";
	constructrice << "#endif\n";
	constructrice << "typedef unsigned char octet;\n";
	constructrice << "typedef void Ksnul;\n";
	constructrice << "typedef struct ContexteProgramme KsContexteProgramme;\n";
	/* À FAIRE : pas beau, mais un pointeur de fonction peut être un pointeur
	 * vers une fonction de LibC dont les arguments variadiques ne sont pas
	 * typés */
	constructrice << "#define Kv ...\n\n";
	constructrice << "#define TAILLE_STOCKAGE_TEMPORAIRE 16384\n";
	constructrice << "static char DONNEES_STOCKAGE_TEMPORAIRE[TAILLE_STOCKAGE_TEMPORAIRE];\n\n";
	constructrice << "static int __ARGC = 0;\n";
	constructrice << "static const char **__ARGV = 0;\n\n";
}

static void ajoute_dependances_implicites(
		ContexteGenerationCode &contexte,
		NoeudDependance *noeud_fonction_principale,
		bool pour_meta_programme)
{
	auto &graphe_dependance = contexte.graphe_dependance;

	/* met en place la dépendance sur la fonction d'allocation par défaut */
	auto fonc_alloc = cherche_fonction_dans_module(contexte, "Kuri", "allocatrice_défaut");
	auto noeud_alloc = graphe_dependance.cree_noeud_fonction(fonc_alloc->nom_broye, fonc_alloc);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_alloc);

	auto fonc_init_alloc = cherche_fonction_dans_module(contexte, "Kuri", "initialise_base_allocatrice");
	auto noeud_init_alloc = graphe_dependance.cree_noeud_fonction(fonc_init_alloc->nom_broye, fonc_init_alloc);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_init_alloc);

	auto fonc_log = cherche_fonction_dans_module(contexte, "Kuri", "__logueur_défaut");
	auto noeud_log = graphe_dependance.cree_noeud_fonction(fonc_log->nom_broye, fonc_log);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_log);

	auto fonc_panique = contexte.interface_kuri.decl_panique;
	auto noeud_panique = graphe_dependance.cree_noeud_fonction(fonc_panique->nom_broye, fonc_panique);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_panique);

	auto fonc_rappel_panique = contexte.interface_kuri.decl_rappel_panique_defaut;
	auto noeud_rappel_panique = graphe_dependance.cree_noeud_fonction(fonc_rappel_panique->nom_broye, fonc_rappel_panique);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_rappel_panique);

	if (pour_meta_programme) {
		auto fonc_init = cherche_fonction_dans_module(contexte, "Kuri", "initialise_RC");
		auto noeud_init = graphe_dependance.cree_noeud_fonction(fonc_init->nom_broye, fonc_init);
		graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_init);
	}
}

static void genere_code_programme(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		NoeudDependance *noeud_fonction_principale,
		dls::chrono::compte_seconde &debut_generation)
{
	auto &graphe_dependance = contexte.graphe_dependance;

	/* création primordiale des typedefs pour éviter les problèmes liés aux
	 * types récursifs, inclus tous les types car dans certains cas il manque
	 * des connexions entre types... */
	genere_typedefs_pour_tous_les_types(contexte, constructrice);

	/* il faut d'abord créer le code pour les structures InfoType */
	const char *noms_structs_infos_types[] = {
		"InfoType",
		"InfoTypeEntier",
		"InfoTypePointeur",
		"InfoTypeÉnum",
		"InfoTypeStructure",
		"InfoTypeTableau",
		"InfoTypeFonction",
		"InfoTypeMembreStructure",
		"ContexteProgramme",
	};

	for (auto nom_struct : noms_structs_infos_types) {
		auto type_struct = contexte.typeuse.type_pour_nom(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(type_struct);

		traverse_graphe_pour_generation_code(contexte, constructrice, noeud, false, false);
		/* restaure le drapeaux pour la génération des infos des types */
		noeud->fut_visite = false;
	}

//	for (auto nom_struct : noms_structs_infos_types) {
//		auto const &ds = contexte.donnees_structure(nom_struct);
//		auto noeud = graphe_dependance.cherche_noeud_type(ds.type);
//		traverse_graphe_pour_generation_code(contexte, constructrice, noeud, false, true);
//	}

	contexte.temps_generation += debut_generation.temps();

	//reduction_transitive(graphe_dependance);

	genere_infos_pour_tous_les_types(contexte, constructrice);

	debut_generation.commence();

	/* génère d'abord les déclarations des fonctions et les types */
	traverse_graphe_pour_generation_code(contexte, constructrice, noeud_fonction_principale, false, true);

	/* génère ensuite les fonctions */
	for (auto noeud_dep : graphe_dependance.noeuds) {
		if (noeud_dep->type == TypeNoeudDependance::FONCTION) {
			noeud_dep->fut_visite = false;
		}
	}

	traverse_graphe_pour_generation_code(contexte, constructrice, noeud_fonction_principale, true, false);
}

static void genere_code_creation_contexte(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice)
{
	auto creation_contexte =
R"(
	KsStockageTemporaire stockage_temp;
	stockage_temp.donnxC3xA9es = DONNEES_STOCKAGE_TEMPORAIRE;
	stockage_temp.taille = TAILLE_STOCKAGE_TEMPORAIRE;
	stockage_temp.occupxC3xA9 = 0;
	stockage_temp.occupation_maximale = 0;

	KsContexteProgramme contexte;
	contexte.trace_appel = &ma_trace;
	contexte.stockage_temporaire = &stockage_temp;

	KsBaseAllocatrice alloc_base;
	contexte.donnxC3xA9es_allocatrice = &alloc_base;

	contexte.donnxC3xA9es_logueur = 0;

	contexte.donnxC3xA9es_rappel_panique = 0;
)";

	auto fonc_alloc = cherche_fonction_dans_module(contexte, "Kuri", "allocatrice_défaut");
	auto fonc_init_alloc = cherche_fonction_dans_module(contexte, "Kuri", "initialise_base_allocatrice");
	auto fonc_log = cherche_fonction_dans_module(contexte, "Kuri", "__logueur_défaut");

	constructrice << creation_contexte;
	constructrice << "contexte.allocatrice = " << fonc_alloc->nom_broye << ";\n";
	constructrice << fonc_init_alloc->nom_broye << "(contexte, &alloc_base);\n";
	constructrice << "contexte.logueur = " << fonc_log->nom_broye << ";\n";
	constructrice << "contexte.rappel_panique = " << contexte.interface_kuri.decl_rappel_panique_defaut->nom_broye << ";\n";
}

void genere_code_C(
		assembleuse_arbre const &arbre,
		ContexteGenerationCode &contexte,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	//auto nombre_allocations = memoire::nombre_allocations();

	auto temps_generation = 0.0;

	auto &graphe_dependance = contexte.graphe_dependance;
	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction("principale");

	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	auto constructrice = ConstructriceCodeC(contexte);

	genere_code_debut_fichier(contexte, constructrice, arbre, racine_kuri);

	ajoute_dependances_implicites(contexte, noeud_fonction_principale, false);

	auto debut_generation = dls::chrono::compte_seconde();

	genere_code_programme(contexte, constructrice, noeud_fonction_principale, debut_generation);

	constructrice << "int main(int argc, char **argv)\n";
	constructrice << "{\n";
	constructrice << "    static KsInfoFonctionTraceAppel mon_info = { { .pointeur = \"main\", .taille = 4 }, { .pointeur = \"???\", .taille = 3 }, main };\n";
	constructrice << "    KsTraceAppel ma_trace = { 0 };\n";
	constructrice << "    ma_trace.info_fonction = &mon_info;\n";
	constructrice << "    __ARGV = argv;\n";
	constructrice << "    __ARGC = argc;\n";
//	constructrice << "    signal(SIGSEGV, gere_erreur_segmentation);\n";

	genere_code_creation_contexte(contexte, constructrice);

	constructrice << "    DEBUTE_RECORD_TRACE_APPEL(1, 0, \"principale(contexte);\", 21);\n";
	constructrice << "    return principale(contexte);";
	constructrice << "}\n";

	temps_generation += debut_generation.temps();

	contexte.temps_generation = temps_generation;

	constructrice.imprime_dans_flux(fichier_sortie);

	//std::cout << "Nombre allocations génération code = " << memoire::nombre_allocations() - nombre_allocations << '\n';
}

void genere_code_C_pour_execution(
		assembleuse_arbre const &arbre,
		NoeudExpression *noeud_appel,
		ContexteGenerationCode &contexte,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	auto dir = static_cast<NoeudExpressionUnaire *>(noeud_appel);
	auto expr = static_cast<NoeudExpressionAppel *>(dir->expr);
	auto decl = static_cast<NoeudDeclarationFonction const *>(expr->noeud_fonction_appelee);


	auto temps_generation = 0.0;

	auto &graphe_dependance = contexte.graphe_dependance;

	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction(decl->nom_broye);
	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	auto constructrice = ConstructriceCodeC(contexte);

	genere_code_debut_fichier(contexte, constructrice, arbre, racine_kuri);

	/* met en place la dépendance sur la fonction d'allocation par défaut */
	ajoute_dependances_implicites(contexte, noeud_fonction_principale, true);

	auto debut_generation = dls::chrono::compte_seconde();

	genere_code_programme(contexte, constructrice, noeud_fonction_principale, debut_generation);

	constructrice << "void lance_execution()\n";
	constructrice << "{\n";

	genere_code_creation_contexte(contexte, constructrice);

	genere_code_C(expr, constructrice, contexte, true);

	constructrice << "    return;\n";
	constructrice << "}\n";

	temps_generation += debut_generation.temps();

	contexte.temps_generation = temps_generation;

	constructrice.imprime_dans_flux(fichier_sortie);

	/* réinitialise les données pour la génération du code finale
	 * À FAIRE: sauvegarde les tampons sources */
	for (auto noeud_dep : graphe_dependance.noeuds) {
		noeud_dep->fut_visite = false;
		noeud_dep->deja_genere = false;
	}

	POUR (contexte.typeuse.types_simples) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_pointeurs) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_references) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_structures) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; it->deja_genere = false; }
	POUR (contexte.typeuse.types_enums) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_tableaux_fixes) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_tableaux_dynamiques) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_fonctions) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_variadiques) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_unions) { it->ptr_info_type = ""; it->drapeaux &= ~TYPEDEF_FUT_GENERE; it->deja_genere = false; };
}

static void genere_code_c_pour_atome(Atome *atome, std::ostream &os)
{
	switch (atome->genre_atome) {
		case Atome::Genre::FONCTION:
		{
			auto atome_fonc = static_cast<AtomeFonction const *>(atome);
			os << atome_fonc->nom;
			break;
		}
		case Atome::Genre::CONSTANTE:
		{
			auto atome_const = static_cast<AtomeConstante const *>(atome);

			switch (atome_const->valeur.genre) {
				case AtomeConstante::Valeur::Genre::NULLE:
				{
					os << "0";
					break;
				}
				case AtomeConstante::Valeur::Genre::CHAINE:
				{
					os << "";
					break;
				}
				case AtomeConstante::Valeur::Genre::REELLE:
				{
					os << atome_const->valeur.valeur_reelle;
					break;
				}
				case AtomeConstante::Valeur::Genre::ENTIERE:
				{
					os << atome_const->valeur.valeur_entiere;
					break;
				}
				case AtomeConstante::Valeur::Genre::BOOLEENNE:
				{
					os << atome_const->valeur.valeur_booleenne;
					break;
				}
				case AtomeConstante::Valeur::Genre::CARACTERE:
				{
					os << atome_const->valeur.valeur_entiere;
					break;
				}
				case AtomeConstante::Valeur::Genre::INDEFINIE:
				{
					break;
				}
				case AtomeConstante::Valeur::Genre::STRUCTURE:
				{
					break;
				}
			}

			break;
		}
		case Atome::Genre::INSTRUCTION:
		{
			auto inst = static_cast<Instruction const *>(atome);

			if (inst->genre == Instruction::Genre::ALLOCATION) {
				os << inst->ident->nom;
			}
			else {
				os << "val" << inst->numero;
			}

			break;
		}
		case Atome::Genre::GLOBALE:
		{
			break;
		}
	}
}

static void genere_code_c_pour_instruction(Instruction const *inst, std::ostream &os)
{
	switch (inst->genre) {
		case Instruction::Genre::INVALIDE:
		{
			os << "  invalide\n";
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			os << "  " << nom_broye_type(inst->type) << ' ' << inst->ident->nom << ";\n";
			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto inst_appel = static_cast<InstructionAppel const *>(inst);

			os << "  ";

			if  (inst_appel->type->genre != GenreType::RIEN) {
				os << nom_broye_type(inst_appel->type) << ' ' << "__ret = ";
			}

			genere_code_c_pour_atome(inst_appel->appele, os);

			auto virgule = '(';

			POUR (inst_appel->args) {
				os << virgule;
				genere_code_c_pour_atome(it, os);
				virgule = ',';
			}

			if (inst_appel->args.taille == 0) {
				os << virgule;
			}

			os << ");\n";

			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto inst_branche = static_cast<InstructionBranche const *>(inst);
			os << "  goto label" << inst_branche->label->numero << ";\n";
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto inst_branche = static_cast<InstructionBrancheCondition const *>(inst);
			os << "  if (";
			genere_code_c_pour_atome(inst_branche->condition, os);
			os << ") goto label" << inst_branche->label_si_vrai->id << "; ";
			os << "else goto label" << inst_branche->label_si_faux->id << ";\n";
			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			os << "  charge\n";
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto inst_stocke = static_cast<InstructionStockeMem const *>(inst);
			os << "  " << inst_stocke->ou->ident->nom << " = ";
			genere_code_c_pour_atome(inst_stocke->valeur, os);
			os << ";\n";
			break;
		}
		case Instruction::Genre::LABEL:
		{
			auto inst_label = static_cast<InstructionLabel const *>(inst);
			os << "label" << inst_label->id << ":\n";
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto inst_un = static_cast<InstructionOpUnaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_un->op)
			   << ' ' << nom_broye_type(inst_un->type);
			genere_code_c_pour_atome(inst_un->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto inst_bin = static_cast<InstructionOpBinaire const *>(inst);

			os << "  " << nom_broye_type(inst_bin->type) << " val" << inst->numero << " = ";

			genere_code_c_pour_atome(inst_bin->valeur_gauche, os);

			switch (inst_bin->op) {
				case OperateurBinaire::Genre::Addition:
				{
					os << " + ";
					break;
				}
				case OperateurBinaire::Genre::Soustraction:
				{
					os << " - ";
					break;
				}
				case OperateurBinaire::Genre::Multiplication:
				{
					os << " * ";
					break;
				}
				case OperateurBinaire::Genre::Division:
				{
					os << " / ";
					break;
				}
				case OperateurBinaire::Genre::Reste:
				{
					os << " % ";
					break;
				}
				case OperateurBinaire::Genre::Comp_Egal:
				{
					os << " == ";
					break;
				}
				case OperateurBinaire::Genre::Comp_Inegal:
				{
					os << " != ";
					break;
				}
				case OperateurBinaire::Genre::Comp_Inf:
				{
					os << " < ";
					break;
				}
				case OperateurBinaire::Genre::Comp_Inf_Egal:
				{
					os << " <= ";
					break;
				}
				case OperateurBinaire::Genre::Comp_Sup:
				{
					os << " > ";
					break;
				}
				case OperateurBinaire::Genre::Comp_Sup_Egal:
				{
					os << " >= ";
					break;
				}
				case OperateurBinaire::Genre::Et_Logique:
				{
					os << " && ";
					break;
				}
				case OperateurBinaire::Genre::Ou_Logique:
				{
					os << " || ";
					break;
				}
				case OperateurBinaire::Genre::Et_Binaire:
				{
					os << " & ";
					break;
				}
				case OperateurBinaire::Genre::Ou_Binaire:
				{
					os << " | ";
					break;
				}
				case OperateurBinaire::Genre::Ou_Exclusif:
				{
					os << " ^ ";
					break;
				}
				case OperateurBinaire::Genre::Dec_Gauche:
				{
					os << " << ";
					break;
				}
				case OperateurBinaire::Genre::Dec_Droite:
				{
					os << " >> ";
					break;
				}
				case OperateurBinaire::Genre::Invalide:
				{
					os << " invalide ";
					break;
				}
			}

			genere_code_c_pour_atome(inst_bin->valeur_droite, os);

			os << ";\n";

			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto inst_retour = static_cast<InstructionRetour const *>(inst);
			os << "  return ";
			if (inst_retour->valeur != nullptr) {
				auto atome = inst_retour->valeur;
				genere_code_c_pour_atome(atome, os);
			}
			os << ";\n";
			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto inst_acces = static_cast<InstructionAccedeIndex const *>(inst);
			genere_code_c_pour_atome(inst_acces->accede, os);
			os << '[';
			genere_code_c_pour_atome(inst_acces->index, os);
			os << "];\n";
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto inst_acces = static_cast<InstructionAccedeMembre const *>(inst);
			genere_code_c_pour_atome(inst_acces->accede, os);
			os << ".";
			genere_code_c_pour_atome(inst_acces->index, os);
			os << ";\n";
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			auto inst_transtype = static_cast<InstructionTranstype const *>(inst);
			os << "(" << nom_broye_type(inst_transtype->type) << ")(";
			genere_code_c_pour_atome(inst_transtype->valeur, os);
			os << ")";
			break;
		}
	}
}

void genere_code_C(ConstructriceRI &contructrice_ri)
{
	std::ostream &os = std::cerr;

	POUR (contructrice_ri.programme) {
		switch (it->genre_atome) {
			case Atome::Genre::FONCTION:
			{
				auto atome_fonc = static_cast<AtomeFonction const *>(it);
				os << "void " << atome_fonc->nom;

				auto virgule = '(';

				for (auto param : atome_fonc->params_entrees) {
					os << virgule;
					os << chaine_type(param->type) << ' ';
					os << param->ident->nom;
					virgule = ',';
				}

				if (atome_fonc->params_entrees.taille == 0) {
					os << virgule;
				}

				os << ")\n{\n";

				for (auto inst : atome_fonc->instructions) {
					genere_code_c_pour_instruction(static_cast<Instruction const *>(inst), os);
				}

				os << "}\n";

				break;
			}
			case Atome::Genre::CONSTANTE:
			{
				break;
			}
			case Atome::Genre::INSTRUCTION:
			{
				break;
			}
			case Atome::Genre::GLOBALE:
			{
				break;
			}
		}
	}
}

}  /* namespace noeud */
