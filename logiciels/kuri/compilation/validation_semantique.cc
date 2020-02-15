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

#include "validation_semantique.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/magasin.hh"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "expression.h"
#include "outils_lexemes.hh"
#include "typeuse.hh"

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

namespace noeud {

/* ************************************************************************** */

static auto &trouve_donnees_type(ContexteGenerationCode const &contexte, long index)
{
	return contexte.typeuse[index];
}

static auto &trouve_donnees_type(ContexteGenerationCode const &contexte, base *b)
{
	return trouve_donnees_type(contexte, b->index_type);
}

/* ************************************************************************** */

static void performe_validation_semantique(
		base *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche);

static long resoud_type_final(
		ContexteGenerationCode &contexte,
		DonneesTypeDeclare &type_declare,
		bool est_type_fonction = false,
		bool evalue_expr = true)
{
	if (type_declare.taille() == 0) {
		return -1l;
	}

	auto type_final = DonneesTypeFinal{};
	auto idx_expr = 0;

	for (auto i = 0; i < type_declare.taille(); ++i) {
		auto type = type_declare[i];

		if (type == GenreLexeme::TYPE_DE) {
			auto expr = type_declare.expressions[idx_expr++];
			assert(expr != nullptr);

			performe_validation_semantique(expr, contexte, false);

			if (expr->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto &dv = contexte.donnees_variable(expr->chaine());
				expr->index_type = dv.index_type;
			}

			auto &dt = trouve_donnees_type(contexte, expr);
			type_final.pousse(dt);
		}
		else if (type == GenreLexeme::TROIS_POINTS) {
			/* Pour la signature des fonctions, il faut préserver le type
			 * variadic sinon nous ne pourrions vérifier que les types attendus
			 * et ceux donnés sont compatibles, ou encore accidentellement
			 * assigner un pointeur de fonction prenant un tableau à un type
			 * espérant une liste variadique et vice versa.
			 */
			type_final.pousse(est_type_fonction ? type : GenreLexeme::TABLEAU);
		}
		else if (type == GenreLexeme::TABLEAU) {
			auto expr = type_declare.expressions[idx_expr++];

			if (expr != nullptr && evalue_expr) {
				performe_validation_semantique(expr, contexte, false);

				auto res = evalue_expression(contexte, expr);

				if (res.est_errone) {
					erreur::lance_erreur(
								res.message_erreur,
								contexte,
								expr->lexeme);
				}

				if (res.type != type_expression::ENTIER) {
					erreur::lance_erreur(
								"Attendu un type entier pour l'expression du tableau",
								contexte,
								expr->lexeme);
				}

				if (res.entier == 0) {
					erreur::lance_erreur(
								"L'expression évalue à zéro",
								contexte,
								expr->lexeme);
				}

				type = type | (static_cast<int>(res.entier) << 8);
			}

			type_final.pousse(type);
		}
		else if (type == GenreLexeme::DOLLAR) {
			for (auto &paire : contexte.paires_expansion_gabarit) {
				if (paire.first == type_declare.nom_gabarit) {
					auto &dt = contexte.typeuse[paire.second];
					type_final.pousse(dt);
				}
			}
		}
		else {
			type_final.pousse(type);
		}
	}

	return contexte.typeuse.ajoute_type(type_final);
}

/* ************************************************************************** */

static bool peut_etre_assigne(base *b, ContexteGenerationCode &contexte, bool emet_erreur = true)
{
	switch (b->genre) {
		default:
		{
			return false;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto iter_local = contexte.iter_locale(b->lexeme.chaine);

			if (iter_local != contexte.fin_locales()) {
				if (!iter_local->second.est_dynamique) {
					if (emet_erreur) {
						erreur::lance_erreur(
									"Ne peut pas assigner une variable locale non-dynamique",
									contexte,
									b->donnees_lexeme(),
									erreur::type_erreur::ASSIGNATION_INVALIDE);
					}

					return false;
				}

				return true;
			}

			auto iter_globale = contexte.iter_globale(b->lexeme.chaine);

			if (iter_globale != contexte.fin_globales()) {
				if (!contexte.non_sur()) {
					if (emet_erreur) {
						erreur::lance_erreur(
									"Ne peut pas assigner une variable globale en dehors d'un bloc 'nonsûr'",
									contexte,
									b->donnees_lexeme(),
									erreur::type_erreur::ASSIGNATION_INVALIDE);
					}

					return false;
				}

				if (!iter_globale->second.est_dynamique) {
					if (emet_erreur) {
						erreur::lance_erreur(
									"Ne peut pas assigner une variable globale non-dynamique",
									contexte,
									b->donnees_lexeme(),
									erreur::type_erreur::ASSIGNATION_INVALIDE);
					}

					return false;
				}

				return true;
			}

			return false;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_INDICE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			return peut_etre_assigne(b->enfants.front(), contexte, emet_erreur);
		}
	}
}

static auto derniere_instruction(base *b)
{
	if (b == nullptr) {
		return static_cast<base *>(nullptr);
	}

	if (est_instruction_retour(b->genre)) {
		return b;
	}

	if (b->genre == GenreNoeud::INSTRUCTION_SI) {
		if (b->enfants.taille() != 3) {
			return static_cast<base *>(nullptr);
		}
	}

	return derniere_instruction(b->dernier_enfant());
}

/* ************************************************************************** */

static auto valides_enfants(base *b, ContexteGenerationCode &contexte, bool expr_gauche)
{
	for (auto enfant : b->enfants) {
		performe_validation_semantique(enfant, contexte, expr_gauche);

		/* inutile de continuer à valider du code qui ne sera pas exécuté
		 * À FAIRE(retour précoce) : ajout d'un avertissement de compilation. */
		if (enfant->genre == GenreNoeud::INSTRUCTION_RETOUR) {
			break;
		}
	}
}

static auto valide_appel_pointeur_fonction(
		base *b,
		ContexteGenerationCode &contexte,
		dls::liste<dls::vue_chaine_compacte> const &noms_arguments,
		dls::chaine const &nom_fonction)
{
	for (auto const &nom : noms_arguments) {
		if (nom.est_vide()) {
			continue;
		}

		/* À FAIRE : trouve les données lexemes des arguments. */
		erreur::lance_erreur(
					"Les arguments d'un pointeur fonction ne peuvent être nommés",
					contexte,
					b->donnees_lexeme(),
					erreur::type_erreur::ARGUMENT_INCONNU);
	}

	auto index_type = (b->aide_generation_code == GENERE_CODE_PTR_FONC_MEMBRE)
			? b->index_type
			: contexte.type_locale(b->lexeme.chaine);
	auto &dt_fonc = contexte.typeuse[index_type];

	/* À FAIRE : bouge ça, trouve le type retour du pointeur de fonction. */

	if (dt_fonc.type_base() != GenreLexeme::FONC) {
		erreur::lance_erreur(
					"La variable doit être un pointeur vers une fonction",
					contexte,
					b->donnees_lexeme(),
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	valides_enfants(b, contexte, false);

	/* vérifie la compatibilité des arguments pour déterminer
	 * s'il y aura besoin d'une transformation. */
	auto nombre_type_retour = 0l;
	auto dt_params = donnees_types_parametres(contexte.typeuse, dt_fonc, nombre_type_retour);

	auto enfant = b->enfants.debut();

	auto debut_params = 0l;

	if (dt_params.taille() > 0) {
		if (dt_params[0] == contexte.index_type_contexte) {
			debut_params = 1;

			auto fonc_courante = contexte.donnees_fonction;

			if (fonc_courante != nullptr && dls::outils::possede_drapeau(fonc_courante->noeud_decl->drapeaux, FORCE_NULCTX)) {
				erreur::lance_erreur_fonction_nulctx(contexte, b, b, fonc_courante->noeud_decl);
			}
		}
		else {
			b->drapeaux |= FORCE_NULCTX;
		}
	}

	/* Validation des types passés en paramètre. */
	for (auto i = debut_params; i < dt_params.taille() - nombre_type_retour; ++i) {
		auto index_type_prm = dt_params[i];
		auto index_type_enf = (*enfant)->index_type;
		auto &type_prm = contexte.typeuse[dt_params[i]];

		if (type_prm.type_base() == GenreLexeme::TROIS_POINTS) {
			index_type_prm = contexte.typeuse.type_dereference_pour(index_type_prm);
		}

		auto transformation = cherche_transformation(
					contexte,
					index_type_enf,
					index_type_prm);

		if (transformation.type == TypeTransformation::IMPOSSIBLE) {
			auto &type_enf = contexte.typeuse[(*enfant)->index_type];

			erreur::lance_erreur_type_arguments(
						type_prm,
						type_enf,
						contexte,
						(*enfant)->lexeme,
						b->lexeme);
		}

		(*enfant)->transformation = transformation;
		++enfant;
	}

	b->nom_fonction_appel = nom_fonction;
	/* À FAIRE : multiples types retours */
	b->index_type = dt_params[dt_params.taille() - nombre_type_retour];
	b->index_type_fonc = index_type;
	b->aide_generation_code = APPEL_POINTEUR_FONCTION;
}

static void valide_acces_membre(
		ContexteGenerationCode &contexte,
		base *b,
		base *structure,
		base *membre,
		bool expr_gauche)
{
	performe_validation_semantique(structure, contexte, expr_gauche);

	auto index_type = structure->index_type;
	auto type_structure = contexte.typeuse[index_type];

	/* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
	while (type_structure.type_base() == GenreLexeme::POINTEUR || type_structure.type_base() == GenreLexeme::REFERENCE) {
		type_structure = type_structure.dereference();
		index_type = contexte.typeuse.ajoute_type(type_structure);
	}

	auto est_structure = (type_structure.type_base() & 0xff) == GenreLexeme::CHAINE_CARACTERE;

	if (membre->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
		/* si nous avons une structure, vérifie si nous avons un appel vers un
		 * pointeur de fonction */
		if (est_structure) {
			auto const index_structure = static_cast<long>(type_structure.type_base() >> 8);
			auto const &nom_membre = membre->chaine();

			auto &ds = contexte.donnees_structure(index_structure);

			if (!ds.est_enum && !ds.est_union) {
				auto const iter = ds.donnees_membres.trouve(nom_membre);

				if (iter != ds.donnees_membres.fin()) {
					/* ceci est le type de la fonction, l'analyse de l'appel
					 * vérifiera le type des arguments et ajournera le type du
					 * membre pour être celui du type de retour */
					b->index_type = ds.index_types[iter->second.index_membre];
					membre->index_type = b->index_type;
					membre->aide_generation_code = GENERE_CODE_PTR_FONC_MEMBRE;

					performe_validation_semantique(membre, contexte, expr_gauche);

					/* le type de l'accès est celui du retour de la fonction */
					b->index_type = membre->index_type;
					b->genre_valeur = GenreValeur::DROITE;
					return;
				}
			}
		}

		membre->enfants.push_front(structure);

		/* les noms d'arguments sont nécessaire pour trouver la bonne fonction,
		 * même vides, et il nous faut le bon compte de noms */
		auto *nom_args = std::any_cast<dls::liste<dls::vue_chaine_compacte>>(&membre->valeur_calculee);
		nom_args->push_front("");

		performe_validation_semantique(membre, contexte, expr_gauche);

		/* transforme le noeud */
		b->genre = GenreNoeud::EXPRESSION_APPEL_FONCTION;
		b->valeur_calculee = membre->valeur_calculee;
		b->module_appel = membre->module_appel;
		b->index_type = membre->index_type;
		b->nom_fonction_appel = membre->nom_fonction_appel;
		b->df = membre->df;
		b->enfants = membre->enfants;
		b->genre_valeur = GenreValeur::DROITE;

		return;
	}

	if (type_structure.type_base() == GenreLexeme::CHAINE) {
		if (membre->chaine() == "taille") {
			b->index_type = contexte.typeuse[TypeBase::Z64];
			return;
		}

		if (membre->chaine() == "pointeur") {
			b->index_type = contexte.typeuse[TypeBase::PTR_Z8];
			return;
		}

		erreur::membre_inconnu_chaine(contexte, b, structure, membre);
	}

	if (type_structure.type_base() == GenreLexeme::EINI) {
		if (membre->chaine() == "info") {
			auto const &ds = contexte.donnees_structure("InfoType");
			b->index_type = contexte.typeuse.type_pointeur_pour(ds.index_type);
			return;
		}

		if (membre->chaine() == "pointeur") {
			b->index_type = contexte.typeuse[TypeBase::PTR_Z8];
			return;
		}

		erreur::membre_inconnu_eini(contexte, b, structure, membre);
	}

	if ((type_structure.type_base() & 0xff) == GenreLexeme::TABLEAU) {
#ifdef NONSUR
		if (!contexte.non_sur() && expr_gauche) {
			erreur::lance_erreur(
						"Modification des membres du tableau hors d'un bloc 'nonsûr' interdite",
						contexte,
						b->lexeme,
						erreur::type_erreur::ASSIGNATION_INVALIDE);
		}
#endif
		if (membre->chaine() == "pointeur") {
			b->index_type = contexte.typeuse.type_pointeur_pour(contexte.typeuse.type_dereference_pour(index_type));
			return;
		}

		if (membre->chaine() == "taille") {
			b->index_type = contexte.typeuse[TypeBase::Z64];
			return;
		}

		erreur::membre_inconnu_tableau(contexte, b, structure, membre);
	}

	if (est_structure) {
		auto const index_structure = static_cast<long>(type_structure.type_base() >> 8);

		auto const &nom_membre = membre->chaine();

		auto &donnees_structure = contexte.donnees_structure(index_structure);

		if (donnees_structure.est_enum) {
			auto const iter = donnees_structure.donnees_membres.trouve(nom_membre);

			if (iter == donnees_structure.donnees_membres.fin()) {
				erreur::membre_inconnu(contexte, donnees_structure, b, structure, membre);
			}

			b->index_type = donnees_structure.index_type;
			b->genre_valeur = GenreValeur::DROITE;
			return;
		}

		auto const iter = donnees_structure.donnees_membres.trouve(nom_membre);

		if (iter == donnees_structure.donnees_membres.fin()) {
			erreur::membre_inconnu(contexte, donnees_structure, b, structure, membre);
		}

		b->index_type = donnees_structure.index_types[iter->second.index_membre];

		if (donnees_structure.est_union && !donnees_structure.est_nonsur) {
			b->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;
			b->valeur_calculee = iter->second.index_membre;

			if (expr_gauche) {
				contexte.renseigne_membre_actif(structure->chaine(), membre->chaine());
			}
			else {
				auto membre_actif = contexte.trouve_membre_actif(structure->chaine());

				/* si l'union vient d'un retour ou d'un paramètre, le membre actif sera inconnu */
				if (membre_actif != "" && membre_actif != membre->chaine()) {
					erreur::membre_inactif(contexte, b, structure, membre);
				}

				/* nous savons que nous avons le bon membre actif */
				b->aide_generation_code = IGNORE_VERIFICATION;
			}
		}

		return;
	}

	auto flux = dls::flux_chaine();
	flux << "Impossible d'accéder au membre d'un objet n'étant pas une structure";
	flux << ", le type est ";
	flux << chaine_type(type_structure, contexte);

	erreur::lance_erreur(
				flux.chn(),
				contexte,
				structure->donnees_lexeme(),
				erreur::type_erreur::TYPE_DIFFERENTS);
}

static void valide_type_fonction(base *b, ContexteGenerationCode &contexte)
{
	// certaines fonctions sont validées 2 fois...
	if (b->index_type != -1) {
		return;
	}

	using dls::outils::possede_drapeau;

	auto module = contexte.fichier(static_cast<size_t>(b->lexeme.fichier))->module;
	auto nom_fonction = b->lexeme.chaine;
	auto &vdf = module->donnees_fonction(nom_fonction);
	auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);

	for (auto &df : vdf) {
		if (df.noeud_decl == b) {
			donnees_fonction = &df;
			break;
		}
	}

	auto iter_enfant = b->enfants.debut();
	auto enfant = *iter_enfant++;
	assert(enfant->genre == GenreNoeud::DECLARATION_PARAMETRES_FONCTION);

	if (donnees_fonction->est_coroutine) {
		b->genre = GenreNoeud::DECLARATION_COROUTINE;
	}

	// -----------------------------------
	if (!contexte.pour_gabarit) {
		auto noms = dls::ensemble<dls::vue_chaine_compacte>();
		auto dernier_est_variadic = false;

		auto feuilles = dls::tableau<noeud::base *>();

		if (!enfant->enfants.est_vide()) {
			rassemble_feuilles(enfant->enfants.front(), feuilles);
		}

		for (auto feuille : feuilles) {
			if (noms.trouve(feuille->chaine()) != noms.fin()) {
				erreur::lance_erreur(
							"Redéfinition de l'argument",
							contexte,
							feuille->lexeme,
							erreur::type_erreur::ARGUMENT_REDEFINI);
			}

			if (dernier_est_variadic) {
				erreur::lance_erreur(
							"Argument déclaré après un argument variadic",
							contexte,
							feuille->lexeme,
							erreur::type_erreur::NORMAL);
			}

			auto &type_declare = feuille->type_declare;

			if (type_declare.est_gabarit) {
				donnees_fonction->noms_types_gabarits.pousse(type_declare.nom_gabarit);
				donnees_fonction->est_gabarit = true;
			}

			auto donnees_arg = DonneesArgument{};
			donnees_arg.type_declare = type_declare;
			donnees_arg.nom = feuille->chaine();

			noms.insere(feuille->chaine());

			/* doit être vrai uniquement pour le dernier argument */
			donnees_arg.est_variadic = donnees_arg.type_declare.type_base() == GenreLexeme::TROIS_POINTS;
			donnees_arg.est_dynamic = possede_drapeau(feuille->drapeaux, DYNAMIC);
			donnees_arg.est_employe = possede_drapeau(feuille->drapeaux, EMPLOYE);

			dernier_est_variadic = donnees_arg.est_variadic;

			donnees_fonction->est_variadique = donnees_arg.est_variadic;
			donnees_fonction->args.pousse(donnees_arg);

			if (feuille == feuilles.back()) {
				if (!donnees_fonction->est_externe && donnees_arg.est_variadic && est_invalide(donnees_arg.type_declare.dereference())) {
					erreur::lance_erreur(
								"La déclaration de fonction variadique sans type n'est"
								" implémentée que pour les fonctions externes",
								contexte,
								feuille->lexeme);
				}
			}
		}

		if (donnees_fonction->est_gabarit) {
			return;
		}
	}
	else {
		for (auto &arg : donnees_fonction->args) {
			arg.index_type = resoud_type_final(contexte, arg.type_declare);

			// XXX - ajournement du type déclaré pour pouvoir définir plus bas
			// le type de la fonction...
			auto &dt_arg = contexte.typeuse[arg.index_type];
			// ajourne seulement les données car il nous faut les expressions !
			arg.type_declare.donnees = dls::tableau_simple_compact<GenreLexeme>();

			auto debut = dt_arg.end() - 1;
			auto fin = dt_arg.begin() - 1;

			while (debut != fin) {
				arg.type_declare.donnees.pousse(*debut--);
			}

			arg.type_declare.est_gabarit = false;
		}
	}

	// -----------------------------------

	donnees_fonction->type_declare.pousse(
				donnees_fonction->est_coroutine ? GenreLexeme::COROUT : GenreLexeme::FONC);

	auto virgule = GenreLexeme::PARENTHESE_OUVRANTE;

	if (!possede_drapeau(b->drapeaux, FORCE_NULCTX)) {
		donnees_fonction->type_declare.pousse(virgule);
		ajoute_contexte_programme(contexte, donnees_fonction->type_declare);
		virgule = GenreLexeme::VIRGULE;
	}

	for (auto &arg : donnees_fonction->args) {
		donnees_fonction->type_declare.pousse(virgule);
		donnees_fonction->type_declare.pousse(arg.type_declare);
		virgule = GenreLexeme::VIRGULE;
	}

	donnees_fonction->type_declare.pousse(GenreLexeme::PARENTHESE_FERMANTE);

	donnees_fonction->type_declare.pousse(GenreLexeme::PARENTHESE_OUVRANTE);

	for (auto i = 0; i < donnees_fonction->types_retours_decl.taille(); ++i) {
		donnees_fonction->type_declare.pousse(donnees_fonction->types_retours_decl[i]);

		if (i < donnees_fonction->types_retours_decl.taille() - 1) {
			donnees_fonction->type_declare.pousse(GenreLexeme::VIRGULE);
		}
	}

	donnees_fonction->type_declare.pousse(GenreLexeme::PARENTHESE_FERMANTE);

	if (vdf.taille() > 1) {
		for (auto const &df : vdf) {
			if (df.noeud_decl == b) {
				continue;
			}

			if (df.type_declare == donnees_fonction->type_declare) {
				erreur::lance_erreur(
							"Redéfinition de la fonction",
							contexte,
							b->lexeme,
							erreur::type_erreur::FONCTION_REDEFINIE);
			}
		}
	}

	donnees_fonction->index_type = resoud_type_final(contexte, donnees_fonction->type_declare, true);

	b->index_type = resoud_type_final(contexte, b->type_declare);

	contexte.donnees_dependance.types_utilises.insere(b->index_type);

	for (auto i = 0; i < donnees_fonction->types_retours_decl.taille(); ++i) {
		auto idx_type = resoud_type_final(contexte, donnees_fonction->types_retours_decl[i]);
		donnees_fonction->idx_types_retours.pousse(idx_type);
		contexte.donnees_dependance.types_utilises.insere(idx_type);
	}

	for (auto &argument : donnees_fonction->args) {
		argument.index_type = resoud_type_final(contexte, argument.type_declare);
	}

	/* nous devons attendre d'avoir les types des arguments avant de
	 * pouvoir broyer le nom de la fonction */
	if (nom_fonction != "principale" && !possede_drapeau(b->drapeaux, EST_EXTERNE)) {
		donnees_fonction->nom_broye = broye_nom_fonction(contexte, *donnees_fonction, nom_fonction, module->nom);
	}
	else {
		donnees_fonction->nom_broye = nom_fonction;
	}

	contexte.graphe_dependance.cree_noeud_fonction(donnees_fonction->nom_broye, b);
}

enum {
	SYMBOLE_VARIABLE_LOCALE,
	SYMBOLE_VARIABLE_GLOBALE,
	SYMBOLE_INCONNU,
};

static auto cherche_symbole(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom)
{
	if (contexte.locale_existe(nom)) {
		return SYMBOLE_VARIABLE_LOCALE;
	}

	if (contexte.globale_existe(nom)) {
		return SYMBOLE_VARIABLE_GLOBALE;
	}

	return SYMBOLE_INCONNU;
}

static void performe_validation_semantique(
		base *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	auto &graphe = contexte.graphe_dependance;
	auto &donnees_dependance = contexte.donnees_dependance;
	auto fonction_courante = contexte.donnees_fonction;

	switch (b->genre) {
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::EXPRESSION_INDICE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		{
			/* À FAIRE : inférence de type
			 * - considération du type de retour des fonctions récursive
			 */

			using dls::outils::possede_drapeau;
			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			auto module = contexte.fichier(static_cast<size_t>(b->lexeme.fichier))->module;
			auto nom_fonction = b->lexeme.chaine;
			auto &vdf = module->donnees_fonction(nom_fonction);
			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);

			for (auto &df : vdf) {
				if (df.noeud_decl == b) {
					donnees_fonction = &df;
					break;
				}
			}

			/* Il est possible que certaines fonctions ne soient pas connectées
			 * dans le graphe de symboles alors que nous avons besoin d'elles,
			 * voir dans la fonction plus bas. */
			if (b->index_type == -1 && !donnees_fonction->est_gabarit) {
				valide_type_fonction(b, contexte);
			}

			if (donnees_fonction->est_gabarit && !contexte.pour_gabarit) {
				// nous ferons l'analyse sémantique plus tard
				return;
			}

			if (est_externe) {
				for (auto &argument : donnees_fonction->args) {
					argument.index_type = resoud_type_final(contexte, argument.type_declare);
					donnees_dependance.types_utilises.insere(argument.index_type);
				}

				auto noeud_dep = graphe.cree_noeud_fonction(nom_fonction, b);
				graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

				return;
			}

			contexte.commence_fonction(donnees_fonction);

			auto donnees_var = DonneesVariable{};

			if (!possede_drapeau(b->drapeaux, FORCE_NULCTX)) {
				donnees_var.est_dynamique = true;
				donnees_var.est_variadic = false;
				donnees_var.index_type = contexte.index_type_contexte;
				donnees_var.est_argument = true;

				contexte.pousse_locale("contexte", donnees_var);
				donnees_dependance.types_utilises.insere(contexte.index_type_contexte);
			}

			/* Pousse les paramètres sur la pile. */
			for (auto &argument : donnees_fonction->args) {
				donnees_dependance.types_utilises.insere(argument.index_type);

				auto index_dt = argument.index_type;

				donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = argument.est_dynamic;
				donnees_var.est_variadic = argument.est_variadic;
				donnees_var.index_type = index_dt;
				donnees_var.est_argument = true;

				contexte.pousse_locale(argument.nom, donnees_var);

				if (argument.est_employe) {
					auto &dt_var = contexte.typeuse[argument.index_type];
					auto id_structure = 0l;

					if (dt_var.type_base() == GenreLexeme::POINTEUR || dt_var.type_base() == GenreLexeme::REFERENCE) {
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

						contexte.pousse_locale(dm.first, donnees_var);
					}
				}
			}

			/* vérifie le type du bloc */
			auto bloc = b->enfants.back();

			auto noeud_dep = graphe.cree_noeud_fonction(donnees_fonction->nom_broye, b);

			performe_validation_semantique(bloc, contexte, true);
			auto inst_ret = derniere_instruction(bloc->dernier_enfant());

			/* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
			if (inst_ret == nullptr) {
				auto &dt = trouve_donnees_type(contexte, b);

				if (dt.type_base() != GenreLexeme::RIEN && !donnees_fonction->est_coroutine) {
					erreur::lance_erreur(
								"Instruction de retour manquante",
								contexte,
								b->lexeme,
								erreur::type_erreur::TYPE_DIFFERENTS);
				}

				b->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;
			}

			graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

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
			auto const nom_fonction = dls::chaine(b->lexeme.chaine);
			auto noms_arguments = std::any_cast<dls::liste<dls::vue_chaine_compacte>>(&b->valeur_calculee);

			b->genre_valeur = GenreValeur::DROITE;

			if (b->nom_fonction_appel != "") {
				/* Nous avons déjà validé ce noeud, sans doute via une syntaxe
				 * d'appel uniforme. */
				return;
			}

			/* Nous avons un pointeur vers une fonction. */
			if (b->aide_generation_code == GENERE_CODE_PTR_FONC_MEMBRE
					|| contexte.locale_existe(b->lexeme.chaine))
			{
				valide_appel_pointeur_fonction(b, contexte, *noms_arguments, nom_fonction);
				return;
			}

			/* Commence par valider les enfants puisqu'il nous faudra leurs
			 * types pour déterminer la fonction à appeler. */
			valides_enfants(b, contexte, false);

			auto res = cherche_donnees_fonction(
						contexte,
						nom_fonction,
						*noms_arguments,
						b->enfants,
						static_cast<size_t>(b->lexeme.fichier),
						static_cast<size_t>(b->module_appel));

			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);
			auto candidate = static_cast<DonneesCandidate *>(nullptr);

			auto poids = 0.0;

			for (auto &dc : res.candidates) {
				if (dc.etat == FONCTION_TROUVEE) {

					if (dc.poids_args > poids) {
						candidate = &dc;
						poids = dc.poids_args;
					}
				}
			}

			if (candidate == nullptr || candidate->df == nullptr) {
				erreur::lance_erreur_fonction_inconnue(
							contexte,
							b,
							res.candidates);
			}

			donnees_fonction = candidate->df;

			/* pour les directives d'exécution, la fonction courante est nulle */
			if (fonction_courante != nullptr) {
				using dls::outils::possede_drapeau;
				auto decl_fonc = fonction_courante->noeud_decl;

				if (possede_drapeau(decl_fonc->drapeaux, FORCE_NULCTX)) {
					auto decl_appel = donnees_fonction->noeud_decl;

					if (!donnees_fonction->est_externe && !possede_drapeau(decl_appel->drapeaux, FORCE_NULCTX)) {
						erreur::lance_erreur_fonction_nulctx(
									contexte,
									b,
									decl_fonc,
									decl_appel);
					}
				}
			}

			/* ---------------------- */

			if (!candidate->paires_expansion_gabarit.est_vide()) {
				auto noeud_decl = copie_noeud(contexte.assembleuse, candidate->df->noeud_decl);
				auto ndf = *candidate->df;
				ndf.noeud_decl = noeud_decl;

				auto module = contexte.fichier(static_cast<size_t>(noeud_decl->lexeme.fichier))->module;
				module->ajoute_donnees_fonctions(b->chaine(), ndf);

				// À FAIRE : le réusinage de l'arbre nous permettra d'avoir des blocs séparés du contexte.
				// sauvegarde état
				auto sauvegarde_m_locales = contexte.m_locales;
				auto sauvegarde_m_pile_nombre_locales = contexte.m_pile_nombre_locales;
				auto sauvegarde_m_nombre_locales = contexte.m_nombre_locales;
				auto sauvegarde_fonc_courante = fonction_courante;
				auto sauvegarde_m_noeuds_differes = contexte.m_noeuds_differes;
				auto sauvegarde_m_pile_nombre_differes = contexte.m_pile_nombre_differes;
				auto sauvegarde_m_nombre_differes = contexte.m_nombre_differes;

				contexte.termine_fonction();

				contexte.pour_gabarit = true;
				contexte.paires_expansion_gabarit = candidate->paires_expansion_gabarit;

				valide_type_fonction(noeud_decl, contexte);

				performe_validation_semantique(noeud_decl, contexte, expr_gauche);

				// XXX
				for (auto &df : module->donnees_fonction(b->chaine())) {
					if (df.noeud_decl == noeud_decl) {
						candidate->df = &df;
						donnees_fonction = &df;
					}
				}

				contexte.pour_gabarit = false;

				contexte.commence_fonction(sauvegarde_fonc_courante);

				contexte.m_locales = sauvegarde_m_locales;
				contexte.m_pile_nombre_locales = sauvegarde_m_pile_nombre_locales;
				contexte.m_nombre_locales = sauvegarde_m_nombre_locales;
				contexte.m_noeuds_differes = sauvegarde_m_noeuds_differes;
				contexte.m_pile_nombre_differes = sauvegarde_m_pile_nombre_differes;
				contexte.m_nombre_differes = sauvegarde_m_nombre_differes;
			}

			/* met en place les drapeaux sur les enfants */

			auto nombre_args_simples = candidate->exprs.taille();
			auto nombre_args_variadics = nombre_args_simples;

			if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
				/* ne compte pas le tableau */
				nombre_args_simples -= 1;
				nombre_args_variadics = candidate->transformations.taille();

				/* ajoute le type du tableau */
				auto noeud_tabl = candidate->exprs.back();
				auto taille_tableau = noeud_tabl->enfants.taille();
				auto &type_tabl = contexte.typeuse[noeud_tabl->index_type];

				auto dt_tfixe = DonneesTypeFinal{};
				dt_tfixe.pousse(GenreLexeme::TABLEAU | static_cast<int>(taille_tableau << 8));
				dt_tfixe.pousse(type_tabl);

				auto idx_dt_tfixe = contexte.typeuse.ajoute_type(dt_tfixe);
				donnees_dependance.types_utilises.insere(idx_dt_tfixe);
			}

			auto i = 0l;
			/* les drapeaux pour les arguments simples */
			for (; i < nombre_args_simples; ++i) {
				auto enfant = candidate->exprs[i];
				enfant->transformation = candidate->transformations[i];
			}

			/* les drapeaux pour les arguments variadics */
			if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
				auto noeud_tableau = candidate->exprs.back();
				auto enfant_tabl = noeud_tableau->enfants.debut();

				for (; i < nombre_args_variadics; ++i) {
					auto enfant = *enfant_tabl++;
					enfant->transformation = candidate->transformations[i];
				}
			}

			b->df = candidate->df;
			b->index_type_fonc = donnees_fonction->index_type;

			if (b->index_type == -1l) {
				/* À FAIRE : multiple types retours */
				b->index_type = donnees_fonction->idx_types_retours[0];
			}

			b->enfants.efface();

			for (auto enfant : candidate->exprs) {
				b->enfants.pousse(enfant);
			}

			if (donnees_fonction->nom_broye.est_vide()) {
				b->nom_fonction_appel = nom_fonction;
			}
			else {
				b->nom_fonction_appel = donnees_fonction->nom_broye;
			}

			donnees_dependance.fonctions_utilisees.insere(donnees_fonction->nom_broye);

			for (auto &argument : donnees_fonction->args) {
				donnees_dependance.types_utilises.insere(argument.index_type);
			}

			for (auto idx : donnees_fonction->idx_types_retours) {
				donnees_dependance.types_utilises.insere(idx);
			}

			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			valides_enfants(b, contexte, true);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			b->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto type_symbole = cherche_symbole(contexte, b->chaine());

			if (type_symbole == SYMBOLE_VARIABLE_LOCALE) {
				auto const &iter_locale = contexte.iter_locale(b->lexeme.chaine);
				b->index_type = iter_locale->second.index_type;

				donnees_dependance.types_utilises.insere(b->index_type);
				return;
			}

			if (type_symbole == SYMBOLE_VARIABLE_GLOBALE) {
				auto const &iter_locale = contexte.iter_globale(b->lexeme.chaine);
				b->index_type = iter_locale->second.index_type;

				donnees_dependance.types_utilises.insere(b->index_type);
				donnees_dependance.globales_utilisees.insere(b->lexeme.chaine);
				return;
			}

			/* Vérifie si c'est une fonction. */
			auto module = contexte.fichier(static_cast<size_t>(b->lexeme.fichier))->module;

			/* À FAIRE : trouve la fonction selon le type */
			if (module->fonction_existe(b->lexeme.chaine)) {
				auto &donnees_fonction = module->donnees_fonction(b->lexeme.chaine);
				b->index_type = donnees_fonction.front().index_type;
				b->nom_fonction_appel = donnees_fonction.front().nom_broye;

				donnees_dependance.types_utilises.insere(b->index_type);
				donnees_dependance.fonctions_utilisees.insere(b->nom_fonction_appel);
				return;
			}

			/* cherche dans les modules importés, À FAIRE: erreur si plusieurs symboles candidats */
			auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme.fichier));
			for (auto &nom_module : fichier->modules_importes) {
				module = contexte.module(nom_module);

				if (module->possede_fonction(b->lexeme.chaine)) {
					auto &donnees_fonction = module->donnees_fonction(b->lexeme.chaine);
					b->index_type = donnees_fonction.front().index_type;
					b->nom_fonction_appel = donnees_fonction.front().nom_broye;

					donnees_dependance.types_utilises.insere(b->index_type);
					donnees_dependance.fonctions_utilisees.insere(b->nom_fonction_appel);
					return;
				}
			}

			/* Nous avons peut-être une énumération. */
			if (contexte.structure_existe(b->lexeme.chaine)) {
				auto &donnees_structure = contexte.donnees_structure(b->lexeme.chaine);

				if (donnees_structure.est_enum) {
					b->index_type = donnees_structure.index_type;
					donnees_dependance.types_utilises.insere(b->index_type);
					return;
				}
			}

			erreur::lance_erreur(
						"Variable inconnue",
						contexte,
						b->lexeme,
						erreur::type_erreur::VARIABLE_INCONNUE);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();
			b->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto const nom_symbole = enfant1->chaine();

			if (enfant1->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme.fichier));

				if (fichier->importe_module(nom_symbole)) {
					auto module_importe = contexte.module(nom_symbole);

					if (module_importe == nullptr) {
						erreur::lance_erreur(
									"module inconnu",
									contexte,
									enfant1->donnees_lexeme(),
									erreur::type_erreur::MODULE_INCONNU);
					}

					auto const nom_fonction = enfant2->chaine();

					if (!module_importe->possede_fonction(nom_fonction)) {
						erreur::lance_erreur(
									"Le module ne possède pas la fonction",
									contexte,
									enfant2->donnees_lexeme(),
									erreur::type_erreur::FONCTION_INCONNUE);
					}

					enfant2->module_appel = static_cast<int>(module_importe->id);

					performe_validation_semantique(enfant2, contexte, expr_gauche);

					b->index_type = enfant2->index_type;
					b->aide_generation_code = ACCEDE_MODULE;

					return;
				}
			}

			valide_acces_membre(contexte, b, enfant1, enfant2, expr_gauche);

			donnees_dependance.types_utilises.insere(b->index_type);

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			performe_validation_semantique(expression, contexte, false);

			if (expression->index_type == -1l) {
				erreur::lance_erreur(
							"Impossible de définir le type de la variable !",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			/* NOTE : l'appel à performe_validation_semantique plus bas peut
			 * changer le vecteur et invalider une référence ou un pointeur,
			 * donc nous faisons une copie... */
			auto const dt = trouve_donnees_type(contexte, expression);

			if (dt.type_base() == GenreLexeme::RIEN) {
				erreur::lance_erreur(
							"Impossible d'assigner une expression de type 'rien' à une variable !",
							contexte,
							b->lexeme,
							erreur::type_erreur::ASSIGNATION_RIEN);
			}

			/* a, b = foo() */
			if (variable->identifiant() == GenreLexeme::VIRGULE) {
				if (expression->genre != GenreNoeud::EXPRESSION_APPEL_FONCTION) {
					erreur::lance_erreur(
								"Une virgule ne peut se trouver qu'à gauche d'un appel de fonction.",
								contexte,
								variable->lexeme,
								erreur::type_erreur::NORMAL);
				}

				dls::tableau<base *> feuilles;
				rassemble_feuilles(variable, feuilles);

				/* Utilisation du type de la fonction et non
				 * DonneesFonction::idx_types_retour car les pointeurs de
				 * fonctions n'ont pas de DonneesFonction. */
				auto idx_dt_fonc = expression->index_type_fonc;
				auto &dt_fonc = trouve_donnees_type(contexte, idx_dt_fonc);

				auto nombre_type_retour = 0l;
				auto dt_params = donnees_types_parametres(contexte.typeuse, dt_fonc, nombre_type_retour);

				if (feuilles.taille() != nombre_type_retour) {
					erreur::lance_erreur(
								"L'ignorance d'une valeur de retour non implémentée.",
								contexte,
								variable->lexeme,
								erreur::type_erreur::NORMAL);
				}

				auto decalage = dt_params.taille() - nombre_type_retour;

				for (auto i = 0l; i < feuilles.taille(); ++i) {
					auto &f = feuilles[i];

					if (f->index_type == -1l) {
						f->index_type = dt_params[decalage + i];
					}

					performe_validation_semantique(f, contexte, true);
				}

				return;
			}

			performe_validation_semantique(variable, contexte, true);

			if (!est_valeur_gauche(variable->genre_valeur)) {
				erreur::lance_erreur(
							"Impossible d'assigner une expression à une valeur-droite !",
							contexte,
							b->lexeme,
							erreur::type_erreur::ASSIGNATION_INVALIDE);
			}

			if (!peut_etre_assigne(variable, contexte)) {
				erreur::lance_erreur(
							"Impossible d'assigner l'expression à la variable !",
							contexte,
							b->lexeme,
							erreur::type_erreur::ASSIGNATION_INVALIDE);
			}

			auto transformation = cherche_transformation(
						contexte,
						expression->index_type,
						variable->index_type);

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				erreur::lance_erreur_assignation_type_differents(
							contexte.typeuse[variable->index_type],
							contexte.typeuse[expression->index_type],
							contexte,
							b->lexeme);
			}

			expression->transformation = transformation;

			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			if (b->enfants.taille() == 0) {
				auto variable = b;
				variable->index_type = resoud_type_final(contexte, variable->type_declare);

				if (variable->index_type == -1) {
					erreur::lance_erreur("variable déclarée sans type", contexte, variable->lexeme);
				}

				auto type_symbole = cherche_symbole(contexte, variable->chaine());

				if (type_symbole != SYMBOLE_INCONNU) {
					erreur::lance_erreur(
								"Redéfinition du symbole !",
								contexte,
								variable->lexeme,
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}

				auto donnees_var = DonneesVariable{};
				donnees_var.est_externe = (variable->drapeaux & EST_EXTERNE) != 0;
				donnees_var.est_dynamique = (variable->drapeaux & DYNAMIC) != 0;
				donnees_var.index_type = variable->index_type;

				if (fonction_courante == nullptr) {
					contexte.pousse_globale(variable->lexeme.chaine, donnees_var);
					graphe.cree_noeud_globale(variable->lexeme.chaine, b);
				}
				else {
					contexte.pousse_locale(variable->lexeme.chaine, donnees_var);
				}

				donnees_dependance.types_utilises.insere(variable->index_type);
				return;
			}

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			performe_validation_semantique(expression, contexte, false);

			if (expression->index_type == -1l) {
				erreur::lance_erreur(
							"Impossible de définir le type de l'expression !",
							contexte,
							expression->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			auto nom_variable = variable->chaine();

			auto type_symbole = cherche_symbole(contexte, nom_variable);

			if (type_symbole != SYMBOLE_INCONNU) {
				erreur::lance_erreur(
							"Redéfinition du symbole !",
							contexte,
							variable->lexeme,
							erreur::type_erreur::VARIABLE_REDEFINIE);
			}

			variable->index_type = resoud_type_final(contexte, variable->type_declare);

			if (variable->index_type == -1) {
				variable->index_type = expression->index_type;
			}
			else {
				auto transformation = cherche_transformation(
							contexte,
							expression->index_type,
							variable->index_type);

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					erreur::lance_erreur_assignation_type_differents(
								contexte.typeuse[variable->index_type],
								contexte.typeuse[expression->index_type],
								contexte,
								b->lexeme);
				}

				expression->transformation = transformation;
			}

			auto donnees_var = DonneesVariable{};
			donnees_var.est_externe = (variable->drapeaux & EST_EXTERNE) != 0;
			donnees_var.est_dynamique = (variable->drapeaux & DYNAMIC) != 0;
			donnees_var.index_type = variable->index_type;

			if (donnees_var.est_externe) {
				erreur::lance_erreur(
							"Ne peut pas assigner une variable globale externe dans sa déclaration",
							contexte,
							b->lexeme);
			}

			if (fonction_courante == nullptr) {
				contexte.pousse_globale(variable->lexeme.chaine, donnees_var);
				graphe.cree_noeud_globale(variable->lexeme.chaine, b);
			}
			else {
				contexte.pousse_locale(variable->lexeme.chaine, donnees_var);
			}

			donnees_dependance.types_utilises.insere(variable->index_type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse[TypeBase::R32];

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse[TypeBase::Z32];

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			b->genre_valeur = GenreValeur::DROITE;

			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			/* À FAIRE : transformation automatique */

			performe_validation_semantique(enfant1, contexte, expr_gauche);
			performe_validation_semantique(enfant2, contexte, expr_gauche);

			auto index_type1 = enfant1->index_type;
			auto index_type2 = enfant2->index_type;

			auto type1 = contexte.typeuse[index_type1];

			/* détecte a comp b comp c */
			if (est_operateur_comp(b->lexeme.genre) && est_operateur_comp(enfant1->lexeme.genre)) {
				b->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
				b->index_type = contexte.typeuse[TypeBase::BOOL];

				auto type_op = b->lexeme.genre;

				index_type1 = enfant1->enfants.back()->index_type;

				auto candidats = cherche_candidats_operateurs(contexte, index_type1, index_type2, type_op);
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					erreur::lance_erreur_type_operation(contexte, b);
				}

				b->op = meilleur_candidat->op;

				if (!b->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(b->op->nom_fonction);
				}
			}
			else if (b->lexeme.genre == GenreLexeme::CROCHET_OUVRANT) {
				b->genre = GenreNoeud::EXPRESSION_INDICE;
				b->genre_valeur = GenreValeur::TRANSCENDANTALE;

				if (type1.type_base() == GenreLexeme::REFERENCE) {
					enfant1->transformation = TypeTransformation::DEREFERENCE;
					type1 = type1.dereference();
				}

				auto type_base = type1.type_base();

				switch (type_base & 0xff) {
					case GenreLexeme::TABLEAU:
					{
						b->index_type = contexte.typeuse.ajoute_type(type1.dereference());

						auto taille_tableau = static_cast<int>(type_base >> 8);

						if (taille_tableau != 0) {
							auto res = evalue_expression(contexte, enfant2);

							if (!res.est_errone) {
								if (res.entier >= taille_tableau) {
									erreur::lance_erreur_acces_hors_limites(
												contexte,
												enfant2,
												taille_tableau,
												type1,
												res.entier);
								}

								/* nous savons que l'accès est dans les limites,
								 * évite d'émettre le code de vérification */
								b->aide_generation_code = IGNORE_VERIFICATION;
							}
						}

						break;
					}
					case GenreLexeme::POINTEUR:
					{
						b->index_type = contexte.typeuse.ajoute_type(type1.dereference());
						break;
					}
					case GenreLexeme::CHAINE:
					{
						b->index_type = contexte.typeuse[TypeBase::Z8];
						break;
					}
					default:
					{
						dls::flux_chaine ss;
						ss << "Le type '" << chaine_type(type1, contexte)
						   << "' ne peut être déréférencé par opérateur[] !";

						erreur::lance_erreur(
									ss.chn(),
									contexte,
									b->lexeme,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
				}
			}
			else {
				auto type_op = b->lexeme.genre;

				if (est_assignation_operee(type_op)) {
					if (!peut_etre_assigne(enfant1, contexte)) {
						erreur::lance_erreur(
									"Impossible d'assigner l'expression à la variable !",
									contexte,
									b->lexeme,
									erreur::type_erreur::ASSIGNATION_INVALIDE);
					}

					type_op = operateur_pour_assignation_operee(type_op);
					b->drapeaux |= EST_ASSIGNATION_OPEREE;
				}

				auto candidats = cherche_candidats_operateurs(contexte, index_type1, index_type2, type_op);
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					erreur::lance_erreur_type_operation(contexte, b);
				}

				b->index_type = meilleur_candidat->op->index_resultat;
				b->op = meilleur_candidat->op;
				enfant1->transformation = meilleur_candidat->transformation_type1;
				enfant2->transformation = meilleur_candidat->transformation_type2;

				if (!b->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(b->op->nom_fonction);
				}
			}

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			b->genre_valeur = GenreValeur::DROITE;

			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte, expr_gauche);
			auto index_type = enfant->index_type;

			auto dt = contexte.typeuse[index_type];

			if (dt.type_base() == GenreLexeme::REFERENCE) {
				enfant->transformation = TypeTransformation::DEREFERENCE;
				dt = dt.dereference();
				index_type = contexte.typeuse.ajoute_type(dt);
			}

			if (b->index_type == -1l) {
				if (b->identifiant() == GenreLexeme::AROBASE) {
					if (!est_valeur_gauche(enfant->genre_valeur)) {
						erreur::lance_erreur(
									"Ne peut pas prendre l'adresse d'une valeur-droite.",
									contexte,
									enfant->lexeme);
					}


					b->index_type = contexte.typeuse.type_pointeur_pour(index_type);
				}
				else {
					auto op = cherche_operateur_unaire(contexte.operateurs, index_type, b->identifiant());

					if (op == nullptr) {
						erreur::lance_erreur_type_operation_unaire(contexte, b);
					}

					b->index_type = op->index_resultat;
					b->op = op;
				}
			}

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			b->genre_valeur = GenreValeur::DROITE;

			if (b->enfants.est_vide()) {
				b->index_type = contexte.typeuse[TypeBase::RIEN];

				if (!fonction_courante->est_coroutine && (fonction_courante->idx_types_retours[0] != b->index_type)) {
					erreur::lance_erreur(
								"Expression de retour manquante",
								contexte,
								b->lexeme);
				}

				donnees_dependance.types_utilises.insere(b->index_type);
				return;
			}

			assert(b->enfants.taille() == 1);

			auto enfant = b->enfants.front();
			auto nombre_retour = fonction_courante->idx_types_retours.taille();

			if (nombre_retour > 1) {
				if (enfant->identifiant() == GenreLexeme::VIRGULE) {
					dls::tableau<base *> feuilles;
					rassemble_feuilles(enfant, feuilles);

					if (feuilles.taille() != fonction_courante->idx_types_retours.taille()) {
						erreur::lance_erreur(
									"Le compte d'expression de retour est invalide",
									contexte,
									b->lexeme);
					}

					for (auto i = 0l; i < feuilles.taille(); ++i) {
						auto f = feuilles[i];
						performe_validation_semantique(f, contexte, false);

						auto transformation = cherche_transformation(
									contexte,
									f->index_type,
									fonction_courante->idx_types_retours[i]);

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							erreur::lance_erreur_type_retour(
										contexte.typeuse[fonction_courante->idx_types_retours[0]],
										contexte.typeuse[f->index_type],
										contexte,
										enfant->lexeme,
										b->lexeme);
						}

						f->transformation = transformation;

						donnees_dependance.types_utilises.insere(f->index_type);
					}

					/* À FAIRE : multiples types de retour */
					b->index_type = feuilles[0]->index_type;
					b->genre = GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE;
				}
				else if (enfant->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
					performe_validation_semantique(enfant, contexte, false);

					/* À FAIRE : multiples types de retour, confirmation typage */
					b->index_type = enfant->index_type;
					b->genre = GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE;
				}
				else {
					erreur::lance_erreur(
								"Le compte d'expression de retour est invalide",
								contexte,
								b->lexeme);
				}
			}
			else {
				performe_validation_semantique(enfant, contexte, false);
				b->index_type = fonction_courante->idx_types_retours[0];
				b->genre = GenreNoeud::INSTRUCTION_RETOUR_SIMPLE;

				auto transformation = cherche_transformation(
							contexte,
							enfant->index_type,
							fonction_courante->idx_types_retours[0]);

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					erreur::lance_erreur_type_arguments(
								contexte.typeuse[fonction_courante->idx_types_retours[0]],
								contexte.typeuse[enfant->index_type],
								contexte,
								enfant->lexeme,
								b->lexeme);
				}

				enfant->transformation = transformation;
			}

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			/* fais en sorte que les caractères échappés ne soient pas comptés
			 * comme deux caractères distincts, ce qui ne peut se faire avec la
			 * dls::vue_chaine */
			dls::chaine corrigee;
			corrigee.reserve(b->lexeme.chaine.taille());

			for (auto i = 0l; i < b->lexeme.chaine.taille(); ++i) {
				auto c = b->lexeme.chaine[i];

				if (c == '\\') {
					c = dls::caractere_echappe(&b->lexeme.chaine[i]);
					++i;
				}

				corrigee.pousse(c);
			}

			/* À FAIRE : ceci ne fonctionne pas dans le cas des noeuds différés
			 * où la valeur calculee est redéfinie. */
			b->valeur_calculee = corrigee;
			b->index_type = contexte.typeuse[TypeBase::CHAINE];
			b->genre_valeur = GenreValeur::DROITE;

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse[TypeBase::BOOL];

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse[TypeBase::Z8];

			donnees_dependance.types_utilises.insere(b->index_type);
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();

			auto enfant1 = *iter_enfant++;
			auto enfant2 = *iter_enfant++;

			performe_validation_semantique(enfant1, contexte, false);
			auto index_type = enfant1->index_type;
			auto const &type_condition = contexte.typeuse[index_type];

			if (type_condition.type_base() != GenreLexeme::BOOL) {
				erreur::lance_erreur("Attendu un type booléen pour l'expression 'si'",
									 contexte,
									 enfant1->donnees_lexeme(),
									 erreur::type_erreur::TYPE_DIFFERENTS);
			}

			performe_validation_semantique(enfant2, contexte, true);

			/* noeud 3 : sinon (optionel) */
			if (nombre_enfants == 3) {
				auto enfant3 = *iter_enfant++;
				performe_validation_semantique(enfant3, contexte, true);
			}

			/* pour les expressions x = si y { z } sinon { w } */
			b->index_type = enfant2->index_type;

			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
#ifdef AVEC_LLVM
			/* Évite les crash lors de l'estimation du bloc suivant les
			 * contrôles de flux. */
			b->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
#endif

			contexte.empile_nombre_locales();

			valides_enfants(b, contexte, true);

			if (b->enfants.est_vide()) {
				b->index_type = contexte.typeuse[TypeBase::RIEN];
			}
			else {
				b->index_type = b->enfants.back()->index_type;
			}

			contexte.depile_nombre_locales();

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter = b->enfants.debut();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;
			auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

			auto verifie_redefinition_variable = [](base *b_local, ContexteGenerationCode &contexte_loc)
			{
				if (contexte_loc.locale_existe(b_local->chaine())) {
					erreur::lance_erreur(
								"(Boucle pour) rédéfinition de la variable",
								contexte_loc,
								b_local->donnees_lexeme(),
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}

				if (contexte_loc.globale_existe(b_local->chaine())) {
					erreur::lance_erreur(
								"(Boucle pour) rédéfinition de la variable globale",
								contexte_loc,
								b_local->donnees_lexeme(),
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}
			};


			performe_validation_semantique(enfant2, contexte, true);

			/* À FAIRE : utilisation du type */
			auto df = static_cast<DonneesFonction *>(nullptr);

			auto feuilles = dls::tableau<base *>{};
			rassemble_feuilles(enfant1, feuilles);

			for (auto f : feuilles) {
				verifie_redefinition_variable(f, contexte);
			}

			auto requiers_index = feuilles.taille() == 2;

			auto index_type = enfant2->index_type;
			auto &type = contexte.typeuse[index_type];

			/* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
			 * type de retour d'une coroutine n'interfère avec le type d'une
			 * variable (par exemple quand nous retournons une chaine). */
			if (enfant2->genre == GenreNoeud::EXPRESSION_PLAGE) {
				if (requiers_index) {
					b->aide_generation_code = GENERE_BOUCLE_PLAGE_INDEX;
				}
				else {
					b->aide_generation_code = GENERE_BOUCLE_PLAGE;
				}
			}
			else if (enfant2->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION && enfant2->df->est_coroutine) {
				enfant1->index_type = enfant2->index_type;

				df = enfant2->df;
				auto nombre_vars_ret = df->idx_types_retours.taille();

				if (feuilles.taille() == nombre_vars_ret) {
					requiers_index = false;
					b->aide_generation_code = GENERE_BOUCLE_COROUTINE;
				}
				else if (feuilles.taille() == nombre_vars_ret + 1) {
					requiers_index = true;
					b->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
				}
				else {
					erreur::lance_erreur(
								"Mauvais compte d'arguments à déployer",
								contexte,
								enfant1->lexeme);
				}
			}
			else {
				if ((type.type_base() & 0xff) == GenreLexeme::TABLEAU) {
					index_type = contexte.typeuse.type_dereference_pour(index_type);

					if (requiers_index) {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}
				}
				else if (type.type_base() == GenreLexeme::CHAINE) {
					index_type = contexte.typeuse[TypeBase::Z8];
					enfant1->index_type = index_type;

					if (requiers_index) {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}
				}
				else {
					auto valeur = contexte.est_locale_variadique(enfant2->chaine());

					if (!valeur) {
						erreur::lance_erreur(
									"La variable n'est ni un argument variadic, ni un tableau",
									contexte,
									enfant2->donnees_lexeme());
					}
				}
			}

			donnees_dependance.types_utilises.insere(index_type);

			contexte.empile_nombre_locales();

			auto est_dynamique = peut_etre_assigne(enfant2, contexte, false);
			auto nombre_feuilles = feuilles.taille() - requiers_index;

			for (auto i = 0l; i < nombre_feuilles; ++i) {
				auto f = feuilles[i];

				auto donnees_var = DonneesVariable{};

				if (df != nullptr) {
					donnees_var.index_type = df->idx_types_retours[i];
				}
				else {
					donnees_var.index_type = index_type;
				}

				donnees_var.est_dynamique = est_dynamique;

				contexte.pousse_locale(f->chaine(), donnees_var);
			}

			if (requiers_index) {
				auto idx = feuilles.back();

				index_type = contexte.typeuse[TypeBase::Z32];
				idx->index_type = index_type;

				auto donnees_var = DonneesVariable{};
				donnees_var.index_type = index_type;
				donnees_var.est_dynamique = est_dynamique;

				contexte.pousse_locale(idx->chaine(), donnees_var);
			}

			/* À FAIRE : ceci duplique logique coulisse. */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);
			auto goto_brise = "__boucle_pour_brise" + dls::vers_chaine(b);

			contexte.empile_goto_continue(enfant1->chaine(), goto_continue);
			contexte.empile_goto_arrete(enfant1->chaine(), (enfant4 != nullptr) ? goto_brise : goto_apres);

			performe_validation_semantique(enfant3, contexte, true);

			if (enfant4 != nullptr) {
				performe_validation_semantique(enfant4, contexte, true);

				if (enfant5 != nullptr) {
					performe_validation_semantique(enfant5, contexte, true);
				}
			}

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			contexte.depile_nombre_locales();

			break;
		}
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = resoud_type_final(contexte, b->type_declare);

			/* À FAIRE : vérifie compatibilité */

			if (b->index_type == -1l) {
				erreur::lance_erreur(
							"Ne peut transtyper vers un type invalide",
							contexte,
							b->donnees_lexeme(),
							erreur::type_erreur::TYPE_INCONNU);
			}

			donnees_dependance.types_utilises.insere(b->index_type);

			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte, false);

			if (enfant->index_type == -1l) {
				erreur::lance_erreur(
							"Ne peut calculer le type d'origine",
							contexte,
							enfant->donnees_lexeme(),
							erreur::type_erreur::TYPE_INCONNU);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse[TypeBase::PTR_NUL];
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto type_declare = std::any_cast<DonneesTypeDeclare>(b->valeur_calculee);
			b->valeur_calculee = resoud_type_final(contexte, type_declare);
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse[TypeBase::N32];
			valides_enfants(b, contexte, false);
			break;
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			auto iter = b->enfants.debut();

			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			performe_validation_semantique(enfant1, contexte, false);
			performe_validation_semantique(enfant2, contexte, false);

			auto index_type_debut = enfant1->index_type;
			auto index_type_fin   = enfant2->index_type;

			if (index_type_debut == -1l || index_type_fin == -1l) {
				erreur::lance_erreur(
							"Les types de l'expression sont invalides !",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			auto const &type_debut = contexte.typeuse[index_type_debut];
			auto const &type_fin   = contexte.typeuse[index_type_fin];

			if (type_debut.est_invalide() || type_fin.est_invalide()) {
				erreur::lance_erreur(
							"Les types de l'expression sont invalides !",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			if (index_type_debut != index_type_fin) {
				erreur::lance_erreur_type_operation(
							type_debut,
							type_fin,
							contexte,
							b->lexeme);
			}

			auto const type = type_debut.type_base();

			if (!est_type_entier_naturel(type) && !est_type_entier_relatif(type) && !est_type_reel(type)) {
				erreur::lance_erreur(
							"Attendu des types réguliers dans la plage de la boucle 'pour'",
							contexte,
							b->donnees_lexeme(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			b->index_type = index_type_debut;

			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			valides_enfants(b, contexte, false);

			auto chaine_var = b->enfants.est_vide() ? dls::vue_chaine_compacte{""} : b->enfants.front()->chaine();

			auto label_goto = (b->lexeme.genre == GenreLexeme::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			if (label_goto.est_vide()) {
				if (chaine_var.est_vide()) {
					erreur::lance_erreur(
								"'continue' ou 'arrête' en dehors d'une boucle",
								contexte,
								b->lexeme,
								erreur::type_erreur::CONTROLE_INVALIDE);
				}
				else {
					erreur::lance_erreur(
								"Variable inconnue",
								contexte,
								b->enfants.front()->donnees_lexeme(),
								erreur::type_erreur::VARIABLE_INCONNUE);
				}
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(b->enfants.front(), contexte, true);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(b->enfants.front(), contexte, true);
			performe_validation_semantique(b->enfants.back(), contexte, false);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_DIFFERE:
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			valides_enfants(b, contexte, true);
			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			assert(b->enfants.taille() == 2);
			auto iter = b->enfants.debut();
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			performe_validation_semantique(enfant1, contexte, false);

			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(enfant2, contexte, true);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			auto &dt = contexte.typeuse[enfant1->index_type];

			/* À FAIRE : tests */
			if (dt.type_base() != GenreLexeme::BOOL) {
				erreur::lance_erreur(
							"Une expression booléenne est requise pour la boucle 'tantque'",
							contexte,
							enfant1->lexeme,
							erreur::type_erreur::TYPE_ARGUMENT);
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_NONSUR:
		{
			contexte.non_sur(true);
			performe_validation_semantique(b->enfants.front(), contexte, true);
			contexte.non_sur(false);

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			b->genre_valeur = GenreValeur::DROITE;

			dls::tableau<base *> feuilles;
			rassemble_feuilles(b->enfants.front(), feuilles);

			for (auto f : feuilles) {
				performe_validation_semantique(f, contexte, false);
			}

			if (feuilles.est_vide()) {
				return;
			}

			auto premiere_feuille = feuilles.front();

			auto type_feuille = premiere_feuille->index_type;

			for (auto f : feuilles) {
				/* À FAIRE : test */
				if (f->index_type != type_feuille) {
					auto dt_feuille0 = contexte.typeuse[type_feuille];
					auto dt_feuille1 = trouve_donnees_type(contexte, f);

					erreur::lance_erreur_assignation_type_differents(
								dt_feuille0,
								dt_feuille1,
								contexte,
								f->lexeme);
				}
			}

			DonneesTypeFinal dt;
			dt.pousse(GenreLexeme::TABLEAU | static_cast<int>(feuilles.taille() << 8));
			dt.pousse(contexte.typeuse[type_feuille]);

			b->index_type = contexte.typeuse.ajoute_type(dt);

			/* ajoute également le type de pointeur pour la génération de code C */
			dt = DonneesTypeFinal{};
			dt.pousse(GenreLexeme::POINTEUR);
			dt.pousse(contexte.typeuse[type_feuille]);

			auto index_type_ptr = contexte.typeuse.ajoute_type(dt);

			donnees_dependance.types_utilises.insere(b->index_type);
			donnees_dependance.types_utilises.insere(index_type_ptr);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			b->genre_valeur = GenreValeur::DROITE;

			/* cherche la structure dans le tableau de structure */
			if (!contexte.structure_existe(b->chaine())) {
				erreur::lance_erreur(
							"Structure inconnue",
							contexte,
							b->lexeme,
							erreur::type_erreur::STRUCTURE_INCONNUE);
			}

			auto &donnees_struct = contexte.donnees_structure(b->chaine());
			b->index_type = donnees_struct.index_type;

			if (donnees_struct.est_enum) {
				erreur::lance_erreur(
							"Ne peut pas construire une énumération",
							contexte,
							b->lexeme);
			}

			auto liste_params = std::any_cast<dls::tableau<dls::vue_chaine_compacte>>(&b->valeur_calculee);

			auto noms_rencontres = dls::ensemble<dls::vue_chaine_compacte>();

			/* À FAIRE : il nous faut les noms -> il nous faut les assignations
			 * dans l'arbre */
			for (auto &nom : *liste_params) {
				if (noms_rencontres.trouve(nom) != noms_rencontres.fin()) {
					erreur::lance_erreur(
								"Redéfinition de l'initialisation du membre",
								contexte,
								b->lexeme);
				}

				noms_rencontres.insere(nom);
			}

			if (donnees_struct.est_union) {
				if (liste_params->taille() > 1) {
					erreur::lance_erreur(
								"On ne peut initialiser qu'un seul membre d'une union à la fois",
								contexte,
								b->lexeme);
				}
				else if (liste_params->taille() == 0) {
					erreur::lance_erreur(
								"On doit initialiser au moins un membre de l'union",
								contexte,
								b->lexeme);
				}
			}

			for (auto enfant : b->enfants) {
				performe_validation_semantique(enfant, contexte, false);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto enfant = b->enfants.front();

			performe_validation_semantique(enfant, contexte, false);

			if (enfant->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto &dv = contexte.donnees_variable(enfant->chaine());
				enfant->index_type = dv.index_type;
			}

			auto &dt_enf = trouve_donnees_type(contexte, enfant);
			auto nom_struct = "InfoType";

			switch (dt_enf.type_base() & 0xff) {
				default:
				{
					break;
				}
				case GenreLexeme::N8:
				case GenreLexeme::Z8:
				case GenreLexeme::N16:
				case GenreLexeme::Z16:
				case GenreLexeme::N32:
				case GenreLexeme::Z32:
				case GenreLexeme::N64:
				case GenreLexeme::Z64:
				{
					nom_struct = "InfoTypeEntier";
					break;
				}
				case GenreLexeme::REFERENCE:
				case GenreLexeme::POINTEUR:
				{
					nom_struct = "InfoTypePointeur";
					break;
				}
				case GenreLexeme::CHAINE_CARACTERE:
				{
					auto const &id_structure = (static_cast<long>(dt_enf.type_base()) & 0xffffff00) >> 8;
					auto &ds = contexte.donnees_structure(id_structure);
					nom_struct = ds.est_enum ? "InfoTypeÉnum" : "InfoTypeStructure";
					break;
				}
				case GenreLexeme::TROIS_POINTS:
				case GenreLexeme::TABLEAU:
				{
					nom_struct = "InfoTypeTableau";
					break;
				}
				case GenreLexeme::COROUT:
				case GenreLexeme::FONC:
				{
					nom_struct = "InfoTypeFonction";
					break;
				}
				case GenreLexeme::EINI:
				case GenreLexeme::NUL: /* À FAIRE */
				case GenreLexeme::RIEN:
				case GenreLexeme::CHAINE:
				{
					nom_struct = "InfoType";
					break;
				}
			}

			auto &ds = contexte.donnees_structure(nom_struct);
			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = contexte.typeuse.type_pointeur_pour(ds.index_type);

			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte, false);

			auto &dt_enfant = trouve_donnees_type(contexte, enfant);
			b->genre_valeur = GenreValeur::TRANSCENDANTALE;
			b->index_type = contexte.typeuse.ajoute_type(dt_enfant.dereference());

			if (dt_enfant.type_base() != GenreLexeme::POINTEUR) {
				erreur::lance_erreur(
							"Un pointeur est requis pour le déréférencement via 'mémoire'",
							contexte,
							enfant->donnees_lexeme(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto nombre_enfant = b->enfants.taille();
			auto enfant = b->enfants.debut();

			b->genre_valeur = GenreValeur::DROITE;
			b->index_type = resoud_type_final(contexte, b->type_declare, false, false);

			auto &dt = trouve_donnees_type(contexte, b);

			if ((dt.type_base() & 0xff) == GenreLexeme::TABLEAU) {
				auto expr = b->type_declare.expressions[0];
				performe_validation_semantique(expr, contexte, false);

				/* transforme en type tableau dynamic */
				auto taille = (static_cast<int>(dt.type_base() >> 8));
				b->drapeaux |= EST_CALCULE;
				b->valeur_calculee = taille;

				auto idx_type_deref = contexte.typeuse.type_dereference_pour(b->index_type);

				// pour la coulisse C, ajout d'une dépendance vers le type du pointeur du tableau
				auto idx_type_pointeur = contexte.typeuse.type_pointeur_pour(idx_type_deref);
				donnees_dependance.types_utilises.insere(idx_type_pointeur);

				b->index_type = contexte.typeuse.type_tableau_pour(idx_type_deref);
			}
			else if (dt.type_base() == GenreLexeme::CHAINE) {
				performe_validation_semantique(*enfant++, contexte, false);
				nombre_enfant -= 1;
			}
			else {
				b->index_type = contexte.typeuse.type_pointeur_pour(b->index_type);
			}

			if (nombre_enfant == 1) {
				performe_validation_semantique(*enfant++, contexte, true);
			}

			donnees_dependance.types_utilises.insere(b->index_type);

			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			b->index_type = resoud_type_final(contexte, b->type_declare, false, false);

			auto &dt = trouve_donnees_type(contexte, b);

			auto nombre_enfant = b->enfants.taille();
			auto enfant = b->enfants.debut();
			auto enfant1 = *enfant++;
			performe_validation_semantique(enfant1, contexte, true);

			if ((dt.type_base() & 0xff) == GenreLexeme::TABLEAU) {
				auto expr = b->type_declare.expressions[0];
				performe_validation_semantique(expr, contexte, false);

				// pour la coulisse C, ajout d'une dépendance vers le type du pointeur du tableau
				auto idx_type_deref = contexte.typeuse.type_dereference_pour(b->index_type);
				auto idx_type_pointeur = contexte.typeuse.type_pointeur_pour(idx_type_deref);
				donnees_dependance.types_utilises.insere(idx_type_pointeur);
			}
			else if (dt.type_base() == GenreLexeme::CHAINE) {
				performe_validation_semantique(*enfant++, contexte, false);
				nombre_enfant -= 1;
			}
			else {
				b->index_type = contexte.typeuse.type_pointeur_pour(b->index_type);
			}

			/* pour les références */
			auto transformation = cherche_transformation(contexte, enfant1->index_type, b->index_type);

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				auto &dt_enf = contexte.typeuse[enfant1->index_type];
				erreur::lance_erreur_type_arguments(
							dt,
							dt_enf,
							contexte,
							b->lexeme,
							enfant1->lexeme);
			}

			enfant1->transformation = transformation;

			if (nombre_enfant == 2) {
				performe_validation_semantique(*enfant++, contexte, true);
			}

			donnees_dependance.types_utilises.insere(b->index_type);

			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte, true);

			auto const &dt = contexte.typeuse[enfant->index_type];
			auto plg_dt = dt.plage();

			if (plg_dt.front() == GenreLexeme::REFERENCE) {
				enfant->transformation = TypeTransformation::DEREFERENCE;
				plg_dt.effronte();
			}

			if (plg_dt.front() != GenreLexeme::POINTEUR && (plg_dt.front() & 0xff) != GenreLexeme::TABLEAU && plg_dt.front() != GenreLexeme::CHAINE) {
				erreur::lance_erreur("Le type n'est pas délogeable", contexte, b->lexeme);
			}

			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto &ds = contexte.donnees_structure(b->chaine());

			if (ds.est_externe && b->enfants.est_vide()) {
				return;
			}

			auto noeud_dependance = graphe.cree_noeud_type(ds.index_type);
			noeud_dependance->noeud_syntactique = ds.noeud_decl;

			auto verifie_inclusion_valeur = [&ds, &contexte](base *enf)
			{
				if (enf->index_type == ds.index_type) {
					erreur::lance_erreur(
								"Ne peut inclure la structure dans elle-même par valeur",
								contexte,
								enf->lexeme,
								erreur::type_erreur::TYPE_ARGUMENT);
				}
				else {
					auto &dt = trouve_donnees_type(contexte, enf);
					auto type_base = dt.type_base();

					if ((type_base & 0xff) == GenreLexeme::TABLEAU && type_base != GenreLexeme::TABLEAU) {
						auto dt_deref = dt.dereference();

						if (dt_deref == contexte.typeuse[ds.index_type]) {
							erreur::lance_erreur(
										"Ne peut inclure la structure dans elle-même par valeur",
										contexte,
										enf->lexeme,
										erreur::type_erreur::TYPE_ARGUMENT);
						}
					}
				}
			};

			auto verifie_redefinition_membre = [&ds, &contexte](base *enf)
			{
				if (ds.donnees_membres.trouve(enf->chaine()) != ds.donnees_membres.fin()) {
					erreur::lance_erreur(
								"Redéfinition du membre",
								contexte,
								enf->lexeme,
								erreur::type_erreur::MEMBRE_REDEFINI);
				}
			};

			auto decalage = 0u;
			auto max_alignement = 0u;

			auto ajoute_donnees_membre = [&contexte, &decalage, &ds, &max_alignement, &donnees_dependance](base *enfant, base *expression)
			{
				auto &dt_membre = trouve_donnees_type(contexte, enfant);
				auto align_type = alignement(contexte, dt_membre);
				max_alignement = std::max(align_type, max_alignement);
				auto padding = (align_type - (decalage % align_type)) % align_type;
				decalage += padding;

				ds.donnees_membres.insere({enfant->chaine(), { ds.index_types.taille(), expression, decalage }});
				ds.index_types.pousse(enfant->index_type);

				donnees_dependance.types_utilises.insere(enfant->index_type);

				decalage += taille_octet_type(contexte, dt_membre);
			};

			if (ds.est_union) {
				for (auto enfant : b->enfants) {
					enfant->index_type = resoud_type_final(contexte, enfant->type_declare);

					verifie_redefinition_membre(enfant);
					verifie_inclusion_valeur(enfant);

					ajoute_donnees_membre(enfant, nullptr);
				}

				auto taille_union = 0u;

				for (auto enfant : b->enfants) {
					auto &dt_membre = trouve_donnees_type(contexte, enfant);
					auto taille = taille_octet_type(contexte, dt_membre);

					taille_union = std::max(taille_union, taille);
				}

				/* Pour les unions sûres, il nous faut prendre en compte le
				 * membre supplémentaire. */
				if (!ds.est_nonsur) {
					/* ajoute une marge d'alignement */
					auto padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
					taille_union += padding;

					/* ajoute la taille du membre actif */
					taille_union += static_cast<unsigned>(sizeof(int));

					/* ajoute une marge d'alignement finale */
					padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
					taille_union += padding;
				}

				ds.taille_octet = taille_union;

				graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
				return;
			}

			for (auto enfant : b->enfants) {
				if (enfant->genre != GenreNoeud::DECLARATION_VARIABLE) {
					erreur::lance_erreur(
								"Déclaration inattendue dans la structure",
								contexte,
								enfant->lexeme,
								erreur::type_erreur::NORMAL);
				}

				if (enfant->enfants.taille() == 2) {
					auto decl_membre = enfant->enfants.front();
					auto decl_expr = enfant->enfants.back();

					decl_membre->index_type = resoud_type_final(contexte, decl_membre->type_declare);

					verifie_redefinition_membre(decl_membre);

					performe_validation_semantique(decl_expr, contexte, false);

					if (decl_membre->index_type != decl_expr->index_type) {
						if (decl_membre->index_type == -1l) {
							decl_membre->index_type = decl_expr->index_type;
						}
						else {
							auto transformation = cherche_transformation(
										contexte,
										decl_expr->index_type,
										decl_membre->index_type);

							if (transformation.type == TypeTransformation::IMPOSSIBLE) {
								erreur::lance_erreur_type_arguments(
											contexte.typeuse[decl_membre->index_type],
											contexte.typeuse[decl_expr->index_type],
											contexte,
											decl_membre->lexeme,
											decl_expr->lexeme);
							}

							decl_expr->transformation = transformation;
						}
					}

					verifie_inclusion_valeur(decl_membre);

					ajoute_donnees_membre(decl_membre, decl_expr);
				}
				else {
					enfant->index_type = resoud_type_final(contexte, enfant->type_declare);

					verifie_redefinition_membre(enfant);
					verifie_inclusion_valeur(enfant);

					// À FAIRE : emploi de structures
					// - validation que nous avons une structure
					// - prend note de la hierarchie, notamment pour valider les transtypages
					// - préserve l'emploi dans les données types
					if (enfant->drapeaux & EMPLOYE) {
						auto &dt_enf = contexte.typeuse[enfant->index_type];
						auto &ds_empl = contexte.donnees_structure(static_cast<long>(dt_enf.type_base() >> 8));

						for (auto i = 0; i < ds_empl.index_types.taille(); ++i) {
							for (auto &membre : ds_empl.donnees_membres) {
								auto idx_membre = membre.second.index_membre;

								if (idx_membre != i) {
									continue;
								}

								auto idx_type_membre = ds_empl.index_types[idx_membre];
								auto &dt_membre = contexte.typeuse[idx_type_membre];

								auto align_type = alignement(contexte, dt_membre);
								max_alignement = std::max(align_type, max_alignement);
								auto padding = (align_type - (decalage % align_type)) % align_type;
								decalage += padding;

								ds.donnees_membres.insere({membre.first, { ds.index_types.taille(), nullptr, decalage }});
								ds.index_types.pousse(idx_type_membre);

								donnees_dependance.types_utilises.insere(idx_type_membre);

								decalage += taille_octet_type(contexte, dt_membre);
							}
						}
					}
					else {
						ajoute_donnees_membre(enfant, nullptr);
					}
				}
			}

			auto padding = (max_alignement - (decalage % max_alignement)) % max_alignement;
			decalage += padding;
			ds.taille_octet = decalage;

			graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto &ds = contexte.donnees_structure(b->chaine());
			ds.noeud_decl->index_type = resoud_type_final(contexte, ds.noeud_decl->type_declare);

			auto const est_drapeau = ds.est_drapeau;

			contexte.operateurs.ajoute_operateur_basique_enum(ds.index_type);

			/* À FAIRE : tests */

			auto noms_presents = dls::ensemble<dls::vue_chaine_compacte>();

			auto dernier_res = ResultatExpression();
			/* utilise est_errone pour indiquer que nous sommes à la première valeur */
			dernier_res.est_errone = true;

			contexte.empile_nombre_locales();

			for (auto enfant : b->enfants) {
				if (enfant->genre != GenreNoeud::DECLARATION_VARIABLE) {
					erreur::lance_erreur(
								"Type d'expression inattendu dans l'énum",
								contexte,
								enfant->lexeme);
				}

				auto var = static_cast<base *>(nullptr);
				auto expr = static_cast<base *>(nullptr);

				if (enfant->enfants.taille() == 2) {
					var = enfant->enfants.front();
					expr = enfant->enfants.back();
				}
				else {
					var = enfant;
				}

				auto nom = var->chaine();

				if (noms_presents.trouve(nom) != noms_presents.fin()) {
					erreur::lance_erreur(
								"Rédéfinition de la valeur de l'énum",
								contexte,
								var->lexeme,
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}

				noms_presents.insere(nom);

				auto res = ResultatExpression();

				auto donnees_variables = DonneesVariable{};
				donnees_variables.index_type = ds.noeud_decl->index_type;
				contexte.pousse_locale(nom, donnees_variables);

				if (expr != nullptr) {
					performe_validation_semantique(expr, contexte, false);

					res = evalue_expression(contexte, expr);

					if (res.est_errone) {
						erreur::lance_erreur(
									res.message_erreur,
									contexte,
									res.noeud_erreur->lexeme,
									erreur::type_erreur::VARIABLE_REDEFINIE);
					}
				}
				else {
					if (dernier_res.est_errone) {
						/* première valeur, laisse à zéro si énum normal */
						dernier_res.est_errone = false;

						if (est_drapeau) {
							res.type = type_expression::ENTIER;
							res.entier = 1;
						}
					}
					else {
						if (dernier_res.type == type_expression::ENTIER) {
							if (est_drapeau) {
								res.entier = dernier_res.entier * 2;
							}
							else {
								res.entier = dernier_res.entier + 1;
							}
						}
						else {
							res.reel = dernier_res.reel + 1;
						}
					}
				}

				/* À FAIRE : pas joli */
				auto &dv = contexte.donnees_variable(nom);
				dv.resultat_expression = res;

				auto &donnees_membre = ds.donnees_membres[nom];
				donnees_membre.resultat_expression = res;
				donnees_membre.noeud_decl = var;

				dernier_res = res;
			}

			contexte.depile_nombre_locales();

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			/* TESTS : si énum -> vérifie que toutes les valeurs soient prises
			 * en compte, sauf s'il y a un bloc sinon après. */

			auto nombre_enfant = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;

			performe_validation_semantique(expression, contexte, false);
			auto index_type = expression->index_type;

			auto dt = trouve_donnees_type(contexte, expression).plage();

			if (dt.front() == GenreLexeme::REFERENCE) {
				dt.effronte();
				b->transformation = TypeTransformation::DEREFERENCE;
				index_type = contexte.typeuse.type_dereference_pour(index_type);
			}

			if ((dt.front() & 0xff) == GenreLexeme::CHAINE_CARACTERE) {
				auto id = static_cast<long>(dt.front() >> 8);
				auto &ds = contexte.donnees_structure(id);

				auto membres_rencontres = dls::ensemble<dls::vue_chaine_compacte>();

				auto valide_presence_membres = [&membres_rencontres, &ds, &contexte, &expression]() {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					for (auto const &paire : ds.donnees_membres) {
						if (membres_rencontres.trouve(paire.first) == membres_rencontres.fin()) {
							valeurs_manquantes.insere(paire.first);
						}
					}

					if (valeurs_manquantes.taille() != 0) {
						erreur::valeur_manquante_discr(contexte, expression, valeurs_manquantes);
					}
				};

				if (ds.est_union) {
					if (ds.est_nonsur) {
						erreur::lance_erreur(
									"« discr » ne peut prendre une union nonsûre",
									contexte,
									expression->lexeme);
					}

					b->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;
					auto sinon_rencontre = false;

					for (auto i = 1; i < nombre_enfant; ++i) {
						auto enfant = *iter_enfant++;
						auto expr_paire = enfant->enfants.front();
						auto bloc_paire = enfant->enfants.back();

						if (expr_paire->genre == GenreNoeud::INSTRUCTION_SINON) {
							sinon_rencontre = true;
							performe_validation_semantique(bloc_paire, contexte, true);
							continue;
						}

						/* vérifie que toutes les expressions des paires sont bel et
						 * bien des membres */
						if (expr_paire->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
							erreur::lance_erreur(
										"Attendu une variable membre de l'union nonsûre",
										contexte,
										expr_paire->lexeme);
						}

						auto nom_membre = expr_paire->chaine();

						if (membres_rencontres.trouve(nom_membre) != membres_rencontres.fin()) {
							erreur::lance_erreur(
										"Redéfinition de l'expression",
										contexte,
										expr_paire->lexeme);
						}

						membres_rencontres.insere(nom_membre);

						auto iter_membre = ds.donnees_membres.trouve(nom_membre);

						if (iter_membre == ds.donnees_membres.fin()) {
							erreur::membre_inconnu(contexte, ds, b, expression, expr_paire);
						}

						contexte.renseigne_membre_actif(expression->chaine(), nom_membre);

						/* Pousse la variable comme étant employée, puisque nous savons ce qu'elle est */
						if (contexte.locale_existe(iter_membre->first)) {
							erreur::lance_erreur(
										"Ne peut pas utiliser implicitement le membre car une variable de ce nom existe déjà",
										contexte,
										expr_paire->lexeme);
						}

						auto donnees_var = DonneesVariable{};
						donnees_var.index_type = ds.index_types[iter_membre->second.index_membre];
						donnees_var.est_argument = true;
						donnees_var.est_membre_emploie = true;
						/* À FAIRE : est_dynamique */

						contexte.empile_nombre_locales();
						contexte.pousse_locale(iter_membre->first, donnees_var);

						performe_validation_semantique(bloc_paire, contexte, true);

						contexte.depile_nombre_locales();
					}

					if (!sinon_rencontre) {
						valide_presence_membres();
					}

					return;
				}

				if (ds.est_enum) {
					b->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;
					auto sinon_rencontre = false;

					for (auto i = 1; i < nombre_enfant; ++i) {
						auto enfant = *iter_enfant++;
						auto expr_paire = enfant->enfants.front();
						auto bloc_paire = enfant->enfants.back();

						if (expr_paire->genre == GenreNoeud::INSTRUCTION_SINON) {
							sinon_rencontre = true;
							performe_validation_semantique(bloc_paire, contexte, true);
							continue;
						}

						auto feuilles = dls::tableau<base *>();
						rassemble_feuilles(expr_paire, feuilles);

						for (auto f : feuilles) {
							auto nom_membre = f->chaine();

							auto iter_membre = ds.donnees_membres.trouve(nom_membre);

							if (iter_membre == ds.donnees_membres.fin()) {
								erreur::membre_inconnu(contexte, ds, b, expression, expr_paire);
							}

							if (membres_rencontres.trouve(nom_membre) != membres_rencontres.fin()) {
								erreur::lance_erreur(
											"Redéfinition de l'expression",
											contexte,
											f->lexeme);
							}

							membres_rencontres.insere(nom_membre);
						}

						performe_validation_semantique(bloc_paire, contexte, true);
					}

					if (!sinon_rencontre) {
						valide_presence_membres();
					}

					return;
				}
			}

			auto candidats = cherche_candidats_operateurs(contexte, index_type, index_type, GenreLexeme::EGALITE);
			auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
			auto poids = 0.0;

			for (auto const &candidat : candidats) {
				if (candidat.poids > poids) {
					poids = candidat.poids;
					meilleur_candidat = &candidat;
				}
			}

			if (meilleur_candidat == nullptr) {
				erreur::lance_erreur_type_operation(contexte, b);
			}

			b->op = meilleur_candidat->op;

			if (!b->op->est_basique) {
				donnees_dependance.fonctions_utilisees.insere(b->op->nom_fonction);
			}

			for (auto i = 1; i < nombre_enfant; ++i) {
				auto enfant = *iter_enfant++;
				assert(enfant->genre == GenreNoeud::INSTRUCTION_PAIRE_DISCR);

				auto expr_paire = enfant->enfants.front();
				auto bloc_paire = enfant->enfants.back();

				performe_validation_semantique(bloc_paire, contexte, true);

				if (expr_paire->genre == GenreNoeud::INSTRUCTION_SINON) {
					continue;
				}

				auto feuilles = dls::tableau<base *>();
				rassemble_feuilles(expr_paire, feuilles);

				for (auto f : feuilles) {
					performe_validation_semantique(f, contexte, true);

					if (f->index_type != expression->index_type) {
						erreur::lance_erreur_type_arguments(
									contexte.typeuse[expression->index_type],
								contexte.typeuse[f->index_type],
								contexte,
								f->lexeme,
								expression->lexeme);
					}
				}
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_PAIRE_DISCR:
		{
			/* RÀF : pris en charge plus haut */
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			if (!fonction_courante->est_coroutine) {
				erreur::lance_erreur(
							"'retiens' hors d'une coroutine",
							contexte,
							b->lexeme);
			}

			valides_enfants(b, contexte, false);

			auto enfant = b->enfants.front();

			/* À FAIRE : multiple types retours. */
			auto idx_type_retour = fonction_courante->idx_types_retours[0];			
			auto transformation = cherche_transformation(contexte, enfant->index_type, idx_type_retour);

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				auto const &dt_arg = contexte.typeuse[idx_type_retour];
				auto const &dt_enf = trouve_donnees_type(contexte, enfant);
				erreur::lance_erreur_type_retour(
							dt_arg,
							dt_enf,
							contexte,
							enfant->lexeme,
							b->lexeme);
			}

			enfant->transformation = transformation;

			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			valides_enfants(b, contexte, expr_gauche);
			b->index_type = b->enfants.front()->index_type;
			b->genre_valeur = b->enfants.front()->genre_valeur;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto variable = b->enfants.front();

			auto type_symbole = cherche_symbole(contexte, variable->chaine());

			if (type_symbole == SYMBOLE_INCONNU) {
				erreur::lance_erreur("variable inconnu", contexte, variable->lexeme);
			}

			auto bloc = b->enfants.back();
			performe_validation_semantique(bloc, contexte, true);

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			performe_validation_semantique(b->enfants.front(), contexte, expr_gauche);
			b->index_type = b->enfants.front()->index_type;
			break;
		}
	}
}

void performe_validation_semantique(
		assembleuse_arbre const &arbre,
		ContexteGenerationCode &contexte)
{
	auto racine = arbre.racine();

	if (racine == nullptr) {
		return;
	}

	if (racine->genre != GenreNoeud::RACINE) {
		return;
	}

	auto debut_validation = dls::chrono::compte_seconde();

	/* valide d'abord les types de fonctions afin de résoudre les fonctions
	 * appelées dans le cas de fonctions mutuellement récursives */
	for (auto noeud : racine->enfants) {
		if (noeud->genre == GenreNoeud::DECLARATION_COROUTINE || noeud->genre == GenreNoeud::DECLARATION_FONCTION) {
			valide_type_fonction(noeud, contexte);
		}
	}

	for (auto noeud : racine->enfants) {
		performe_validation_semantique(noeud, contexte, true);
	}

	contexte.temps_validation = debut_validation.temps();
}

}  /* namespace noeud */
