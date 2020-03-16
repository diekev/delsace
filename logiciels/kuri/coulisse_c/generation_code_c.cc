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

#include "generation_code_c.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "conversion_type_c.hh"
#include "erreur.h"
#include "generatrice_code_c.hh"
#include "info_type_c.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
#include "typage.hh"

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

namespace noeud {

/* ************************************************************************** */

/* À FAIRE : trouve une bonne manière de générer des noms uniques. */
static int index = 0;

void genere_code_C(
		NoeudExpression *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche);

static void applique_transformation(
		NoeudExpression *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	/* force une expression gauche afin de ne pas prendre la référence d'une
	 * variable temporaire */
	expr_gauche |= b->transformation.type == TypeTransformation::PREND_REFERENCE;
	expr_gauche |= b->transformation.type == TypeTransformation::CONSTRUIT_EINI;
	genere_code_C(b, generatrice, contexte, expr_gauche);

	auto nom_courant = b->chaine_calculee();
	auto type = b->type;

	auto nom_var_temp = "__var_temp_conv" + dls::vers_chaine(index++);

	switch (b->transformation.type) {
		case TypeTransformation::IMPOSSIBLE:
		{
			break;
		}
		case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
		case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
		case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
		case TypeTransformation::INUTILE:
		case TypeTransformation::PREND_PTR_RIEN:
		case TypeTransformation::AUGMENTE_TAILLE_TYPE:
		{
			nom_var_temp = nom_courant;
			break;
		}
		case TypeTransformation::CONSTRUIT_EINI:
		{
			/* dans le cas d'un nombre ou d'une chaine, etc. */
			if (!est_valeur_gauche(b->genre_valeur)) {
				generatrice.declare_variable(type, nom_var_temp, b->chaine_calculee());
				nom_courant = nom_var_temp;
				nom_var_temp = "__var_temp_eini" + dls::vers_chaine(index++);
			}

			generatrice << "Kseini " << nom_var_temp << " = {\n";
			generatrice << "\t.pointeur = &" << nom_courant << ",\n";
			generatrice << "\t.info = (InfoType *)(&" << type->ptr_info_type << ")\n";
			generatrice << "};\n";

			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			generatrice << nom_broye_type(b->transformation.type_cible, true) << " " << nom_var_temp << " = *(";
			generatrice << nom_broye_type(b->transformation.type_cible, true) << " *)(" << nom_courant << ".pointeur);\n";
			break;
		}
		case TypeTransformation::CONSTRUIT_TABL_OCTET:
		{
			/* dans le cas d'un nombre ou d'un tableau, etc. */
			if (!est_valeur_gauche(b->genre_valeur)) {
				generatrice.declare_variable(type, nom_var_temp, b->chaine_calculee());
				nom_courant = nom_var_temp;
				nom_var_temp = "__var_temp_eini" + dls::vers_chaine(index++);
			}

			generatrice << "KtKsoctet " << nom_var_temp << " = {\n";

			switch (type->genre) {
				default:
				{
					generatrice << "\t.pointeur = (unsigned char *)(&" << nom_courant << "),\n";
					generatrice << "\t.taille = sizeof(" << nom_broye_type(type, true) << ")\n";
					break;
				}
				case GenreType::POINTEUR:
				{
					generatrice << "\t.pointeur = (unsigned char *)(" << nom_courant << "),\n";
					generatrice << "\t.taille = sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					generatrice << nom_broye_type(type_deref, true) << ")\n";
					break;
				}
				case GenreType::CHAINE:
				{
					generatrice << "\t.pointeur = " << nom_courant << ".pointeur,\n";
					generatrice << "\t.taille = " << nom_courant << ".taille,\n";
					break;
				}
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					generatrice << "\t.pointeur = (unsigned char *)(&" << nom_courant << ".pointeur),\n";
					generatrice << "\t.taille = " << nom_courant << ".taille * sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					generatrice << nom_broye_type(type_deref, true) << ")\n";
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					generatrice << "\t.pointeur = " << nom_courant << ",\n";
					generatrice << "\t.taille = " << static_cast<TypeTableauFixe *>(type)->taille << " * sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					generatrice << nom_broye_type(type_deref, true) << ")\n";
					break;
				}
			}

			generatrice << "};\n";

			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);
			auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_deref);
			generatrice << nom_broye_type(type_tabl, true) << ' ' << nom_var_temp << " = {\n";
			generatrice << "\t.taille = " << static_cast<TypeTableauFixe *>(type)->taille << ",\n";
			generatrice << "\t.pointeur = " << nom_courant << "\n";
			generatrice << "};";

			break;
		}
		case TypeTransformation::FONCTION:
		{
			generatrice << nom_broye_type(b->transformation.type_cible, true) << ' ';
			generatrice << nom_var_temp << " = " << b->transformation.nom_fonction << '(';
			generatrice << nom_courant << ");\n";

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
				generatrice << nom_broye_type(type_deref, true);
				generatrice << ' ' << nom_var_temp << " = *(" << nom_courant << ");\n";
			}

			break;
		}
		case TypeTransformation::CONVERTI_VERS_BASE:
		{
			//generatrice.declare_variable(b->transformation.type_cible, nom_var_temp, b->chaine_calculee());
			nom_var_temp = nom_courant;
			break;
		}
	}

	b->valeur_calculee = nom_var_temp;
}

static void genere_code_extra_pre_retour(
		NoeudBloc *bloc,
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice)
{
	if (contexte.donnees_fonction->est_coroutine) {
		generatrice << "__etat->__termine_coro = 1;\n";
		generatrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		generatrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
		generatrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	}

	/* génère le code pour les blocs déférés */
	while (bloc != nullptr) {
		for (auto i = bloc->noeuds_differes.taille - 1; i >= 0; --i) {
			auto bloc_differe = bloc->noeuds_differes[i];
			bloc_differe->est_differe = false;
			genere_code_C(bloc_differe, generatrice, contexte, false);
			bloc_differe->est_differe = true;
		}

		bloc = bloc->parent;
	}
}

/* ************************************************************************** */

static inline auto broye_chaine(NoeudExpression *b)
{
	return broye_nom_simple(b->ident->nom);
}

static inline auto broye_chaine(dls::vue_chaine_compacte const &chn)
{
	return broye_nom_simple(chn);
}

/* ************************************************************************** */

static void cree_appel(
		NoeudExpression *b,
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice)
{
	auto expr_appel = static_cast<NoeudExpressionAppel *>(b);
	auto decl_fonction_appelee = static_cast<NoeudDeclarationFonction *>(expr_appel->noeud_fonction_appelee);

	auto pour_appel_precedent = contexte.pour_appel;
	contexte.pour_appel = static_cast<NoeudExpressionAppel *>(b);
	POUR (expr_appel->exprs) {
		applique_transformation(it, generatrice, contexte, false);
	}
	contexte.pour_appel = pour_appel_precedent;

	auto type = b->type;

	dls::liste<NoeudExpression *> liste_var_retour{};
	dls::tableau<dls::chaine> liste_noms_retour{};

	if (b->aide_generation_code == APPEL_FONCTION_MOULT_RET) {
		liste_var_retour = std::any_cast<dls::liste<NoeudExpression *>>(b->valeur_calculee);
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
	else if (type->genre != GenreType::RIEN && (b->aide_generation_code == APPEL_POINTEUR_FONCTION || ((decl_fonction_appelee != nullptr) && !decl_fonction_appelee->est_coroutine))) {
		auto nom_indirection = "__ret" + dls::vers_chaine(b);
		generatrice << nom_broye_type(type, true) << ' ' << nom_indirection << " = ";
		b->valeur_calculee = nom_indirection;
	}
	else {
		/* la valeur calculée doit être toujours valide. */
		b->valeur_calculee = dls::chaine("");
	}

	generatrice << expr_appel->nom_fonction_appel;

	auto virgule = '(';

	if (expr_appel->params.taille == 0 && liste_var_retour.est_vide() && liste_noms_retour.est_vide()) {
		generatrice << virgule;
		virgule = ' ';
	}

	if (decl_fonction_appelee != nullptr) {
		if (decl_fonction_appelee->est_coroutine) {
			generatrice << virgule << "&__etat" << dls::vers_chaine(b) << ");\n";
			return;
		}

		if (!decl_fonction_appelee->est_externe && !dls::outils::possede_drapeau(decl_fonction_appelee->drapeaux, FORCE_NULCTX)) {
			generatrice << virgule;
			generatrice << "contexte";
			virgule = ',';
		}
	}
	else {
		if (!dls::outils::possede_drapeau(expr_appel->drapeaux, FORCE_NULCTX)) {
			generatrice << virgule;
			generatrice << "contexte";
			virgule = ',';
		}
	}

	POUR (expr_appel->exprs) {
		generatrice << virgule;
		generatrice << it->chaine_calculee();
		virgule = ',';
	}

	for (auto n : liste_var_retour) {
		generatrice << virgule;
		generatrice << "&(" << n->chaine_calculee() << ')';
		virgule = ',';
	}

	for (auto n : liste_noms_retour) {
		generatrice << virgule << n;
		virgule = ',';
	}

	generatrice << ");\n";
}

static void genere_code_acces_membre(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		NoeudExpression *b,
		NoeudExpression *structure,
		NoeudExpression *membre,
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
			genere_code_C(structure, generatrice, contexte, expr_gauche);

			if (membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				genere_code_C(membre, generatrice, contexte, expr_gauche);
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
			generatrice << "long " << nom_acces << " = " << taille_tableau << ";\n";
			flux << nom_acces;
		}
		else if (type_structure->genre == GenreType::ENUM) {
			auto type_enum = static_cast<TypeEnum *>(type_structure);
			flux << chaine_valeur_enum(type_enum->decl, membre->ident->nom);
		}
		else if (type_structure->genre == GenreType::STRUCTURE) {
			genere_code_C(structure, generatrice, contexte, expr_gauche);

			if (est_reference) {
				flux << "(*" << structure->chaine_calculee() << ')';
			}
			else {
				flux << structure->chaine_calculee();
			}

			flux << ((est_pointeur) ? "->" : ".");

			/* À FAIRE: variable de retour pour les appels de pointeur de fonction */
			if (membre->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
				generatrice << flux.chn();
				membre->nom_fonction_appel = broye_chaine(membre);
				genere_code_C(membre, generatrice, contexte, false);
			}
			else {
				if (membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					genere_code_C(membre, generatrice, contexte, expr_gauche);
				}

				flux << broye_chaine(membre);
			}
		}
		else {
			genere_code_C(structure, generatrice, contexte, expr_gauche);

			if (membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				genere_code_C(membre, generatrice, contexte, expr_gauche);
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
		if (membre->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
			membre->nom_fonction_appel = flux.chn();
			genere_code_C(membre, generatrice, contexte, false);

			generatrice.declare_variable(b->type, nom_acces, membre->chaine_calculee());

			b->valeur_calculee = nom_acces;
		}
		else {
			auto nom_var = "__var_temp_acc" + dls::vers_chaine(index++);
			generatrice << "const ";
			generatrice.declare_variable(b->type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
		}
	}
}

static void genere_code_echec_logement(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::chaine const &nom_ptr,
		NoeudExpression *b,
		NoeudExpression *bloc)
{
	generatrice << "if (" << nom_ptr  << " == 0)";

	if (bloc) {
		genere_code_C(bloc, generatrice, contexte, true);
	}
	else {
		auto const &lexeme = b->lexeme;
		auto module = contexte.fichier(static_cast<size_t>(lexeme->fichier));
		auto pos = trouve_position(*lexeme, module);

		generatrice << " {\n";
		generatrice << "KR__hors_memoire(";
		generatrice << '"' << module->chemin << '"' << ',';
		generatrice << pos.numero_ligne;
		generatrice << ");\n";
		generatrice << "}\n";
	}
}

static void genere_declaration_structure(GeneratriceCodeC &generatrice, NoeudStruct *decl)
{
	if (decl->est_externe) {
		return;
	}

	auto nom_broye = broye_nom_simple(decl->ident->nom);

	if (decl->est_union) {
		if (decl->est_nonsure) {
			generatrice << "typedef union " << nom_broye << "{\n";
		}
		else {
			generatrice << "typedef struct " << nom_broye << "{\n";
			generatrice << "int membre_actif;\n";
			generatrice << "union {\n";
		}
	}
	else {
		generatrice << "typedef struct " << nom_broye << "{\n";
	}

	POUR (decl->desc.membres) {
		auto nom = broye_nom_simple(dls::vue_chaine_compacte(it.nom.pointeur, it.nom.taille));
		generatrice << nom_broye_type(it.type, true) << ' ' << nom << ";\n";
	}

	if (decl->est_union && !decl->est_nonsure) {
		generatrice << "};\n";
	}

	generatrice << "} " << nom_broye << ";\n\n";
}

static void cree_initialisation_structure(GeneratriceCodeC &generatrice, Type *type, NoeudStruct *decl)
{
	generatrice << '\n';
	generatrice << "void initialise_" << type->nom_broye << "("
				   << type->nom_broye << " *pointeur)\n";
	generatrice << "{\n";

	POUR (decl->desc.membres) {
		auto type_membre = it.type;
		auto nom_broye_membre = broye_chaine(dls::vue_chaine_compacte(it.nom.pointeur, it.nom.taille));

		// À FAIRE : structures employées, expressions par défaut

		if (type_membre->genre == GenreType::CHAINE || type_membre->genre == GenreType::TABLEAU_DYNAMIQUE) {
			generatrice << "pointeur->" << nom_broye_membre << ".pointeur = 0;\n";
			generatrice << "pointeur->" << nom_broye_membre << ".taille = 0;\n";
		}
		else if (type_membre->genre == GenreType::EINI) {
			generatrice << "pointeur->" << nom_broye_membre << ".pointeur = 0\n;";
			generatrice << "pointeur->" << nom_broye_membre << ".info = 0\n;";
		}
		else if (type_membre->genre == GenreType::ENUM) {
			generatrice << "pointeur->" << nom_broye_membre << " = 0;\n";
		}
		else if (type_membre->genre == GenreType::UNION) {
			auto type_union = static_cast<TypeUnion *>(type_membre);

			/* À FAIRE: initialise le type le plus large */
			if (!type_union->decl->est_nonsure) {
				generatrice << "pointeur->" << nom_broye_membre << ".membre_actif = 0;\n";
			}
		}
		else if (type_membre->genre == GenreType::STRUCTURE) {
			generatrice << "initialise_" << type_membre->nom_broye << "(&pointeur->" << nom_broye_membre << ");\n";
		}
		else if (type_membre->genre == GenreType::TABLEAU_FIXE) {
			/* À FAIRE */
		}
		else {
			generatrice << "pointeur->" << nom_broye_membre << " = 0;\n";
		}
	}

	generatrice << "}\n";
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

	auto pos = trouve_position(*b->lexeme, fichier);

	flux << "{ ";
	flux << ".fichier = { .pointeur = \"" << fichier->nom << ".kuri\", .taille = " << fichier->nom.taille() + 5 << " },";
	flux << ".fonction = { .pointeur = \"" << nom_fonction << "\", .taille = " << nom_fonction.taille() << " },";
	flux << ".ligne = " << pos.numero_ligne << " ,";
	flux << ".colonne = " << pos.pos;
	flux << " };";
}

static void genere_code_allocation(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
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
	auto expr_acces_taille = dls::chaine("");
	auto expr_pointeur = dls::chaine("");
	auto chn_enfant = dls::chaine("");

	/* variable n'est nul que pour les allocations simples */
	if (variable != nullptr) {
		assert(mode == 1 || mode == 2);
		applique_transformation(variable, generatrice, contexte, true);
		chn_enfant = variable->chaine_calculee();
	}
	else {
		assert(mode == 0);
		chn_enfant = generatrice.declare_variable_temp(type, index++);
	}

	switch (type->genre) {
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			expr_pointeur = chn_enfant + ".pointeur";
			expr_acces_taille = chn_enfant + ".taille";
			expr_ancienne_taille_octet = expr_acces_taille;
			expr_ancienne_taille_octet += " * sizeof(" + nom_broye_type(type_deref, true) + ")";

			/* allocation ou réallocation */
			if (!b->type_declare.expressions.est_vide()) {
				expression = b->type_declare.expressions[0];
				genere_code_C(expression, generatrice, contexte, false);
				expr_nouvelle_taille = expression->chaine_calculee();
				expr_nouvelle_taille_octet = expr_nouvelle_taille;
				expr_nouvelle_taille_octet += " * sizeof(" + nom_broye_type(type_deref, true) + ")";
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
			expr_acces_taille = chn_enfant + ".taille";
			expr_ancienne_taille_octet = expr_acces_taille;

			/* allocation ou réallocation */
			if (expression != nullptr) {
				genere_code_C(expression, generatrice, contexte, false);
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
			expr_ancienne_taille_octet = "sizeof(" + nom_broye_type(type_deref, true) + ")";

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

	generatrice << "long " << nom_nouvelle_taille << " = " << expr_nouvelle_taille_octet << ";\n";
	generatrice << "long " << nom_ancienne_taille << " = " << expr_ancienne_taille_octet << ";\n";
	generatrice << "InfoType *" << nom_info_type << " = (InfoType *)(&" << type->ptr_info_type << ");\n";
	generatrice << "void *" << nom_ancien_pointeur << " = " << expr_pointeur << ";\n";

	auto nom_pos = "__var_temp_pos" + dls::vers_chaine(index++);
	generatrice << "PositionCodeSource " << nom_pos << " = ";
	genere_code_position_source(contexte, generatrice.os, b);
	generatrice << "\n;";

	generatrice << "void *" << nom_nouveu_pointeur << " = ";
	generatrice << "contexte.allocatrice(contexte, ";
	generatrice << mode << ',';
	generatrice << nom_nouvelle_taille << ',';
	generatrice << nom_ancienne_taille << ',';
	generatrice << nom_ancien_pointeur << ',';
	generatrice << broye_nom_simple("contexte.données_allocatrice") << ',';
	generatrice << nom_info_type << ',';
	generatrice << nom_pos << ");\n";
	generatrice << expr_pointeur << " = " << nom_nouveu_pointeur << ";\n";

	switch (mode) {
		case 0:
		{
			genere_code_echec_logement(
						contexte,
						generatrice,
						expr_pointeur,
						b,
						bloc_sinon);

			if (type->genre != GenreType::CHAINE && type->genre != GenreType::TABLEAU_DYNAMIQUE) {
				auto type_deref = contexte.typeuse.type_dereference_pour(type);

				if (type_deref->genre == GenreType::STRUCTURE) {
					generatrice << "initialise_" << type_deref->nom_broye << '(' << expr_pointeur << ");\n";
				}
			}

			if (expr_acces_taille.taille() != 0) {
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
						generatrice,
						expr_pointeur,
						b,
						bloc_sinon);

			break;
		}
	}

	/* Il faut attendre d'avoir généré le code d'ajournement de la mémoire avant
	 * de modifier la taille. */
	if (expr_acces_taille.taille() != 0) {
		generatrice << expr_acces_taille << " = " << expr_nouvelle_taille <<  ";\n";
	}
}

static void genere_declaration_fonction(
		NoeudExpression *b,
		GeneratriceCodeC &generatrice)
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
			generatrice << "static inline void ";
		}
		else if (possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
			generatrice << "static void __attribute__ ((noinline)) ";
		}
		else {
			generatrice << "static void ";
		}

		generatrice << nom_fonction;
	}
	else {
		if (possede_drapeau(b->drapeaux, FORCE_ENLIGNE)) {
			generatrice << "static inline ";
		}
		else if (possede_drapeau(b->drapeaux, FORCE_HORSLIGNE)) {
			generatrice << "__attribute__ ((noinline)) ";
		}

		auto type_fonc = static_cast<TypeFonction *>(b->type);
		generatrice << nom_broye_type(type_fonc->types_sorties[0], true) << ' ' << nom_fonction;
	}

	/* Code pour les arguments */

	auto virgule = '(';

	if (decl->params.taille == 0 && !moult_retour) {
		generatrice << '(' << '\n';
		virgule = ' ';
	}

	if (!decl->est_externe && !possede_drapeau(decl->drapeaux, FORCE_NULCTX)) {
		generatrice << virgule << '\n';
		generatrice << "KsContexteProgramme contexte";
		virgule = ',';
	}

	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident->nom);

		generatrice << virgule << '\n';
		generatrice << nom_broye_type(it->type, true) << ' ' << nom_broye;

		virgule = ',';
	}

	if (moult_retour) {
		auto idx_ret = 0l;
		POUR (decl->type_fonc->types_sorties) {
			generatrice << virgule << '\n';

			auto nom_ret = "*" + decl->noms_retours[idx_ret++];

			generatrice << nom_broye_type(it, true) << ' ' << nom_ret;
			virgule = ',';
		}
	}

	generatrice << ")";
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
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	switch (b->genre) {
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::RACINE:
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

			genere_declaration_fonction(b, generatrice);
			generatrice << '\n';

			auto decl = static_cast<NoeudDeclarationFonction *>(b);
			contexte.commence_fonction(decl);

			generatrice << "{\n";

			/* Crée code pour le bloc. */
			decl->bloc->est_bloc_fonction = true;
			genere_code_C(decl->bloc, generatrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(decl->bloc, contexte, generatrice);
			}

			generatrice << "}\n";

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
			generatrice << "typedef struct " << nom_type_coro << " {\n";
			generatrice << "pthread_mutex_t mutex_boucle;\n";
			generatrice << "pthread_cond_t cond_boucle;\n";
			generatrice << "pthread_mutex_t mutex_coro;\n";
			generatrice << "pthread_cond_t cond_coro;\n";
			generatrice << "bool __termine_coro;\n";
			generatrice << "ContexteProgramme contexte;\n";

			auto idx_ret = 0l;
			POUR (decl->type_fonc->types_sorties) {
				auto &nom_ret = decl->noms_retours[idx_ret++];
				generatrice.declare_variable(it, nom_ret, "");
			}

			POUR (decl->params) {
				auto nom_broye = broye_nom_simple(it->ident->nom);
				generatrice.declare_variable(it->type, nom_broye, "");
			}

			generatrice << " } " << nom_type_coro << ";\n";

			/* Déclare la fonction. */
			generatrice << "static void *" << nom_fonction << "(\nvoid *data)\n";
			generatrice << "{\n";
			generatrice << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
			generatrice << "ContexteProgramme contexte = __etat->contexte;\n";

			/* déclare les paramètres. */
			POUR (decl->params) {
				auto nom_broye = broye_nom_simple(it->ident->nom);
				generatrice.declare_variable(it->type, nom_broye, "__etat->" + nom_broye);
			}

			/* Crée code pour le bloc. */
			genere_code_C(decl->bloc, generatrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(decl->bloc, contexte, generatrice);
			}

			generatrice << "}\n";

			contexte.termine_fonction();

			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			cree_appel(b, contexte, generatrice);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto flux = dls::flux_chaine();

			/*if (b->aide_generation_code == GENERE_CODE_ACCES_VAR)*/ {
				if (b->nom_fonction_appel != "") {
					flux << b->nom_fonction_appel;
				}
				else {
					// À FAIRE(réusinage arbre)
//					if (dv.est_membre_emploie) {
//						flux << dv.structure;
//					}

					if ((b->drapeaux & EST_VAR_BOUCLE) != 0) {
						flux << "(*";
					}

					flux << broye_chaine(b);

					if ((b->drapeaux & EST_VAR_BOUCLE) != 0) {
						flux << ")";
					}
				}

				b->valeur_calculee = dls::chaine(flux.chn());
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto structure = inst->expr1;
			auto membre = inst->expr2;

			if (b->drapeaux & EST_APPEL_SYNTAXE_UNIFORME) {
				cree_appel(membre, contexte, generatrice);
				b->valeur_calculee = membre->valeur_calculee;
			}
			else {
				genere_code_acces_membre(contexte, generatrice, b, structure, membre, expr_gauche);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto structure = inst->expr1;
			auto membre = inst->expr2;

			auto index_membre = std::any_cast<long>(b->valeur_calculee);
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
				generatrice << expr_membre << " = " << index_membre + 1 << ";";
			}
			else {
				if (b->aide_generation_code != IGNORE_VERIFICATION) {
					auto const &lexeme = b->lexeme;
					auto module = contexte.fichier(static_cast<size_t>(lexeme->fichier));
					auto pos = trouve_position(*lexeme, module);

					generatrice << "if (" << expr_membre << " != " << index_membre + 1 << ") {\n";
					generatrice << "KR__acces_membre_union(";
					generatrice << '"' << module->chemin << '"' << ',';
					generatrice << pos.numero_ligne;
					generatrice << ");\n";
					generatrice << "}\n";
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
				dls::tableau<NoeudExpression *> feuilles;
				rassemble_feuilles(variable, feuilles);

				dls::liste<NoeudExpression *> noeuds;

				for (auto f : feuilles) {
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

			applique_transformation(expression, generatrice, contexte, expr_gauche);

			genere_code_C(variable, generatrice, contexte, true);

			generatrice << variable->chaine_calculee();
			generatrice << " = ";
			generatrice << expression->chaine_calculee();

			/* pour les globales */
			generatrice << ";\n";

			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(b);
			auto variable = expr->valeur;
			auto expression = expr->expression;

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
			flux << nom_broye_type(type, true) << ' ' << nom_broye;

			/* nous avons une déclaration, initialise à zéro */
			if (expression == nullptr) {
				generatrice << flux.chn() << ";\n";

				/* À FAIRE: initialisation pour les variables globales */
				if (contexte.donnees_fonction != nullptr) {
					if (variable->type->genre == GenreType::STRUCTURE || variable->type->genre == GenreType::UNION) {
						generatrice << "initialise_" << type->nom_broye << "(&" << nom_broye << ");\n";
					}
					else if (variable->type->genre == GenreType::CHAINE || variable->type->genre == GenreType::TABLEAU_DYNAMIQUE) {
						generatrice << nom_broye << ".pointeur = 0;\n";
						generatrice << nom_broye << ".taille = 0;\n";
					}
					else if (variable->type->genre == GenreType::EINI) {
						generatrice << nom_broye << ".pointeur = 0\n;";
						generatrice << nom_broye << ".info = 0\n;";
					}
					else if (variable->type->genre != GenreType::TABLEAU_FIXE) {
						generatrice << nom_broye << " = 0;\n";
					}
				}
			}
			else {
				applique_transformation(expression, generatrice, contexte, false);
				generatrice << flux.chn();
				generatrice << " = ";
				generatrice << expression->chaine_calculee();
				generatrice << ";\n";
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_reel(
												 b->lexeme->chaine,
												 b->lexeme->genre);

			b->valeur_calculee = dls::vers_chaine(valeur);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<long>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_entier(
												 b->lexeme->chaine,
												 b->lexeme->genre);

			b->valeur_calculee = dls::vers_chaine(valeur);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			/* À FAIRE : tests */
			auto flux = dls::flux_chaine();

			applique_transformation(enfant1, generatrice, contexte, expr_gauche);
			applique_transformation(enfant2, generatrice, contexte, expr_gauche);

			auto op = expr->op;

			if (op->est_basique) {
				flux << enfant1->chaine_calculee();
				flux << b->lexeme->chaine;
				flux << enfant2->chaine_calculee();
			}
			else {
				/* À FAIRE: gestion du contexte. */
				flux << op->nom_fonction << '(';
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
				generatrice << "const ";
				generatrice.declare_variable(b->type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = static_cast<NoeudExpressionBinaire *>(inst->expr1);
			auto enfant2 = inst->expr2;

			/* À FAIRE : tests */
			auto flux = dls::flux_chaine();

			genere_code_C(enfant1, generatrice, contexte, expr_gauche);
			genere_code_C(enfant2, generatrice, contexte, expr_gauche);

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
			generatrice << "const ";
			generatrice.declare_variable(b->type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
			break;
		}
		case GenreNoeud::EXPRESSION_INDICE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			auto type1 = enfant1->type;

			if (type1->genre == GenreType::REFERENCE) {
				type1 = contexte.typeuse.type_dereference_pour(type1);
			}

			/* À FAIRE : tests */
			auto flux = dls::flux_chaine();

			// force une expression gauche pour les types tableau fixe car une
			// expression droite pourrait créer une variable temporarire T x[N] = pointeur
			// ce qui est invalide en C.
			applique_transformation(enfant1, generatrice, contexte, expr_gauche || type1->genre == GenreType::TABLEAU_FIXE);
			genere_code_C(enfant2, generatrice, contexte, expr_gauche);

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

			auto const &lexeme = b->lexeme;
			auto module = contexte.fichier(static_cast<size_t>(lexeme->fichier));
			auto pos = trouve_position(*lexeme, module);

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
					generatrice << "if (";
					generatrice << enfant2->chaine_calculee();
					generatrice << " < 0 || ";
					generatrice << enfant2->chaine_calculee();
					generatrice << " >= ";
					generatrice << enfant1->chaine_calculee();
					generatrice << ".taille) {\n";
					generatrice << "KR__depassement_limites_chaine(";
					generatrice << '"' << module->chemin << '"' << ',';
					generatrice << pos.numero_ligne << ',';
					generatrice << enfant1->chaine_calculee();
					generatrice << ".taille,";
					generatrice << enfant2->chaine_calculee();
					generatrice << ");\n}\n";

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
						generatrice << "if (";
						generatrice << enfant2->chaine_calculee();
						generatrice << " < 0 || ";
						generatrice << enfant2->chaine_calculee();
						generatrice << " >= ";
						generatrice << enfant1->chaine_calculee();
						generatrice << ".taille";
						generatrice << ") {\n";
						generatrice << "KR__depassement_limites_tableau(";
						generatrice << '"' << module->chemin << '"' << ',';
						generatrice << pos.numero_ligne << ',';
						generatrice << enfant1->chaine_calculee();
						generatrice << ".taille";
						generatrice << ",";
						generatrice << enfant2->chaine_calculee();
						generatrice << ");\n}\n";
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
						generatrice << "if (";
						generatrice << enfant2->chaine_calculee();
						generatrice << " < 0 || ";
						generatrice << enfant2->chaine_calculee();
						generatrice << " >= ";
						generatrice << taille_tableau;

						generatrice << ") {\n";
						generatrice << "KR__depassement_limites_tableau(";
						generatrice << '"' << module->chemin << '"' << ',';
						generatrice << pos.numero_ligne << ',';
							generatrice << taille_tableau;
						generatrice << ",";
						generatrice << enfant2->chaine_calculee();
						generatrice << ");\n}\n";
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
				generatrice << "const ";
				generatrice.declare_variable(b->type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = expr->expr;

			/* À FAIRE : tests */

			/* force une expression si l'opérateur est @, pour que les
			 * expressions du type @a[0] retourne le pointeur à a + 0 et non le
			 * pointeur de la variable temporaire du code généré */
			expr_gauche |= b->lexeme->genre == GenreLexeme::AROBASE;
			applique_transformation(enfant, generatrice, contexte, expr_gauche);

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
					flux << expr->op->nom_fonction;
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
			genere_code_extra_pre_retour(b->bloc_parent, contexte, generatrice);
			generatrice << "return;\n";
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
			applique_transformation(enfant, generatrice, contexte, true);

			generatrice.declare_variable(
						b->type,
						nom_variable,
						enfant->chaine_calculee());

			/* NOTE : le code différé doit être créé après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(b->bloc_parent, contexte, generatrice);
			generatrice << "return " << nom_variable << ";\n";
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
				genere_code_C(enfant, generatrice, contexte, false);
			}
			else if (enfant->lexeme->genre == GenreLexeme::VIRGULE) {
				/* retourne a, b; -> *__ret1 = a; *__ret2 = b; return; */
				dls::tableau<NoeudExpression *> feuilles;
				rassemble_feuilles(enfant, feuilles);

				auto idx = 0l;
				for (auto f : feuilles) {
					applique_transformation(f, generatrice, contexte, false);

					generatrice << '*' << df->noms_retours[idx++] << " = ";
					generatrice << f->chaine_calculee();
					generatrice << ';';
				}
			}

			/* NOTE : le code différé doit être créé après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(b->bloc_parent, contexte, generatrice);
			generatrice << "return;\n";
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			/* Note : dû à la possibilité de différer le code, nous devons
			 * utiliser la chaine originale. */
			auto chaine = b->lexeme->chaine;

			auto nom_chaine = "__chaine_tmp" + dls::vers_chaine(b);

			generatrice << "chaine " << nom_chaine << " = {.pointeur=";
			generatrice << '"';

			for (auto c : chaine) {
				if (c == '\n') {
					generatrice << '\\' << 'n';
				}
				else if (c == '\t') {
					generatrice << '\\' << 't';
				}
				else {
					generatrice << c;
				}
			}

			generatrice << '"';
			generatrice << "};\n";
			/* on utilise strlen pour être sûr d'avoir la bonne taille à cause
			 * des caractères échappés */
			generatrice << nom_chaine << ".taille=strlen(" << nom_chaine << ".pointeur);\n";
			b->valeur_calculee = nom_chaine;
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
			auto c = b->lexeme->chaine[0];

			auto flux = dls::flux_chaine();

			flux << '\'';
			if (c == '\\') {
				flux << c << b->lexeme->chaine[1];
			}
			else {
				flux << c;
			}

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

				genere_code_C(enfant1, generatrice, contexte, false);
				genere_code_C(enfant2->expressions[0], generatrice, contexte, false);
				genere_code_C(enfant3->expressions[0], generatrice, contexte, false);

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

				generatrice.declare_variable(
							enfant2->expressions[0]->type,
							nom_variable,
							flux.chn());

				enfant1->valeur_calculee = nom_variable;

				return;
			}

			auto condition = inst->condition;

			genere_code_C(condition, generatrice, contexte, false);

			generatrice << "if (";

			if (b->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
				generatrice << '!';
			}

			generatrice << condition->chaine_calculee();
			generatrice << ")";

			genere_code_C(inst->bloc_si_vrai, generatrice, contexte, false);

			if (inst->bloc_si_faux) {
				generatrice << "else ";
				genere_code_C(inst->bloc_si_faux, generatrice, contexte, false);
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
				generatrice << "{\n";
			}

			POUR (inst->expressions) {
				genere_code_C(it, generatrice, contexte, true);

				dernier_enfant = it;

				if (est_instruction_retour(it->genre)) {
					break;
				}

				if (it->genre == GenreNoeud::OPERATEUR_BINAIRE) {
					/* les assignations opérées (+=, etc) n'ont pas leurs codes
					 * générées via genere_code_C  */
					if (est_assignation_operee(it->lexeme->genre)) {
						generatrice << it->chaine_calculee();
						generatrice << ";\n";
					}
				}
			}

			if (dernier_enfant != nullptr && !est_instruction_retour(dernier_enfant->genre)) {
				/* génère le code pour tous les noeuds différés de ce bloc */
				for (auto i = inst->noeuds_differes.taille - 1; i >= 0; --i) {
					auto bloc_differe = inst->noeuds_differes[i];
					bloc_differe->est_differe = false;
					genere_code_C(bloc_differe, generatrice, contexte, false);
					bloc_differe->est_differe = true;
				}
			}

			if (!inst->est_bloc_fonction) {
				generatrice << "}\n";
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

			auto genere_code_tableau_chaine = [&generatrice](
					GeneratriceCodeC &gen_loc,
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
				genere_code_C(enfant_2, generatrice, contexte_loc, true);

				gen_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= ";
				gen_loc << enfant_2->chaine_calculee();
				gen_loc << ".taille - 1; ++"<< nom_var <<") {\n";
				gen_loc << nom_broye_type(type_loc, true);
				gen_loc << " *" << broye_chaine(var) << " = &";
				gen_loc << enfant_2->chaine_calculee();
				gen_loc << ".pointeur["<< nom_var <<"];\n";

				if (idx) {
					gen_loc << "int " << broye_chaine(idx) << " = " << nom_var << ";\n";
				}
			};

			auto genere_code_tableau_fixe = [&generatrice](
					GeneratriceCodeC &gen_loc,
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

				gen_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= "
				   << taille_tableau << "-1; ++"<< nom_var <<") {\n";

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, generatrice, contexte_loc, true);
				gen_loc << nom_broye_type(type_loc, true);
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

					auto nom_broye = broye_chaine(var);

					if (idx != nullptr) {
						generatrice << "int " << broye_chaine(idx) << " = 0;\n";
					}

					genere_code_C(plage->expr1, generatrice, contexte, false);
					genere_code_C(plage->expr2, generatrice, contexte, false);

					generatrice << "for (";
					generatrice << nom_broye_type(type, true);
					generatrice << " " << nom_broye << " = ";
					generatrice << plage->expr1->chaine_calculee();

					generatrice << "; "
					   << nom_broye << " <= ";
					generatrice << plage->expr2->chaine_calculee();

					generatrice <<"; ++" << nom_broye;

					if (idx != nullptr) {
						generatrice << ", ++" << broye_chaine(idx);
					}

					generatrice  << ") {\n";

					break;
				}
				case GENERE_BOUCLE_TABLEAU:
				case GENERE_BOUCLE_TABLEAU_INDEX:
				{
					auto nom_var = "__i" + dls::vers_chaine(b);

					if (type->genre == GenreType::TABLEAU_FIXE) {
						auto type_tabl = static_cast<TypeTableauFixe *>(type);
						auto type_deref = type_tabl->type_pointe;
						genere_code_tableau_fixe(generatrice, contexte, enfant1, enfant2, type_deref, nom_var, type_tabl->taille);
					}
					else if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::VARIADIQUE) {
						auto type_deref = contexte.typeuse.type_dereference_pour(type);
						genere_code_tableau_chaine(generatrice, contexte, enfant1, enfant2, type_deref, nom_var);
					}
					else if (type->genre == GenreType::CHAINE) {
						type = contexte.typeuse[TypeBase::Z8];
						genere_code_tableau_chaine(generatrice, contexte, enfant1, enfant2, type, nom_var);
					}

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					auto expr_appel = static_cast<NoeudExpressionAppel *>(enfant2);
					auto decl_fonc = static_cast<NoeudDeclarationFonction *>(expr_appel->noeud_fonction_appelee);
					auto nom_etat = "__etat" + dls::vers_chaine(enfant2);
					auto nom_type_coro = "__etat_coro" + decl_fonc->nom_broye;

					generatrice << nom_type_coro << " " << nom_etat << " = {\n";
					generatrice << ".mutex_boucle = PTHREAD_MUTEX_INITIALIZER,\n";
					generatrice << ".mutex_coro = PTHREAD_MUTEX_INITIALIZER,\n";
					generatrice << ".cond_coro = PTHREAD_COND_INITIALIZER,\n";
					generatrice << ".cond_boucle = PTHREAD_COND_INITIALIZER,\n";
					generatrice << ".contexte = contexte,\n";
					generatrice << ".__termine_coro = 0\n";
					generatrice << "};\n";

					/* intialise les arguments de la fonction. */
					POUR (expr_appel->params) {
						genere_code_C(it, generatrice, contexte, false);
					}

					auto iter_enf = expr_appel->params.begin();

					POUR (decl_fonc->params) {
						auto nom_broye = broye_nom_simple(it->ident->nom);
						generatrice << nom_etat << '.' << nom_broye << " = ";
						generatrice << (*iter_enf)->chaine_calculee();
						generatrice << ";\n";
						++iter_enf;
					}

					generatrice << "pthread_t fil_coro;\n";
					generatrice << "pthread_create(&fil_coro, NULL, " << decl_fonc->nom_broye << ", &" << nom_etat << ");\n";
					generatrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";
					generatrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
					generatrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

					/* À FAIRE : utilisation du type */
					auto nombre_vars_ret = decl_fonc->type_fonc->types_sorties.taille;

					auto feuilles = dls::tableau<NoeudExpression *>{};
					rassemble_feuilles(enfant1, feuilles);

					auto idx = static_cast<NoeudExpression *>(nullptr);
					auto nom_idx = dls::chaine{};

					if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
						idx = feuilles.back();
						nom_idx = "__idx" + dls::vers_chaine(b);
						generatrice << "int " << nom_idx << " = 0;";
					}

					generatrice << "while (" << nom_etat << ".__termine_coro == 0) {\n";
					generatrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";

					for (auto i = 0l; i < nombre_vars_ret; ++i) {
						auto f = feuilles[i];
						auto nom_var_broye = broye_chaine(f);
						generatrice.declare_variable(type, nom_var_broye, "");
						generatrice << nom_var_broye << " = "
									   << nom_etat << '.' << decl_fonc->noms_retours[i]
									   << ";\n";
					}

					generatrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_coro);\n";
					generatrice << "pthread_cond_signal(&" << nom_etat << ".cond_coro);\n";
					generatrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_coro);\n";
					generatrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
					generatrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

					if (idx) {
						generatrice << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
						generatrice << nom_idx << " += 1;";
					}
				}
			}

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);
			auto goto_brise = "__boucle_pour_brise" + dls::vers_chaine(b);

			contexte.empile_goto_continue(enfant1->lexeme->chaine, goto_continue);
			contexte.empile_goto_arrete(enfant1->lexeme->chaine, (enfant_sinon != nullptr) ? goto_brise : goto_apres);

			genere_code_C(enfant3, generatrice, contexte, false);

			generatrice << goto_continue << ":;\n";
			generatrice << "}\n";

			if (enfant_sans_arret) {
				genere_code_C(enfant_sans_arret, generatrice, contexte, false);
				generatrice << "goto " << goto_apres << ";";
			}

			if (enfant_sinon) {
				generatrice << goto_brise << ":;\n";
				genere_code_C(enfant_sinon, generatrice, contexte, false);
			}

			generatrice << goto_apres << ":;\n";

			contexte.depile_goto_arrete();
			contexte.depile_goto_continue();

			if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE || b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
				generatrice << "pthread_join(fil_coro, NULL);\n";
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto chaine_var = inst->expr == nullptr ? dls::vue_chaine_compacte{""} : inst->expr->ident->nom;

			auto label_goto = (b->lexeme->genre == GenreLexeme::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			generatrice << "goto " << label_goto << ";\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			generatrice << "while (1) {\n";

			genere_code_C(inst->bloc, generatrice, contexte, false);

			generatrice << goto_continue << ":;\n";
			generatrice << "}\n";
			generatrice << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			generatrice << "while (1) {\n";
			genere_code_C(inst->bloc, generatrice, contexte, false);
			generatrice << goto_continue << ":;\n";
			genere_code_C(inst->condition, generatrice, contexte, false);
			generatrice << "if (!";
			generatrice << inst->condition->chaine_calculee();
			generatrice << ") {\nbreak;\n}\n";
			generatrice << "}\n";
			generatrice << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

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
			generatrice << "while (1) {";
			genere_code_C(inst->condition, generatrice, contexte, false);
			generatrice << "if (!";
			generatrice << inst->condition->chaine_calculee();
			generatrice << ") { break; }\n";

			genere_code_C(inst->bloc, generatrice, contexte, false);

			generatrice << goto_continue << ":;\n";
			generatrice << "}\n";
			generatrice << goto_apres << ":;\n";

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = inst->expr;
			auto const &type_de = enfant->type;

			genere_code_C(enfant, generatrice, contexte, false);

			if (type_de == b->type) {
				b->valeur_calculee = enfant->valeur_calculee;
				return;
			}

			auto flux = dls::flux_chaine();

			flux << "(";
			flux << nom_broye_type(b->type, true);
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
					applique_transformation(it, generatrice, contexte, false);
				}

				/* alloue un tableau fixe */
				auto type_tfixe = contexte.typeuse.type_tableau_fixe(b->type, taille_tableau);

				nom_tableau_fixe = dls::chaine("__tabl_fix")
						.append(dls::vers_chaine(reinterpret_cast<long>(b)));

				generatrice << nom_broye_type(type_tfixe, true) << ' ' << nom_tableau_fixe;
				generatrice << " = ";

				auto virgule = '{';

				POUR (noeud_tableau->exprs) {
					generatrice << virgule;
					generatrice << it->chaine_calculee();
					virgule = ',';
				}

				generatrice << "};\n";
			}

			/* alloue un tableau dynamique */
			auto type_tdyn = contexte.typeuse.type_tableau_dynamique(b->type);

			auto nom_tableau_dyn = dls::chaine("__tabl_dyn")
					.append(dls::vers_chaine(b));

			generatrice << nom_broye_type(type_tdyn, true) << ' ' << nom_tableau_dyn << ";\n";
			generatrice << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			generatrice << nom_tableau_dyn << ".taille = " << taille_tableau << ";\n";

			b->valeur_calculee = nom_tableau_dyn;
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(b);
			auto flux = dls::flux_chaine();

			if (b->lexeme->chaine == "PositionCodeSource") {
				genere_code_position_source(contexte, flux, b);
			}
			else {
				POUR (expr->params) {
					auto param = static_cast<NoeudExpressionBinaire *>(it);
					genere_code_C(param->expr2, generatrice, contexte, false);
				}

				auto virgule = '{';

				POUR (expr->params) {
					auto param = static_cast<NoeudExpressionBinaire *>(it);
					flux << virgule;

					flux << '.' << broye_nom_simple(param->expr1->ident->nom) << '=';
					flux << param->expr2->chaine_calculee();

					virgule = ',';
				}

				if (expr->params.taille == 0) {
					flux << '{';
				}

				flux << '}';
			}

			auto nom_temp = "__var_temp_struct" + dls::vers_chaine(index++);
			generatrice.declare_variable(b->type, nom_temp, flux.chn());

			b->valeur_calculee = nom_temp;

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto nom_tableau = "__tabl" + dls::vers_chaine(b);
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			b->genre_valeur = GenreValeur::DROITE;

			dls::tableau<NoeudExpression *> feuilles;
			rassemble_feuilles(expr->expr, feuilles);

			for (auto f : feuilles) {
				genere_code_C(f, generatrice, contexte, false);
			}

			generatrice << nom_broye_type(b->type, true) << ' ' << nom_tableau << " = ";

			auto virgule = '{';

			for (auto f : feuilles) {
				generatrice << virgule;
				generatrice << f->chaine_calculee();
				virgule = ',';
			}

			generatrice << "};\n";

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
			genere_code_C(expr->expr, generatrice, contexte, false);
			b->valeur_calculee = "*(" + expr->expr->chaine_calculee() + ")";
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			genere_code_allocation(contexte, generatrice, b->type, 0, b, expr->expr, expr->expr_chaine, expr->bloc);
			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			genere_code_allocation(contexte, generatrice, expr->expr->type, 2, expr->expr, expr->expr, expr->expr_chaine, expr->bloc);
			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			genere_code_allocation(contexte, generatrice, expr->expr->type, 1, b, expr->expr, expr->expr_chaine, expr->bloc);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			if (b->type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(b->type);

				if (type_union->deja_genere) {
					return;
				}

				genere_declaration_structure(generatrice, type_union->decl);
				cree_initialisation_structure(generatrice, type_union, type_union->decl);

				type_union->deja_genere = true;
			}
			else if (b->type->genre == GenreType::STRUCTURE) {
				auto type_struct = static_cast<TypeStructure *>(b->type);

				if (type_struct->deja_genere) {
					return;
				}

				genere_declaration_structure(generatrice, type_struct->decl);
				cree_initialisation_structure(generatrice, type_struct, type_struct->decl);

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

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			auto op = inst->op;

			POUR (inst->paires_discr) {
				auto enf0 = it.first;
				auto enf1 = it.second;

				auto feuilles = dls::tableau<NoeudExpression *>();
				rassemble_feuilles(enf0, feuilles);

				for (auto f : feuilles) {
					genere_code_C(f, generatrice, contexte, true);
				}

				auto prefixe = "if (";

				for (auto f : feuilles) {
					generatrice << prefixe;

					if (op->est_basique) {
						generatrice << chaine_expr;
						generatrice << " == ";
						generatrice << f->chaine_calculee();
					}
					else {
						generatrice << op->nom_fonction;
						generatrice << "(contexte,";
						generatrice << chaine_expr;
						generatrice << ',';
						generatrice << f->chaine_calculee();
						generatrice << ')';
					}

					prefixe = " || ";
				}

				generatrice << ") ";

				genere_code_C(enf1, generatrice, contexte, false);

				if (it != inst->paires_discr[inst->paires_discr.taille - 1]) {
					generatrice << "else {\n";
				}
			}

			if (inst->bloc_sinon) {
				generatrice << "else {\n";
				genere_code_C(inst->bloc_sinon, generatrice, contexte, false);
			}

			/* il faut fermer tous les blocs else */
			for (auto i = 0; i < inst->paires_discr.taille - 1; ++i) {
				generatrice << "}\n";
			}

			if (inst->bloc_sinon) {
				generatrice << "}\n";
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		{
			/* le premier enfant est l'expression, les suivants les paires */
			auto inst = static_cast<NoeudDiscr *>(b);
			auto expression = inst->expr;
			auto type_enum = static_cast<TypeEnum *>(expression->type);

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			generatrice << "switch (" << chaine_expr << ") {\n";

			POUR (inst->paires_discr) {
				auto enf0 = it.first;
				auto enf1 = it.second;

				auto feuilles = dls::tableau<NoeudExpression *>();
				rassemble_feuilles(enf0, feuilles);

				for (auto f : feuilles) {
					generatrice << "case " << chaine_valeur_enum(type_enum->decl, f->ident->nom) << ":\n";
				}

				generatrice << " {\n";
				genere_code_C(enf1, generatrice, contexte, false);
				generatrice << "break;\n}\n";
			}

			if (inst->bloc_sinon) {
				generatrice << "default:";
				generatrice << " {\n";
				genere_code_C(inst->bloc_sinon, generatrice, contexte, false);
				generatrice << "break;\n}\n";
			}

			generatrice << "}\n";

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

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_calculee = expression->chaine_calculee();
			chaine_calculee += (est_pointeur ? "->" : ".");

			generatrice << "switch (" << chaine_calculee << "membre_actif) {\n";

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

				generatrice << "case " << index_membre + 1;

				generatrice << ": {\n";
				genere_code_C(bloc_paire, generatrice, contexte, true);
				generatrice << "break;\n}\n";
			}

			if (inst->bloc_sinon) {
				generatrice << "default:";
				generatrice << " {\n";
				genere_code_C(inst->bloc_sinon, generatrice, contexte, false);
				generatrice << "break;\n}\n";
			}

			generatrice << "}\n";

			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto df = contexte.donnees_fonction;
			auto enfant = inst->expr;

			generatrice << "pthread_mutex_lock(&__etat->mutex_coro);\n";

			auto feuilles = dls::tableau<NoeudExpression *>{};
			rassemble_feuilles(enfant, feuilles);

			for (auto i = 0l; i < feuilles.taille(); ++i) {
				auto f = feuilles[i];

				genere_code_C(f, generatrice, contexte, true);

				generatrice << "__etat->" << df->noms_retours[i] << " = ";
				generatrice << f->chaine_calculee();
				generatrice << ";\n";
			}

			generatrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
			generatrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
			generatrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
			generatrice << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
			generatrice << "pthread_mutex_unlock(&__etat->mutex_coro);\n";

			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionParenthese *>(b);
			genere_code_C(expr->expr, generatrice, contexte, expr_gauche);
			b->valeur_calculee = '(' + expr->expr->chaine_calculee() + ')';
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto inst = static_cast<NoeudPousseContexte *>(b);
			auto variable = inst->expr;

			generatrice << "KsContexteProgramme ancien_contexte = contexte;\n";
			generatrice << "contexte = " << broye_chaine(variable) << ";\n";
			genere_code_C(inst->bloc, generatrice, contexte, true);
			generatrice << "contexte = ancien_contexte;\n";

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			applique_transformation(expr->expr, generatrice, contexte, expr_gauche);
			b->valeur_calculee = expr->expr->valeur_calculee;
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

		/* À FAIRE : dépendances cycliques :
		 * - types qui s'incluent indirectement (listes chainées intrusives)
		 * - fonctions recursives
		 */
		if (relation.noeud_fin->fut_visite) {
			continue;
		}

		traverse_graphe_pour_generation_code(contexte, generatrice, relation.noeud_fin, genere_fonctions, genere_info_type);
	}

	if (noeud->type == TypeNoeudDependance::TYPE) {
		if (noeud->noeud_syntactique != nullptr) {
			genere_code_C(noeud->noeud_syntactique, generatrice, contexte, false);
		}

//		if (noeud->type_ == nullptr || !genere_info_type) {
//			return;
//		}

//		/* Suppression des avertissements pour les conversions dites
//		 * « imcompatibles » alors qu'elles sont bonnes.
//		 * Elles surviennent dans les assignations des pointeurs, par exemple pour
//		 * ceux des tableaux des membres des fonctions.
//		 */
//		generatrice << "#pragma GCC diagnostic push\n";
//		generatrice << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

//		cree_info_type_C(contexte, generatrice, generatrice, noeud->type_);

//		generatrice << "#pragma GCC diagnostic pop\n";
	}
	else {
		if (noeud->type == TypeNoeudDependance::FONCTION && !genere_fonctions) {
			genere_declaration_fonction(noeud->noeud_syntactique, generatrice);
			generatrice << ";\n";
			return;
		}

		genere_code_C(noeud->noeud_syntactique, generatrice, contexte, false);
	}
}

static bool peut_etre_dereference(Type *type)
{
	return type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::REFERENCE || type->genre == GenreType::POINTEUR;
}

static void genere_typedefs_recursifs(
		ContexteGenerationCode &contexte,
		Type *type,
		dls::flux_chaine &os)
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
			genere_typedefs_recursifs(contexte, type_deref, os);
		}

		type_deref->drapeaux |= TYPEDEF_FUT_GENERE;
	}
	/* ajoute les types des paramètres et de retour des fonctions */
	else if (type->genre == GenreType::FONCTION) {
		auto type_fonc = static_cast<TypeFonction *>(type);

		POUR (type_fonc->types_entrees) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(contexte, it, os);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}

		POUR (type_fonc->types_sorties) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(contexte, it, os);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}
	}

	cree_typedef(type, os);
	type->drapeaux |= TYPEDEF_FUT_GENERE;
}

static void genere_typedefs_pour_tous_les_types(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	POUR (contexte.typeuse.types_simples) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_pointeurs) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_references) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_structures) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_enums) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_tableaux_fixes) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_tableaux_dynamiques) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_fonctions) genere_typedefs_recursifs(contexte, it, os);
	POUR (contexte.typeuse.types_unions) genere_typedefs_recursifs(contexte, it, os);
}

static void genere_infos_pour_tous_les_types(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice)
{
	/* Suppression des avertissements pour les conversions dites
	 * « imcompatibles » alors qu'elles sont bonnes.
	 * Elles surviennent dans les assignations des pointeurs, par exemple pour
	 * ceux des tableaux des membres des fonctions.
	 */
	generatrice << "#pragma GCC diagnostic push\n";
	generatrice << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

	POUR (contexte.typeuse.types_simples) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_pointeurs) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_references) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_structures) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_enums) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_tableaux_fixes) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_tableaux_dynamiques) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_fonctions) predeclare_info_type_C(generatrice, it);
	POUR (contexte.typeuse.types_unions) predeclare_info_type_C(generatrice, it);

	POUR (contexte.typeuse.types_simples) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_pointeurs) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_references) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_structures) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_enums) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_tableaux_fixes) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_tableaux_dynamiques) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_fonctions) cree_info_type_C(contexte, generatrice, it);
	POUR (contexte.typeuse.types_unions) cree_info_type_C(contexte, generatrice, it);

	generatrice << "#pragma GCC diagnostic pop\n";
}

// ----------------------------------------------

static void genere_code_debut_fichier(
		dls::flux_chaine &os,
		assembleuse_arbre const &arbre,
		dls::chaine const &racine_kuri)
{
	for (auto const &inc : arbre.inclusions) {
		os << "#include <" << inc << ">\n";
	}

	os << "\n";

	os << "#include <" << racine_kuri << "/fichiers/r16_c.h>\n";

	auto depassement_limites_tableau =
R"(
void KR__depassement_limites_tableau(
	const char *fichier,
	long ligne,
	long taille,
	long index)
{
	fprintf(stderr, "%s:%ld\n", fichier, ligne);
	fprintf(stderr, "Dépassement des limites du tableau !\n");
	fprintf(stderr, "La taille est de %ld mais l'index est de %ld !\n", taille, index);
	abort();
}
)";

	os << depassement_limites_tableau;

	auto depassement_limites_chaine =
R"(
void KR__depassement_limites_chaine(
	const char *fichier,
	long ligne,
	long taille,
	long index)
{
	fprintf(stderr, "%s:%ld\n", fichier, ligne);
	fprintf(stderr, "Dépassement des limites de la chaine !\n");
	fprintf(stderr, "La taille est de %ld mais l'index est de %ld !\n", taille, index);
	abort();
}
)";

	os << depassement_limites_chaine;

	auto hors_memoire =
R"(
void KR__hors_memoire(
	const char *fichier,
	long ligne)
{
	fprintf(stderr, "%s:%ld\n", fichier, ligne);
	fprintf(stderr, "Impossible d'allouer de la mémoire !\n");
	abort();
}
)";

	os << hors_memoire;

	/* À FAIRE : renseigner le membre actif */
	auto acces_membre_union =
R"(
void KR__acces_membre_union(
	const char *fichier,
	long ligne)
{
	fprintf(stderr, "%s:%ld\n", fichier, ligne);
	fprintf(stderr, "Impossible d'accèder au membre de l'union car il n'est pas actif !\n");
	abort();
}
)";

	os << acces_membre_union;

	/* déclaration des types de bases */
	os << "typedef struct chaine { char *pointeur; long taille; } chaine;\n";
	os << "typedef struct eini { void *pointeur; struct InfoType *info; } eini;\n";
	os << "#ifndef bool // bool est défini dans stdbool.h\n";
	os << "typedef unsigned char bool;\n";
	os << "#endif\n";
	os << "typedef unsigned char octet;\n";
	os << "typedef void Ksnul;\n";
	os << "typedef struct ContexteProgramme KsContexteProgramme;\n";
	/* À FAIRE : pas beau, mais un pointeur de fonction peut être un pointeur
	 * vers une fonction de LibC dont les arguments variadiques ne sont pas
	 * typés */
	os << "#define Kv ...\n\n";
	os << "#define TAILLE_STOCKAGE_TEMPORAIRE 16384\n";
	os << "static char DONNEES_STOCKAGE_TEMPORAIRE[TAILLE_STOCKAGE_TEMPORAIRE];\n\n";
	os << "static int __ARGC = 0;\n";
	os << "static const char **__ARGV = 0;\n\n";
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

	if (pour_meta_programme) {
		auto fonc_init = cherche_fonction_dans_module(contexte, "Kuri", "initialise_RC");
		auto noeud_init = graphe_dependance.cree_noeud_fonction(fonc_init->nom_broye, fonc_init);
		graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_init);
	}
}

static void genre_code_programme(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os,
		NoeudDependance *noeud_fonction_principale,
		dls::chrono::compte_seconde &debut_generation)
{
	auto generatrice = GeneratriceCodeC(contexte, os);
	auto &graphe_dependance = contexte.graphe_dependance;

	/* création primordiale des typedefs pour éviter les problèmes liés aux
	 * types récursifs, inclus tous les types car dans certains cas il manque
	 * des connexions entre types... */
	genere_typedefs_pour_tous_les_types(contexte, os);

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

		traverse_graphe_pour_generation_code(contexte, generatrice, noeud, false, false);
		/* restaure le drapeaux pour la génération des infos des types */
		noeud->fut_visite = false;
	}

//	for (auto nom_struct : noms_structs_infos_types) {
//		auto const &ds = contexte.donnees_structure(nom_struct);
//		auto noeud = graphe_dependance.cherche_noeud_type(ds.type);
//		traverse_graphe_pour_generation_code(contexte, generatrice, noeud, false, true);
//	}

	contexte.temps_generation += debut_generation.temps();

	//reduction_transitive(graphe_dependance);

	genere_infos_pour_tous_les_types(contexte, generatrice);

	debut_generation.commence();

	/* génère d'abord les déclarations des fonctions et les types */
	traverse_graphe_pour_generation_code(contexte, generatrice, noeud_fonction_principale, false, true);

	/* génère ensuite les fonctions */
	for (auto noeud_dep : graphe_dependance.noeuds) {
		if (noeud_dep->type == TypeNoeudDependance::FONCTION) {
			noeud_dep->fut_visite = false;
		}
	}

	traverse_graphe_pour_generation_code(contexte, generatrice, noeud_fonction_principale, true, false);
}

static void genere_code_creation_contexte(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &os)
{
	auto creation_contexte =
R"(
	KsStockageTemporaire stockage_temp;
	stockage_temp.donnxC3xA9es = DONNEES_STOCKAGE_TEMPORAIRE;
	stockage_temp.taille = TAILLE_STOCKAGE_TEMPORAIRE;
	stockage_temp.occupxC3xA9 = 0;
	stockage_temp.occupation_maximale = 0;

	KsContexteProgramme contexte;
	contexte.stockage_temporaire = &stockage_temp;

	KsBaseAllocatrice alloc_base;
	contexte.donnxC3xA9es_allocatrice = &alloc_base;

	contexte.donnxC3xA9es_logueur = 0;
)";

	auto fonc_alloc = cherche_fonction_dans_module(contexte, "Kuri", "allocatrice_défaut");
	auto fonc_init_alloc = cherche_fonction_dans_module(contexte, "Kuri", "initialise_base_allocatrice");
	auto fonc_log = cherche_fonction_dans_module(contexte, "Kuri", "__logueur_défaut");

	os << creation_contexte;
	os << "contexte.allocatrice = " << fonc_alloc->nom_broye << ";\n";
	os << fonc_init_alloc->nom_broye << "(contexte, &alloc_base);\n";
	os << "contexte.logueur = " << fonc_log->nom_broye << ";\n";
}

void genere_code_C(
		assembleuse_arbre const &arbre,
		ContexteGenerationCode &contexte,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	dls::flux_chaine os;

	auto temps_generation = 0.0;

	auto &graphe_dependance = contexte.graphe_dependance;
	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction("principale");

	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	genere_code_debut_fichier(os, arbre, racine_kuri);

	ajoute_dependances_implicites(contexte, noeud_fonction_principale, false);

	auto debut_generation = dls::chrono::compte_seconde();

	genre_code_programme(contexte, os, noeud_fonction_principale, debut_generation);

	os << "int main(int argc, char **argv)\n";
	os << "{\n";
	os << "    __ARGV = argv;\n";
	os << "    __ARGC = argc;\n";

	genere_code_creation_contexte(contexte, os);

	os << "    return principale(contexte);";
	os << "}\n";

	temps_generation += debut_generation.temps();

	contexte.temps_generation = temps_generation;

	fichier_sortie << os.chn();
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
	auto decl = static_cast<NoeudDeclarationFonction *>(expr->noeud_fonction_appelee);

	dls::flux_chaine os;

	auto generatrice = GeneratriceCodeC(contexte, os);

	auto temps_generation = 0.0;

	auto &graphe_dependance = contexte.graphe_dependance;

	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction(decl->nom_broye);
	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	genere_code_debut_fichier(os, arbre, racine_kuri);

	/* met en place la dépendance sur la fonction d'allocation par défaut */
	ajoute_dependances_implicites(contexte, noeud_fonction_principale, true);

	auto debut_generation = dls::chrono::compte_seconde();

	genre_code_programme(contexte, os, noeud_fonction_principale, debut_generation);

	os << "void lance_execution()\n";
	os << "{\n";

	genere_code_creation_contexte(contexte, os);

	genere_code_C(expr, generatrice, contexte, true);

	os << "    return;\n";
	os << "}\n";

	temps_generation += debut_generation.temps();

	contexte.temps_generation = temps_generation;

	fichier_sortie << os.chn();

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

}  /* namespace noeud */
