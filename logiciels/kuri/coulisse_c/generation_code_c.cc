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
	/* force une expression gauche afin de ne pas prendre la référence d'une
	 * variable temporaire */
	expr_gauche |= b->transformation.type == TypeTransformation::PREND_REFERENCE;
	expr_gauche |= b->transformation.type == TypeTransformation::CONSTRUIT_EINI;
	genere_code_C(b, generatrice, contexte, expr_gauche);

	auto nom_courant = b->chaine_calculee();
	auto type = b->type;

	auto &os = generatrice.os;

	auto nom_var_temp = "__var_temp_conv" + dls::vers_chaine(index++);

	switch (b->transformation.type) {
		case TypeTransformation::IMPOSSIBLE:
		{
			break;
		}
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

			os << "Kseini " << nom_var_temp << " = {\n";
			os << "\t.pointeur = &" << nom_courant << ",\n";
			os << "\t.info = (InfoType *)(&" << type->ptr_info_type << ")\n";
			os << "};\n";

			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			os << nom_broye_type(contexte, b->transformation.type_cible, true) << " " << nom_var_temp << " = *(";
			os << nom_broye_type(contexte, b->transformation.type_cible, true) << " *)(" << nom_courant << ".pointeur);\n";
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

			os << "KtKsoctet " << nom_var_temp << " = {\n";

			switch (type->genre) {
				default:
				{
					os << "\t.pointeur = (unsigned char *)(&" << nom_courant << "),\n";
					os << "\t.taille = sizeof(" << nom_broye_type(contexte, type, true) << ")\n";
					break;
				}
				case GenreType::POINTEUR:
				{
					os << "\t.pointeur = (unsigned char *)(" << nom_courant << "),\n";
					os << "\t.taille = sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					os << nom_broye_type(contexte, type_deref, true) << ")\n";
					break;
				}
				case GenreType::CHAINE:
				{
					os << "\t.pointeur = " << nom_courant << ".pointeur,\n";
					os << "\t.taille = " << nom_courant << ".taille,\n";
					break;
				}
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					os << "\t.pointeur = (unsigned char *)(&" << nom_courant << ".pointeur),\n";
					os << "\t.taille = " << nom_courant << ".taille * sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					os << nom_broye_type(contexte, type_deref, true) << ")\n";
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					os << "\t.pointeur = " << nom_courant << ",\n";
					os << "\t.taille = " << static_cast<TypeTableauFixe *>(type)->taille << " * sizeof(";
					auto type_deref = contexte.typeuse.type_dereference_pour(type);
					os << nom_broye_type(contexte, type_deref, true) << ")\n";
					break;
				}
			}

			os << "};\n";

			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);
			auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_deref);
			os << nom_broye_type(contexte, type_tabl, true) << ' ' << nom_var_temp << " = {\n";
			os << "\t.taille = " << static_cast<TypeTableauFixe *>(type)->taille << ",\n";
			os << "\t.pointeur = " << nom_courant << "\n";
			os << "};";

			break;
		}
		case TypeTransformation::FONCTION:
		{
			os << nom_broye_type(contexte, b->transformation.type_cible, true) << ' ';
			os << nom_var_temp << " = " << b->transformation.nom_fonction << '(';
			os << nom_courant << ");\n";

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
				os << nom_broye_type(contexte, type_deref, true);
				os << ' ' << nom_var_temp << " = *(" << nom_courant << ");\n";
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

	auto type = b->type;

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
	else if (type->genre != GenreType::RIEN && (b->aide_generation_code == APPEL_POINTEUR_FONCTION || ((b->df != nullptr) && !b->df->est_coroutine))) {
		auto nom_indirection = "__ret" + dls::vers_chaine(b);
		os << nom_broye_type(contexte, type, true) << ' ' << nom_indirection << " = ";
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
			os << "contexte";
			virgule = ',';
		}
	}
	else {
		if (!dls::outils::possede_drapeau(b->drapeaux, FORCE_NULCTX)) {
			os << virgule;
			os << "contexte";
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
		Type *type_parent,
		dls::chaine const &chaine_parent,
		dls::vue_chaine_compacte const &accesseur,
		dls::flux_chaine &os)
{
	if (type_parent->genre == GenreType::CHAINE || type_parent->genre == GenreType::TABLEAU_DYNAMIQUE) {
		os << chaine_parent << accesseur << "pointeur = 0;\n";
		os << chaine_parent << accesseur << "taille = 0;\n";
	}
	else if (type_parent->genre == GenreType::EINI) {
		os << chaine_parent << accesseur << "pointeur = 0\n;";
		os << chaine_parent << accesseur << "info = 0\n;";
	}
	else if (type_parent->genre == GenreType::ENUM) {
		os << chaine_parent << " = 0;\n";
	}
	else if (type_parent->genre == GenreType::UNION) {
		auto type_union = static_cast<TypeStructure *>(type_parent);
		auto const &ds = contexte.donnees_structure(type_union->nom);

		/* À FAIRE: initialise le type le plus large */
		if (!ds.est_nonsur) {
			os << chaine_parent << dls::chaine(accesseur) << "membre_actif = 0;\n";
		}
	}
	else if (type_parent->genre == GenreType::STRUCTURE) {
		auto type_struct = static_cast<TypeStructure *>(type_parent);
		auto const &ds = contexte.donnees_structure(type_struct->nom);

		for (auto i = 0l; i < ds.types.taille(); ++i) {
			auto type_enf = ds.types[i];

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
								type_enf,
								decl_nom,
								".",
								os);
				}
			}
		}
	}
	else if (type_parent->genre == GenreType::TABLEAU_FIXE) {
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
			generatrice.os << "long " << nom_acces << " = " << taille_tableau << ";\n";
			flux << nom_acces;
		}
		else if (type_structure->genre == GenreType::ENUM) {
			auto nom_struct = static_cast<TypeEnum *>(type_structure)->nom;
			auto &ds = contexte.donnees_structure(nom_struct);
			flux << chaine_valeur_enum(ds, membre->chaine());
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
				generatrice.os << flux.chn();
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
			flux << broye_chaine(membre);
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
			generatrice.os << "const ";
			generatrice.declare_variable(b->type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
		}
	}
}

static void genere_code_echec_logement(
		ContexteGenerationCode &contexte,
		GeneratriceCodeC &generatrice,
		dls::chaine const &nom_ptr,
		base *b,
		base *bloc)
{
	generatrice.os << "if (" << nom_ptr  << " == 0)";

	if (bloc) {
		genere_code_C(bloc, generatrice, contexte, true);
	}
	else {
		auto const &lexeme = b->lexeme;
		auto module = contexte.fichier(static_cast<size_t>(lexeme.fichier));
		auto pos = trouve_position(lexeme, module);

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
	auto module = contexte.fichier(static_cast<size_t>(b->lexeme.fichier))->module;
	auto &vdf = module->donnees_fonction(b->lexeme.chaine);

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
	donnees_var.type = argument.type;
	donnees_var.est_argument = true;

	contexte.pousse_locale(argument.nom, donnees_var);

	if (argument.est_employe) {
		auto type = argument.type;
		auto nom_structure = dls::vue_chaine_compacte("");
		auto est_pointeur = false;

		if (type->genre == GenreType::POINTEUR) {
			est_pointeur = true;

			type = contexte.typeuse.type_dereference_pour(type);
		}

		nom_structure = static_cast<TypeStructure *>(type)->nom;

		auto &ds = contexte.donnees_structure(nom_structure);

		/* pousse chaque membre de la structure sur la pile */

		for (auto &dm : ds.donnees_membres) {
			auto type_membre = ds.types[dm.second.index_membre];

			donnees_var.est_dynamique = argument.est_dynamic;
			donnees_var.type = type_membre;
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

	for (auto i = 0l; i < donnees.types.taille(); ++i) {
		auto type_membre = donnees.types[i];

		for (auto paire_idx_mb : donnees.donnees_membres) {
			if (paire_idx_mb.second.index_membre == i) {
				auto nom = broye_nom_simple(paire_idx_mb.first);
				os << nom_broye_type(contexte, type_membre, true) << ' ' << nom << ";\n";
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

static void genere_code_position_source(
		ContexteGenerationCode &contexte,
		dls::flux_chaine &flux,
		noeud::base *b)
{
	/* À FAIRE: pour les appels de fonction où l'objet est construit
	 * via une valeur d'argument par défaut, les informations seront
	 * toujours celles de la déclaration de l'argument. */
	auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme.fichier));

	auto fonction_courante = contexte.donnees_fonction;
	auto nom_fonction = dls::vue_chaine_compacte("");

	if (fonction_courante != nullptr) {
		nom_fonction = fonction_courante->noeud_decl->lexeme.chaine;
	}

	auto pos = trouve_position(b->lexeme, fichier);

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
		base *b,
		base *variable,
		base *expression,
		base *bloc_sinon)
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
			expr_ancienne_taille_octet += " * sizeof(" + nom_broye_type(contexte, type_deref, true) + ")";

			/* allocation ou réallocation */
			if (!b->type_declare.expressions.est_vide()) {
				expression = b->type_declare.expressions[0];
				genere_code_C(expression, generatrice, contexte, false);
				expr_nouvelle_taille = expression->chaine_calculee();
				expr_nouvelle_taille_octet = expr_nouvelle_taille;
				expr_nouvelle_taille_octet += " * sizeof(" + nom_broye_type(contexte, type_deref, true) + ")";
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
			expr_ancienne_taille_octet = "sizeof(" + nom_broye_type(contexte, type_deref, true) + ")";

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

	generatrice.os << "long " << nom_nouvelle_taille << " = " << expr_nouvelle_taille_octet << ";\n";
	generatrice.os << "long " << nom_ancienne_taille << " = " << expr_ancienne_taille_octet << ";\n";
	generatrice.os << "InfoType *" << nom_info_type << " = (InfoType *)(&" << type->ptr_info_type << ");\n";
	generatrice.os << "void *" << nom_ancien_pointeur << " = " << expr_pointeur << ";\n";

	auto nom_pos = "__var_temp_pos" + dls::vers_chaine(index++);
	generatrice.os << "PositionCodeSource " << nom_pos << " = ";
	genere_code_position_source(contexte, generatrice.os, b);
	generatrice.os << "\n;";

	generatrice.os << "void *" << nom_nouveu_pointeur << " = ";
	generatrice.os << "contexte.allocatrice(contexte, ";
	generatrice.os << mode << ',';
	generatrice.os << nom_nouvelle_taille << ',';
	generatrice.os << nom_ancienne_taille << ',';
	generatrice.os << nom_ancien_pointeur << ',';
	generatrice.os << broye_nom_simple("contexte.données_allocatrice") << ',';
	generatrice.os << nom_info_type << ',';
	generatrice.os << nom_pos << ");\n";
	generatrice.os << expr_pointeur << " = " << nom_nouveu_pointeur << ";\n";

	switch (mode) {
		case 0:
		{
			genere_code_echec_logement(
						contexte,
						generatrice,
						expr_pointeur,
						b,
						bloc_sinon);

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
		generatrice.os << expr_acces_taille << " = " << expr_nouvelle_taille <<  ";\n";
	}
}

static void genere_declaration_fonction(
		base *b,
		GeneratriceCodeC &generatrice,
		ContexteGenerationCode &contexte)
{
	using dls::outils::possede_drapeau;

	auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

	if (est_externe) {
		return;
	}

	auto donnees_fonction = cherche_donnees_fonction(contexte, b);

	/* Code pour le type et le nom */

	auto nom_fonction = donnees_fonction->nom_broye;
	donnees_fonction->est_sans_contexte = possede_drapeau(b->drapeaux, FORCE_NULCTX);

	auto moult_retour = donnees_fonction->type->types_sorties.taille > 1;

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

		auto type_fonc = static_cast<TypeFonction *>(b->type);
		generatrice.os << nom_broye_type(contexte, type_fonc->types_sorties[0], true) << ' ' << nom_fonction;
	}

	/* Code pour les arguments */

	auto virgule = '(';

	if (donnees_fonction->args.taille() == 0 && !moult_retour) {
		generatrice.os << '(' << '\n';
		virgule = ' ';
	}

	if (!donnees_fonction->est_externe && !donnees_fonction->est_sans_contexte) {
		generatrice.os << virgule << '\n';
		generatrice.os << "KsContexteProgramme contexte";
		virgule = ',';
	}

	for (auto &argument : donnees_fonction->args) {
		auto nom_broye = broye_nom_simple(argument.nom);

		generatrice.os << virgule << '\n';
		generatrice.os << nom_broye_type(contexte, argument.type, true) << ' ' << nom_broye;

		virgule = ',';
	}

	if (moult_retour) {
		auto idx_ret = 0l;
		POUR (donnees_fonction->type->types_sorties) {
			generatrice.os << virgule << '\n';

			auto nom_ret = "*" + donnees_fonction->noms_retours[idx_ret++];

			generatrice.os << nom_broye_type(contexte, it, true) << ' ' << nom_ret;
			virgule = ',';
		}
	}

	generatrice.os << ")";
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
	switch (b->genre) {
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::RACINE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		{
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

			genere_declaration_fonction(b, generatrice, contexte);

			generatrice.os << '\n';

			auto donnees_fonction = cherche_donnees_fonction(contexte, b);
			contexte.commence_fonction(donnees_fonction);

			/* pousse les arguments sur la pile */
			if (!donnees_fonction->est_externe && !donnees_fonction->est_sans_contexte) {
				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = true;
				donnees_var.est_variadic = false;
				donnees_var.type = contexte.type_contexte;
				donnees_var.est_argument = true;

				contexte.pousse_locale("contexte", donnees_var);
			}

			for (auto &argument : donnees_fonction->args) {
				auto nom_broye = broye_nom_simple(argument.nom);
				pousse_argument_fonction_pile(contexte, argument, nom_broye);
			}

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.back();

			genere_code_C(bloc, generatrice, contexte, false);

			if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			}

			contexte.termine_fonction();

			break;
		}
		case GenreNoeud::DECLARATION_COROUTINE:
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
			generatrice.os << "ContexteProgramme contexte;\n";

			auto idx_ret = 0l;
			POUR (donnees_fonction->type->types_sorties) {
				auto &nom_ret = donnees_fonction->noms_retours[idx_ret++];
				generatrice.declare_variable(it, nom_ret, "");
			}

			for (auto &argument : donnees_fonction->args) {
				auto nom_broye = broye_nom_simple(argument.nom);
				generatrice.declare_variable(argument.type, nom_broye, "");
			}

			generatrice.os << " } " << nom_type_coro << ";\n";

			/* Déclare la fonction. */
			generatrice.os << "static void *" << nom_fonction << "(\nvoid *data)\n";
			generatrice.os << "{\n";
			generatrice.os << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
			generatrice.os << "ContexteProgramme contexte = __etat->contexte;\n";

			/* déclare les paramètres. */
			for (auto &argument : donnees_fonction->args) {
				auto nom_broye = broye_nom_simple(argument.nom);

				generatrice.declare_variable(argument.type, nom_broye, "__etat->" + nom_broye);

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
		case GenreNoeud::DECLARATION_PARAMETRES_FONCTION:
		{
			/* géré dans DECLARATION_FONCTION */
			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			cree_appel(b, generatrice.os, contexte, generatrice, b->nom_fonction_appel, b->enfants);
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
					auto dv = contexte.donnees_variable(b->lexeme.chaine);

					if (dv.est_membre_emploie) {
						flux << dv.structure;
					}

					if (dv.est_var_boucle) {
						flux << "(*";
					}

					flux << broye_chaine(b);

					if (dv.est_var_boucle) {
						flux << ")";
					}
				}

				b->valeur_calculee = dls::chaine(flux.chn());
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto structure = b->enfants.front();
			auto membre = b->enfants.back();
			genere_code_acces_membre(contexte, generatrice, b, structure, membre, expr_gauche);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto structure = b->enfants.front();
			auto membre = b->enfants.back();

			auto index_membre = std::any_cast<long>(b->valeur_calculee);
			auto type = structure->type;

			auto est_pointeur = (type->genre == GenreType::POINTEUR);
			est_pointeur |= (type->genre == GenreType::REFERENCE);

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
					auto const &lexeme = b->lexeme;
					auto module = contexte.fichier(static_cast<size_t>(lexeme.fichier));
					auto pos = trouve_position(lexeme, module);

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
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.taille() == 2);

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			/* a, b = foo(); -> foo(&a, &b); */
			if (variable->identifiant() == GenreLexeme::VIRGULE) {
				dls::tableau<base *> feuilles;
				rassemble_feuilles(variable, feuilles);

				dls::liste<base *> noeuds;

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

			generatrice.os << variable->chaine_calculee();
			generatrice.os << " = ";
			generatrice.os << expression->chaine_calculee();

			/* pour les globales */
			generatrice.os << ";\n";

			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto variable = static_cast<noeud::base *>(nullptr);
			auto expression = static_cast<noeud::base *>(nullptr);

			if (b->enfants.taille() == 2) {
				variable = b->enfants.front();
				expression = b->enfants.back();
			}
			else {
				variable = b;
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

			auto nom_broye = broye_chaine(variable);
			flux << nom_broye_type(contexte, type, true) << ' ' << nom_broye;

			/* nous avons une déclaration, initialise à zéro */
			if (expression == nullptr) {
				generatrice.os << flux.chn() << ";\n";

				/* À FAIRE: initialisation pour les variables globales */
				if (contexte.donnees_fonction != nullptr) {
					cree_initialisation(
								contexte,
								generatrice,
								type,
								nom_broye,
								".",
								generatrice.os);
				}
			}
			else {
				applique_transformation(expression, generatrice, contexte, false);
				generatrice.os << flux.chn();
				generatrice.os << " = ";
				generatrice.os << expression->chaine_calculee();
				generatrice.os << ";\n";
			}

			auto donnees_var = DonneesVariable{};
			donnees_var.est_externe = (variable->drapeaux & EST_EXTERNE) != 0;
			donnees_var.est_dynamique = (variable->drapeaux & DYNAMIC) != 0;
			donnees_var.type = variable->type;

			if (contexte.donnees_fonction == nullptr) {
				contexte.pousse_globale(variable->chaine(), donnees_var);
			}
			else {
				contexte.pousse_locale(variable->chaine(), donnees_var);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_reel(
												 b->lexeme.chaine,
												 b->lexeme.genre);

			b->valeur_calculee = dls::vers_chaine(valeur);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<long>(b->valeur_calculee) :
											 denombreuse::converti_chaine_nombre_entier(
												 b->lexeme.chaine,
												 b->lexeme.genre);

			b->valeur_calculee = dls::vers_chaine(valeur);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
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
				flux << b->lexeme.chaine;
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
				generatrice.os << "const ";
				generatrice.declare_variable(b->type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

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
			flux << enfant1->enfants.back()->chaine_calculee();
			flux << b->lexeme.chaine;
			flux << enfant2->chaine_calculee();
			flux << ')';

			auto nom_var = "__var_temp_cmp" + dls::vers_chaine(index++);
			generatrice.os << "const ";
			generatrice.declare_variable(b->type, nom_var, flux.chn());

			b->valeur_calculee = nom_var;
			break;
		}
		case GenreNoeud::EXPRESSION_INDICE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

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
			auto module = contexte.fichier(static_cast<size_t>(lexeme.fichier));
			auto pos = trouve_position(lexeme, module);

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
					generatrice.os << "if (";
					generatrice.os << enfant2->chaine_calculee();
					generatrice.os << " < 0 || ";
					generatrice.os << enfant2->chaine_calculee();
					generatrice.os << " >= ";
					generatrice.os << enfant1->chaine_calculee();
					generatrice.os << ".taille) {\n";
					generatrice.os << "KR__depassement_limites_chaine(";
					generatrice.os << '"' << module->chemin << '"' << ',';
					generatrice.os << pos.numero_ligne << ',';
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
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					if (b->aide_generation_code != IGNORE_VERIFICATION) {
						generatrice.os << "if (";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << " < 0 || ";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << " >= ";
						generatrice.os << enfant1->chaine_calculee();
						generatrice.os << ".taille";
						generatrice.os << ") {\n";
						generatrice.os << "KR__depassement_limites_tableau(";
						generatrice.os << '"' << module->chemin << '"' << ',';
						generatrice.os << pos.numero_ligne << ',';
						generatrice.os << enfant1->chaine_calculee();
						generatrice.os << ".taille";
						generatrice.os << ",";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << ");\n}\n";
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
						generatrice.os << "if (";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << " < 0 || ";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << " >= ";
						generatrice.os << taille_tableau;

						generatrice.os << ") {\n";
						generatrice.os << "KR__depassement_limites_tableau(";
						generatrice.os << '"' << module->chemin << '"' << ',';
						generatrice.os << pos.numero_ligne << ',';
							generatrice.os << taille_tableau;
						generatrice.os << ",";
						generatrice.os << enfant2->chaine_calculee();
						generatrice.os << ");\n}\n";
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
				generatrice.os << "const ";
				generatrice.declare_variable(b->type, nom_var, flux.chn());

				b->valeur_calculee = nom_var;
			}

			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto enfant = b->enfants.front();

			/* À FAIRE : tests */

			/* force une expression si l'opérateur est @, pour que les
			 * expressions du type @a[0] retourne le pointeur à a + 0 et non le
			 * pointeur de la variable temporaire du code généré */
			expr_gauche |= b->lexeme.genre == GenreLexeme::AROBASE;
			applique_transformation(enfant, generatrice, contexte, expr_gauche);

			if (b->lexeme.genre == GenreLexeme::AROBASE) {
				b->valeur_calculee = "&(" + enfant->chaine_calculee() + ")";
			}
			else {
				auto flux = dls::flux_chaine();
				auto op = b->op;

				if (op->est_basique) {
					flux << b->lexeme.chaine;
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
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			generatrice.os << "return;\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		{
			auto enfant = b->enfants.front();
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
			genere_code_extra_pre_retour(contexte, generatrice, generatrice.os);
			generatrice.os << "return " << nom_variable << ";\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		{
			auto enfant = b->enfants.front();
			auto df = contexte.donnees_fonction;

			if (enfant->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
				/* retourne foo() -> foo(__ret...); return; */
				enfant->aide_generation_code = APPEL_FONCTION_MOULT_RET2;
				enfant->valeur_calculee = df->noms_retours;
				genere_code_C(enfant, generatrice, contexte, false);
			}
			else if (enfant->identifiant() == GenreLexeme::VIRGULE) {
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
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			/* Note : dû à la possibilité de différer le code, nous devons
			 * utiliser la chaine originale. */
			auto chaine = b->lexeme.chaine;

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
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto const est_calcule = dls::outils::possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
										   : (b->chaine() == "vrai");
			b->valeur_calculee = valeur ? dls::chaine("1") : dls::chaine("0");
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			auto c = b->lexeme.chaine[0];

			auto flux = dls::flux_chaine();

			flux << '\'';
			if (c == '\\') {
				flux << c << b->lexeme.chaine[1];
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

				if (b->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
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
							enfant2->enfants.front()->type,
							nom_variable,
							flux.chn());

				enfant1->valeur_calculee = nom_variable;

				return;
			}

			/* noeud 1 : condition */
			auto enfant1 = *iter_enfant++;

			genere_code_C(enfant1, generatrice, contexte, false);

			generatrice.os << "if (";

			if (b->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
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
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			contexte.empile_nombre_locales();

			auto dernier_enfant = static_cast<base *>(nullptr);

			generatrice.os << "{\n";

			for (auto enfant : b->enfants) {
				genere_code_C(enfant, generatrice, contexte, true);

				dernier_enfant = enfant;

				if (est_instruction_retour(enfant->genre)) {
					break;
				}

				if (enfant->genre == GenreNoeud::OPERATEUR_BINAIRE) {
					/* les assignations opérées (+=, etc) n'ont pas leurs codes
					 * générées via genere_code_C  */
					if (est_assignation_operee(enfant->identifiant())) {
						generatrice.os << enfant->chaine_calculee();
						generatrice.os << ";\n";
					}
				}
			}

			if (dernier_enfant != nullptr && !est_instruction_retour(dernier_enfant->genre)) {
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
		case GenreNoeud::INSTRUCTION_POUR:
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

			auto type = enfant2->type;
			enfant1->type = type;

			contexte.empile_nombre_locales();

			auto genere_code_tableau_chaine = [&generatrice](
					dls::flux_chaine &os_loc,
					ContexteGenerationCode &contexte_loc,
					base *enfant_1,
					base *enfant_2,
					Type *type_loc,
					dls::chaine const &nom_var)
			{
				auto var = enfant_1;
				auto idx = static_cast<noeud::base *>(nullptr);

				if (enfant_1->lexeme.genre == GenreLexeme::VIRGULE) {
					var = enfant_1->enfants.front();
					idx = enfant_1->enfants.back();
				}

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, generatrice, contexte_loc, true);

				os_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= ";
				os_loc << enfant_2->chaine_calculee();
				os_loc << ".taille - 1; ++"<< nom_var <<") {\n";
				os_loc << nom_broye_type(contexte_loc, type_loc, true);
				os_loc << " *" << broye_chaine(var) << " = &";
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
					Type *type_loc,
					dls::chaine const &nom_var,
					long taille_tableau)
			{
				auto var = enfant_1;
				auto idx = static_cast<noeud::base *>(nullptr);

				if (enfant_1->lexeme.genre == GenreLexeme::VIRGULE) {
					var = enfant_1->enfants.front();
					idx = enfant_1->enfants.back();
				}

				os_loc << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= "
				   << taille_tableau << "-1; ++"<< nom_var <<") {\n";

				/* utilise une expression gauche, car dans les coroutine, les
				 * variables temporaires ne sont pas sauvegarées */
				genere_code_C(enfant_2, generatrice, contexte_loc, true);
				os_loc << nom_broye_type(contexte_loc, type_loc, true);
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

					if (enfant1->lexeme.genre == GenreLexeme::VIRGULE) {
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
					generatrice.os << nom_broye_type(contexte, type, true);
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

						donnees_var.type = var->type;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.type = idx->type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						auto donnees_var = DonneesVariable{};
						donnees_var.type = type;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					break;
				}
				case GENERE_BOUCLE_TABLEAU:
				case GENERE_BOUCLE_TABLEAU_INDEX:
				{
					auto nom_var = "__i" + dls::vers_chaine(b);

					auto donnees_var = DonneesVariable{};

					if (type->genre == GenreType::TABLEAU_FIXE) {
						auto type_tabl = static_cast<TypeTableauFixe *>(type);
						auto type_deref = type_tabl->type_pointe;
						genere_code_tableau_fixe(generatrice.os, contexte, enfant1, enfant2, type_deref, nom_var, type_tabl->taille);
					}
					else if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::VARIADIQUE) {
						auto type_deref = contexte.typeuse.type_dereference_pour(type);
						genere_code_tableau_chaine(generatrice.os, contexte, enfant1, enfant2, type_deref, nom_var);
					}
					else if (type->genre == GenreType::CHAINE) {
						type = contexte.typeuse[TypeBase::Z8];
						genere_code_tableau_chaine(generatrice.os, contexte, enfant1, enfant2, type, nom_var);
					}

					if (b->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
						auto var = enfant1->enfants.front();
						auto idx = enfant1->enfants.back();

						donnees_var.type = type;
						donnees_var.est_var_boucle = true;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.type = idx->type;
						donnees_var.est_var_boucle = false;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						donnees_var.type = type;
						donnees_var.est_var_boucle = true;
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
					generatrice.os << ".contexte = contexte,\n";
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
					auto nombre_vars_ret = df->type->types_sorties.taille;

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
						generatrice.declare_variable(type, nom_var_broye, "");
						generatrice.os << nom_var_broye << " = "
									   << nom_etat << '.' << df->noms_retours[i]
									   << ";\n";

						auto donnees_var = DonneesVariable{};
						donnees_var.type = df->type->types_sorties[i];
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
						donnees_var.type = idx->type;
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
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto chaine_var = b->enfants.est_vide() ? dls::vue_chaine_compacte{""} : b->enfants.front()->chaine();

			auto label_goto = (b->lexeme.genre == GenreLexeme::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			generatrice.os << "goto " << label_goto << ";\n";
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
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
		case GenreNoeud::INSTRUCTION_REPETE:
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
		case GenreNoeud::INSTRUCTION_TANTQUE:
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
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			auto enfant = b->enfants.front();
			auto const &type_de = enfant->type;

			genere_code_C(enfant, generatrice, contexte, false);

			if (type_de == b->type) {
				b->valeur_calculee = enfant->valeur_calculee;
				return;
			}

			auto flux = dls::flux_chaine();

			flux << "(";
			flux << nom_broye_type(contexte, b->type, true);
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
		case GenreNoeud::INSTRUCTION_DIFFERE:
		{
			auto noeud = b->enfants.front();
			contexte.differe_noeud(noeud);
			break;
		}
		case GenreNoeud::INSTRUCTION_NONSUR:
		{
			contexte.non_sur(true);
			genere_code_C(b->enfants.front(), generatrice, contexte, false);
			contexte.non_sur(false);
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			/* utilisé principalement pour convertir les listes d'arguments
			 * variadics en un tableau */

			auto taille_tableau = b->enfants.taille();
			auto nom_tableau_fixe = dls::chaine("0");

			// si la liste d'arguments est vide, nous passons un tableau vide,
			// sinon nous créons d'abord un tableau fixe, qui sera converti en
			// un tableau dynamique
			if (taille_tableau != 0) {
				for (auto enfant : b->enfants) {
					applique_transformation(enfant, generatrice, contexte, false);
				}

				/* alloue un tableau fixe */
				auto type_tfixe = contexte.typeuse.type_tableau_fixe(b->type, taille_tableau);

				nom_tableau_fixe = dls::chaine("__tabl_fix")
						.append(dls::vers_chaine(reinterpret_cast<long>(b)));

				generatrice.os << nom_broye_type(contexte, type_tfixe, true) << ' ' << nom_tableau_fixe;
				generatrice.os << " = ";

				auto virgule = '{';

				for (auto enfant : b->enfants) {
					generatrice.os << virgule;
					generatrice.os << enfant->chaine_calculee();
					virgule = ',';
				}

				generatrice.os << "};\n";
			}

			/* alloue un tableau dynamique */
			auto type_tdyn = contexte.typeuse.type_tableau_dynamique(b->type);

			auto nom_tableau_dyn = dls::chaine("__tabl_dyn")
					.append(dls::vers_chaine(b));

			generatrice.os << nom_broye_type(contexte, type_tdyn, true) << ' ' << nom_tableau_dyn << ";\n";
			generatrice.os << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			generatrice.os << nom_tableau_dyn << ".taille = " << taille_tableau << ";\n";

			b->valeur_calculee = nom_tableau_dyn;
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto flux = dls::flux_chaine();

			if (b->chaine() == "PositionCodeSource") {
				genere_code_position_source(contexte, flux, b);
			}
			else {
				auto liste_params = std::any_cast<dls::tableau<dls::vue_chaine_compacte>>(&b->valeur_calculee);

				for (auto enfant : b->enfants) {
					genere_code_C(enfant, generatrice, contexte, false);
				}

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
			}

			auto nom_temp = "__var_temp_struct" + dls::vers_chaine(index++);
			generatrice.declare_variable(b->type, nom_temp, flux.chn());

			b->valeur_calculee = nom_temp;

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto nom_tableau = "__tabl" + dls::vers_chaine(b);

			dls::tableau<base *> feuilles;
			rassemble_feuilles(b->enfants.front(), feuilles);

			for (auto f : feuilles) {
				genere_code_C(f, generatrice, contexte, false);
			}

			generatrice.os << nom_broye_type(contexte, b->type, true) << ' ' << nom_tableau << " = ";

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
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto enfant = b->enfants.front();
			b->valeur_calculee = "&" + enfant->type->ptr_info_type;
			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			genere_code_C(b->enfants.front(), generatrice, contexte, false);
			b->valeur_calculee = "*(" + b->enfants.front()->chaine_calculee() + ")";
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto iter_enfant = b->enfants.debut();
			auto nombre_enfant = b->enfants.taille();
			auto expression = static_cast<base *>(nullptr);
			auto bloc_sinon = static_cast<base *>(nullptr);

			if (nombre_enfant == 1) {
				auto enfant = *iter_enfant++;

				if (enfant->genre == GenreNoeud::INSTRUCTION_COMPOSEE) {
					bloc_sinon = enfant;
				}
				else {
					expression = enfant;
				}
			}
			else if (nombre_enfant == 2) {
				expression = *iter_enfant++;
				bloc_sinon = *iter_enfant++;
			}

			genere_code_allocation(contexte, generatrice, b->type, 0, b, nullptr, expression, bloc_sinon);
			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto enfant = b->enfants.front();
			genere_code_allocation(contexte, generatrice, enfant->type, 2, b, enfant, nullptr, nullptr);
			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto iter_enfant = b->enfants.debut();
			auto nombre_enfant = b->enfants.taille();
			auto variable = static_cast<base *>(nullptr);
			auto expression = static_cast<base *>(nullptr);
			auto bloc_sinon = static_cast<base *>(nullptr);

			if (nombre_enfant == 1) {
				variable = *iter_enfant;
			}
			else if (nombre_enfant == 2) {
				variable = *iter_enfant++;

				auto enfant = *iter_enfant++;

				if (enfant->genre == GenreNoeud::INSTRUCTION_COMPOSEE) {
					bloc_sinon = enfant;
				}
				else {
					expression = enfant;
				}
			}
			else if (nombre_enfant == 3) {
				variable = *iter_enfant++;
				expression = *iter_enfant++;
				bloc_sinon = *iter_enfant++;
			}

			genere_code_allocation(contexte, generatrice, b->type, 1, b, variable, expression, bloc_sinon);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			genere_declaration_structure(contexte, generatrice.os, b->chaine());
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

			auto nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			auto op = b->op;

			for (auto i = 1l; i < nombre_enfants; ++i) {
				auto paire = *iter_enfant++;
				auto enf0 = paire->enfants.front();
				auto enf1 = paire->enfants.back();

				if (enf0->genre != GenreNoeud::INSTRUCTION_SINON) {
					auto feuilles = dls::tableau<base *>();
					rassemble_feuilles(enf0, feuilles);

					for (auto f : feuilles) {
						genere_code_C(f, generatrice, contexte, true);
					}

					auto prefixe = "if (";

					for (auto f : feuilles) {
						generatrice.os << prefixe;

						if (op->est_basique) {
							generatrice.os << chaine_expr;
							generatrice.os << " == ";
							generatrice.os << f->chaine_calculee();
						}
						else {
							generatrice.os << op->nom_fonction;
							generatrice.os << "(contexte,";
							generatrice.os << chaine_expr;
							generatrice.os << ',';
							generatrice.os << f->chaine_calculee();
							generatrice.os << ')';
						}

						prefixe = " || ";
					}

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
		case GenreNoeud::INSTRUCTION_PAIRE_DISCR:
		{
			/* RAF : pris en charge dans type_noeud::DISCR, ce noeud n'est que
			 * pour ajouter un niveau d'indirection et faciliter la compilation
			 * des discriminations. */
			assert(false);
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		{
			/* le premier enfant est l'expression, les suivants les paires */

			auto nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;
			auto const nom_enum = static_cast<TypeEnum *>(expression->type)->nom;
			auto &ds = contexte.donnees_structure(nom_enum);

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_expr = expression->chaine_calculee();

			generatrice.os << "switch (" << chaine_expr << ") {\n";

			for (auto i = 1l; i < nombre_enfants; ++i) {
				auto paire = *iter_enfant++;
				auto enf0 = paire->enfants.front();
				auto enf1 = paire->enfants.back();

				if (enf0->genre == GenreNoeud::INSTRUCTION_SINON) {
					generatrice.os << "default:";
				}
				else {
					auto feuilles = dls::tableau<base *>();
					rassemble_feuilles(enf0, feuilles);

					for (auto f : feuilles) {
						generatrice.os << "case " << chaine_valeur_enum(ds, f->chaine()) << ":\n";
					}
				}

				generatrice.os << " {\n";
				genere_code_C(enf1, generatrice, contexte, false);
				generatrice.os << "break;\n}\n";
			}

			generatrice.os << "}\n";

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

			auto nombre_enfant = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			auto type = expression->type;

			auto const est_pointeur = type->genre == GenreType::POINTEUR;

			if (est_pointeur) {
				type = static_cast<TypePointeur *>(type)->type_pointe;
			}

			auto const nom_union = static_cast<TypeStructure *>(type)->nom;
			auto &ds = contexte.donnees_structure(nom_union);

			genere_code_C(expression, generatrice, contexte, true);
			auto chaine_calculee = expression->chaine_calculee();
			chaine_calculee += (est_pointeur ? "->" : ".");

			generatrice.os << "switch (" << chaine_calculee << "membre_actif) {\n";

			for (auto i = 1; i < nombre_enfant; ++i) {
				auto enfant = *iter_enfant++;
				auto expr_paire = enfant->enfants.front();
				auto bloc_paire = enfant->enfants.back();

				contexte.empile_nombre_locales();

				if (expr_paire->genre == GenreNoeud::INSTRUCTION_SINON) {
					generatrice.os << "default";
				}
				else {
					auto iter_dm = ds.donnees_membres.trouve(expr_paire->chaine());
					auto const &dm = iter_dm->second;
					generatrice.os << "case " << dm.index_membre + 1;

					auto iter_membre = ds.donnees_membres.trouve(expr_paire->chaine());

					auto dv = DonneesVariable{};
					dv.type = ds.types[iter_membre->second.index_membre];
					dv.est_argument = true;
					dv.est_membre_emploie = true;
					dv.structure = chaine_calculee;

					contexte.pousse_locale(iter_membre->first, dv);
				}

				generatrice.os << ": {\n";
				genere_code_C(bloc_paire, generatrice, contexte, true);
				generatrice.os << "break;\n}\n";
				contexte.depile_nombre_locales();
			}

			generatrice.os << "}\n";

			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
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
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto enfant = b->enfants.front();
			genere_code_C(enfant, generatrice, contexte, expr_gauche);
			b->valeur_calculee = '(' + enfant->chaine_calculee() + ')';
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto variable = b->enfants.front();
			auto bloc = b->enfants.back();

			generatrice.os << "KsContexteProgramme ancien_contexte = contexte;\n";
			generatrice.os << "contexte = " << broye_chaine(variable) << ";\n";
			genere_code_C(bloc, generatrice, contexte, true);
			generatrice.os << "contexte = ancien_contexte;\n";

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto enfant = b->enfants.front();
			applique_transformation(enfant, generatrice, contexte, expr_gauche);
			b->valeur_calculee = enfant->valeur_calculee;
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
//		generatrice.os << "#pragma GCC diagnostic push\n";
//		generatrice.os << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

//		cree_info_type_C(contexte, generatrice, generatrice.os, noeud->type_);

//		generatrice.os << "#pragma GCC diagnostic pop\n";
	}
	else {
		if (noeud->type == TypeNoeudDependance::FONCTION && !genere_fonctions) {
			genere_declaration_fonction(noeud->noeud_syntactique, generatrice, contexte);
			generatrice.os << ";\n";
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

	cree_typedef(contexte, type, os);
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
	generatrice.os << "#pragma GCC diagnostic push\n";
	generatrice.os << "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n";

	POUR (contexte.typeuse.types_simples) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_pointeurs) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_references) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_structures) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_enums) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_tableaux_fixes) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_tableaux_dynamiques) cree_info_type_C(contexte, generatrice, generatrice.os, it);
	POUR (contexte.typeuse.types_fonctions) cree_info_type_C(contexte, generatrice, generatrice.os, it);

	generatrice.os << "#pragma GCC diagnostic pop\n";
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
	auto &df_fonc_alloc = contexte.module("Kuri")->donnees_fonction("allocatrice_défaut").front();
	auto noeud_alloc = graphe_dependance.cree_noeud_fonction(df_fonc_alloc.nom_broye, df_fonc_alloc.noeud_decl);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_alloc);

	auto &df_fonc_init_alloc = contexte.module("Kuri")->donnees_fonction("initialise_base_allocatrice").front();
	auto noeud_init_alloc = graphe_dependance.cree_noeud_fonction(df_fonc_init_alloc.nom_broye, df_fonc_init_alloc.noeud_decl);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_init_alloc);

	auto &df_fonc_log = contexte.module("Kuri")->donnees_fonction("__logueur_défaut").front();
	auto noeud_log = graphe_dependance.cree_noeud_fonction(df_fonc_log.nom_broye, df_fonc_log.noeud_decl);
	graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_log);

	if (pour_meta_programme) {
		auto &df_fonc_init = contexte.module("Kuri")->donnees_fonction("initialise_RC").front();
		auto noeud_init = graphe_dependance.cree_noeud_fonction(df_fonc_init.nom_broye, df_fonc_init.noeud_decl);
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
		auto const &ds = contexte.donnees_structure(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(ds.type);

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

	auto &df_fonc_alloc = contexte.module("Kuri")->donnees_fonction("allocatrice_défaut").front();
	auto &df_fonc_init_alloc = contexte.module("Kuri")->donnees_fonction("initialise_base_allocatrice").front();
	auto &df_fonc_log = contexte.module("Kuri")->donnees_fonction("__logueur_défaut").front();

	os << creation_contexte;
	os << "contexte.allocatrice = " << df_fonc_alloc.nom_broye << ";\n";
	os << df_fonc_init_alloc.nom_broye << "(contexte, &alloc_base);\n";
	os << "contexte.logueur = " << df_fonc_log.nom_broye << ";\n";
}

void genere_code_C(
		assembleuse_arbre const &arbre,
		ContexteGenerationCode &contexte,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	auto racine = arbre.racine();

	if (racine == nullptr) {
		return;
	}

	if (racine->genre != GenreNoeud::RACINE) {
		return;
	}

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
		noeud::base *noeud_appel,
		ContexteGenerationCode &contexte,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	auto df = noeud_appel->df;

	dls::flux_chaine os;

	auto generatrice = GeneratriceCodeC(contexte, os);

	auto temps_generation = 0.0;

	auto &graphe_dependance = contexte.graphe_dependance;

	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction(df->nom_broye);
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

	genere_code_C(noeud_appel, generatrice, contexte, true);

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

	for (auto &ds : contexte.structures) {
		ds.second.deja_genere = false;
	}

	POUR (contexte.typeuse.types_simples) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_pointeurs) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_references) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_structures) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_enums) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_tableaux_fixes) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_tableaux_dynamiques) it->ptr_info_type = "";
	POUR (contexte.typeuse.types_fonctions) it->ptr_info_type = "";
}

}  /* namespace noeud */
