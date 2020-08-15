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

#include "noeud_code.hh"

#include "arbre_syntaxique.hh"
#include "compilatrice.hh"
#include "identifiant.hh"

NoeudCode *ConvertisseuseNoeudCode::converti_noeud_syntaxique(EspaceDeTravail *espace, NoeudExpression *noeud_expression)
{
	auto noeud_code = static_cast<NoeudCode *>(nullptr);

	if (noeud_expression == nullptr) {
		return nullptr;
	}

	if (noeud_expression->noeud_code != nullptr) {
		return noeud_expression->noeud_code;
	}

	switch (noeud_expression->genre) {
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		{
			auto decl = noeud_expression->comme_entete_fonction();

			auto n = noeuds_entetes_fonctions.ajoute_element();
			n->params_entree.reserve(decl->params.taille);

			n->nom.pointeur = const_cast<char *>(decl->lexeme->chaine.pointeur());
			n->nom.taille = decl->lexeme->chaine.taille();

			POUR (decl->params) {
				auto n_param = converti_noeud_syntaxique(espace, it);
				n->params_entree.pousse(static_cast<NoeudCodeDeclaration *>(n_param));
			}

			n->est_coroutine = decl->est_coroutine;
			n->est_operateur = decl->est_operateur;

			noeud_code = n;
			break;
		}
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			auto decl = static_cast<NoeudDeclarationCorpsFonction *>(noeud_expression);

			auto n = noeuds_corps_fonctions.ajoute_element();
			n->bloc = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(espace, decl->bloc));
			n->entete = static_cast<NoeudCodeEnteteFonction *>(converti_noeud_syntaxique(espace, decl->entete));

			noeud_code = n;
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto decl = static_cast<NoeudEnum *>(noeud_expression);

			auto n = noeuds_codes.ajoute_element();
			n->info_type = cree_info_type_pour(decl->type);

			noeud_code = n;
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct *>(noeud_expression);

			auto n = noeuds_codes.ajoute_element();
			n->info_type = cree_info_type_pour(decl->type);

			noeud_code = n;
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto decl = static_cast<NoeudDeclarationVariable *>(noeud_expression);

			auto n = noeuds_declarations.ajoute_element();
			n->nom.pointeur = const_cast<char *>(decl->ident->nom.pointeur());
			n->nom.taille = decl->ident->nom.taille();

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto noeud_bloc = static_cast<NoeudBloc *>(noeud_expression);

			auto expressions = noeud_bloc->expressions.verrou_lecture();

			auto n = noeuds_blocs.ajoute_element();
			n->expressions.reserve(expressions->taille);

			POUR (*expressions) {
				n->expressions.pousse(converti_noeud_syntaxique(espace, it));
			}

			auto membres = noeud_bloc->membres.verrou_lecture();
			n->membres.reserve(membres->taille);

			POUR (*membres) {
				// le noeud_code peut être nul pour le contexte implicite
				if (it->noeud_code == nullptr) {
					converti_noeud_syntaxique(espace, it);
				}

				n->membres.pousse(static_cast<NoeudCodeDeclaration *>(it->noeud_code));
			}

			noeud_code = n;
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto noeud_op_bin = static_cast<NoeudExpressionBinaire *>(noeud_expression);

			auto n = noeuds_operations_binaire.ajoute_element();
			n->operande_gauche = converti_noeud_syntaxique(espace, noeud_op_bin->expr1);
			n->operande_droite = converti_noeud_syntaxique(espace, noeud_op_bin->expr2);

			noeud_code = n;
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto noeud_reference_membre = static_cast<NoeudExpressionMembre *>(noeud_expression);

			auto n = noeuds_reference_membre.ajoute_element();
			n->accede = converti_noeud_syntaxique(espace, noeud_reference_membre->accede);
			n->membre = converti_noeud_syntaxique(espace, noeud_reference_membre->membre);

			noeud_code = n;
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto noeud_logement = static_cast<NoeudExpressionLogement *>(noeud_expression);

			auto n = noeuds_logements.ajoute_element();
			n->expr = converti_noeud_syntaxique(espace, noeud_logement->expr);
			n->expr_taille = converti_noeud_syntaxique(espace, noeud_logement->expr_taille);
			n->bloc = converti_noeud_syntaxique(espace, noeud_logement->bloc);

			noeud_code = n;
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto noeud_appel = static_cast<NoeudExpressionAppel *>(noeud_expression);

			auto n = noeuds_appel.ajoute_element();
			n->expression = converti_noeud_syntaxique(espace, noeud_appel->appelee);
			n->params.reserve(noeud_appel->params.taille);

			POUR (noeud_appel->params) {
				n->params.pousse(converti_noeud_syntaxique(espace, it));
			}

			noeud_code = n;
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::INSTRUCTION_RETIENS:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud_expression);

			auto n = noeuds_operations_unaire.ajoute_element();
			n->operande = converti_noeud_syntaxique(espace, expr->expr);

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto noeud_si = static_cast<NoeudSi *>(noeud_expression);

			auto n = noeuds_sis.ajoute_element();
			n->condition = converti_noeud_syntaxique(espace, noeud_si->condition);
			n->bloc_si_vrai = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(espace, noeud_si->bloc_si_vrai));
			n->bloc_si_faux = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(espace, noeud_si->bloc_si_faux));

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto noeud_si_statique = noeud_expression->comme_si_statique();

			auto n = noeuds_sis.ajoute_element();
			n->condition = converti_noeud_syntaxique(espace, noeud_si_statique->condition);
			n->bloc_si_vrai = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(espace, noeud_si_statique->bloc_si_vrai));
			n->bloc_si_faux = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(espace, noeud_si_statique->bloc_si_faux));

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto noeud_boucle = static_cast<NoeudBoucle *>(noeud_expression);

			auto n = noeuds_boucles.ajoute_element();
			n->condition = converti_noeud_syntaxique(espace, noeud_boucle->condition);
			n->bloc = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(espace, noeud_boucle->bloc));

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			noeud_code = noeuds_codes.ajoute_element();
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto noeud_pour = noeud_expression->comme_pour();

			auto n = noeuds_pour.ajoute_element();
			n->variable = converti_noeud_syntaxique(espace, noeud_pour->variable);
			n->expression = converti_noeud_syntaxique(espace, noeud_pour->expression);
			n->bloc = converti_noeud_syntaxique(espace, noeud_pour->bloc);
			n->bloc_sansarret = converti_noeud_syntaxique(espace, noeud_pour->bloc_sansarret);
			n->bloc_sinon = converti_noeud_syntaxique(espace, noeud_pour->bloc_sinon);

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto noeud_discr = static_cast<NoeudDiscr *>(noeud_expression);

			auto n = noeuds_discr.ajoute_element();
			n->expression = converti_noeud_syntaxique(espace, noeud_discr->expr);
			n->bloc_sinon = converti_noeud_syntaxique(espace, noeud_discr->bloc_sinon);
			n->paires_discr.reserve(noeud_discr->paires_discr.taille);

			POUR (noeud_discr->paires_discr) {
				auto expr = converti_noeud_syntaxique(espace, it.first);
				auto bloc = converti_noeud_syntaxique(espace, it.second);

				n->paires_discr.pousse({ expr, bloc });
			}

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto noeud_pc = noeud_expression->comme_pousse_contexte();

			auto n = noeuds_pousse_contexte.ajoute_element();
			n->expression = converti_noeud_syntaxique(espace, noeud_pc->expr);
			n->bloc = converti_noeud_syntaxique(espace, noeud_pc->bloc);

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto noeud_tente = noeud_expression->comme_tente();

			auto n = noeuds_tente.ajoute_element();
			n->expression_appel = converti_noeud_syntaxique(espace, noeud_tente->expr_appel);
			n->expression_piege = converti_noeud_syntaxique(espace, noeud_tente->expr_piege);
			n->bloc = converti_noeud_syntaxique(espace, noeud_tente->bloc);

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_EMPL:
		{
			auto noeud_empl = noeud_expression->comme_empl();

			auto n = noeuds_operations_unaire.ajoute_element();
			n->operande = converti_noeud_syntaxique(espace, noeud_empl->expr);

			noeud_code = n;
			break;
		}
	}

	// À FAIRE : supprime cela quand nous gérerons tous les cas
	if (noeud_code) {
		noeud_code->genre = static_cast<int>(noeud_expression->genre);

		if (noeud_expression->type) {
			noeud_code->info_type = cree_info_type_pour(noeud_expression->type);
		}

		auto lexeme = noeud_expression->lexeme;

		// lexeme peut-être nul pour les blocs
		if (lexeme) {
			auto fichier = espace->fichier(lexeme->fichier);

			noeud_code->chemin_fichier = fichier->chemin;
			noeud_code->nom_fichier = fichier->nom;
			noeud_code->numero_ligne = lexeme->ligne;
			noeud_code->numero_colonne = lexeme->colonne;
		}
	}

	noeud_expression->noeud_code = noeud_code;

	return noeud_code;
}

InfoType *ConvertisseuseNoeudCode::cree_info_type_pour(Type *type)
{
	auto cree_info_type_entier = [this](uint taille_en_octet, bool est_signe)
	{
		auto info_type = infos_types_entiers.ajoute_element();
		info_type->genre = GenreInfoType::ENTIER;
		info_type->taille_en_octet = taille_en_octet;
		info_type->est_signe = est_signe;

		return info_type;
	};

	// À FAIRE : il est possible que les types ne soient pas encore validé quand nous générons des messages pour les entêtes de fonctions
	if (type == nullptr) {
		return nullptr;
	}

	if (type->info_type != nullptr) {
		return type->info_type;
	}

	switch (type->genre) {
		case GenreType::INVALIDE:
		case GenreType::POLYMORPHIQUE:
		{
			return nullptr;
		}
		case GenreType::OCTET:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::OCTET;
			info_type->taille_en_octet = type->taille_octet;

			type->info_type = info_type;
			break;
		}
		case GenreType::BOOL:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::BOOLEEN;
			info_type->taille_en_octet = type->taille_octet;

			type->info_type = info_type;
			break;
		}
		case GenreType::CHAINE:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::CHAINE;
			info_type->taille_en_octet = type->taille_octet;

			type->info_type = info_type;
			break;
		}
		case GenreType::EINI:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::EINI;
			info_type->taille_en_octet = type->taille_octet;

			type->info_type = info_type;
			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_tableau = type->comme_tableau_dynamique();

			auto info_type = infos_types_tableaux.ajoute_element();
			info_type->genre = GenreInfoType::TABLEAU;
			info_type->taille_en_octet = type->taille_octet;
			info_type->est_tableau_fixe = false;
			info_type->taille_fixe = 0;
			info_type->type_pointe = cree_info_type_pour(type_tableau->type_pointe);

			type->info_type = info_type;
			break;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_variadique = type->comme_variadique();

			auto info_type = infos_types_tableaux.ajoute_element();
			info_type->genre = GenreInfoType::TABLEAU;
			info_type->taille_en_octet = type->taille_octet;
			info_type->est_tableau_fixe = false;
			info_type->taille_fixe = 0;

			// type nul pour les types variadiques des fonctions externes (p.e. printf(const char *, ...))
			if (type_variadique->type_pointe) {
				info_type->type_pointe = cree_info_type_pour(type_variadique->type_pointe);
			}

			type->info_type = info_type;
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tableau = type->comme_tableau_fixe();

			auto info_type = infos_types_tableaux.ajoute_element();
			info_type->genre = GenreInfoType::TABLEAU;
			info_type->taille_en_octet = type->taille_octet;
			info_type->est_tableau_fixe = true;
			info_type->taille_fixe = static_cast<int>(type_tableau->taille);
			info_type->type_pointe = cree_info_type_pour(type_tableau->type_pointe);

			type->info_type = info_type;
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			type->info_type = cree_info_type_entier(4, true);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			type->info_type = cree_info_type_entier(type->taille_octet, false);
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			type->info_type = cree_info_type_entier(type->taille_octet, true);
			break;
		}
		case GenreType::REEL:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::REEL;
			info_type->taille_en_octet = type->taille_octet;

			type->info_type = info_type;
			break;
		}
		case GenreType::RIEN:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::RIEN;
			info_type->taille_en_octet = 0;

			type->info_type = info_type;
			break;
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			auto info_type = infos_types.ajoute_element();
			info_type->genre = GenreInfoType::TYPE_DE_DONNEES;
			info_type->taille_en_octet = type->taille_octet;

			type->info_type = info_type;
			break;
		}
		case GenreType::POINTEUR:
		case GenreType::REFERENCE:
		{
			auto info_type = infos_types_pointeurs.ajoute_element();
			info_type->genre = GenreInfoType::POINTEUR;
			info_type->type_pointe = cree_info_type_pour(type_dereference_pour(type));
			info_type->taille_en_octet = type->taille_octet;
			info_type->est_reference = type->genre == GenreType::REFERENCE;

			type->info_type = info_type;
			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = type->comme_structure();

			auto info_type = infos_types_structures.ajoute_element();
			type->info_type = info_type;

			info_type->genre = GenreInfoType::STRUCTURE;
			info_type->taille_en_octet = type->taille_octet;
			info_type->nom = type_struct->nom;

			info_type->membres.reserve(type_struct->membres.taille);

			POUR (type_struct->membres) {
				auto info_type_membre = infos_types_membres_structures.ajoute_element();
				info_type_membre->info = cree_info_type_pour(it.type);
				info_type_membre->decalage = it.decalage;
				info_type_membre->nom = it.nom;
				info_type_membre->drapeaux = it.drapeaux;

				info_type->membres.pousse(info_type_membre);
			}

			break;
		}
		case GenreType::UNION:
		{
			auto type_union = type->comme_union();

			auto info_type = infos_types_unions.ajoute_element();
			info_type->genre = GenreInfoType::UNION;
			info_type->est_sure = !type_union->est_nonsure;
			info_type->type_le_plus_grand = cree_info_type_pour(type_union->type_le_plus_grand);
			info_type->decalage_index = type_union->decalage_index;
			info_type->taille_en_octet = type_union->taille_octet;

			info_type->membres.reserve(type_union->membres.taille);

			POUR (type_union->membres) {
				auto info_type_membre = infos_types_membres_structures.ajoute_element();
				info_type_membre->info = cree_info_type_pour(it.type);
				info_type_membre->decalage = it.decalage;
				info_type_membre->nom = it.nom;
				info_type_membre->drapeaux = it.drapeaux;

				info_type->membres.pousse(info_type_membre);
			}

			type->info_type = info_type;
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = type->comme_enum();

			auto info_type = infos_types_enums.ajoute_element();
			info_type->genre = GenreInfoType::ENUM;
			info_type->nom = type_enum->nom;
			info_type->est_drapeau = type_enum->est_drapeau;
			info_type->taille_en_octet = type_enum->taille_octet;

			info_type->noms.reserve(type_enum->membres.taille);
			info_type->valeurs.reserve(type_enum->membres.taille);

			POUR (type_enum->membres) {
				info_type->noms.pousse(it.nom);
				info_type->valeurs.pousse(it.valeur);
			}

			type->info_type = info_type;
			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonction = type->comme_fonction();

			auto info_type = infos_types_fonctions.ajoute_element();
			info_type->genre = GenreInfoType::FONCTION;
			info_type->est_coroutine = false;
			info_type->taille_en_octet = type->taille_octet;

			info_type->types_entrees.reserve(type_fonction->types_entrees.taille);

			POUR (type_fonction->types_entrees) {
				info_type->types_entrees.pousse(cree_info_type_pour(it));
			}

			info_type->types_sorties.reserve(type_fonction->types_sorties.taille);

			POUR (type_fonction->types_sorties) {
				info_type->types_sorties.pousse(cree_info_type_pour(it));
			}

			type->info_type = info_type;
			break;
		}
	}

	return type->info_type;
}

long ConvertisseuseNoeudCode::memoire_utilisee() const
{
	auto memoire = 0l;

	memoire += noeuds_codes.memoire_utilisee();
	memoire += noeuds_entetes_fonctions.memoire_utilisee();
	memoire += noeuds_corps_fonctions.memoire_utilisee();
	memoire += noeuds_assignations.memoire_utilisee();
	memoire += noeuds_declarations.memoire_utilisee();
	memoire += noeuds_operations_unaire.memoire_utilisee();
	memoire += noeuds_operations_binaire.memoire_utilisee();
	memoire += noeuds_blocs.memoire_utilisee();
	memoire += noeuds_sis.memoire_utilisee();
	memoire += noeuds_boucles.memoire_utilisee();
	memoire += noeuds_pour.memoire_utilisee();
	memoire += noeuds_tente.memoire_utilisee();
	memoire += noeuds_discr.memoire_utilisee();
	memoire += noeuds_pousse_contexte.memoire_utilisee();
	memoire += noeuds_reference_membre.memoire_utilisee();
	memoire += noeuds_logements.memoire_utilisee();
	memoire += noeuds_appel.memoire_utilisee();

	memoire += infos_types.memoire_utilisee();
	memoire += infos_types_entiers.memoire_utilisee();
	memoire += infos_types_enums.memoire_utilisee();
	memoire += infos_types_fonctions.memoire_utilisee();
	memoire += infos_types_membres_structures.memoire_utilisee();
	memoire += infos_types_pointeurs.memoire_utilisee();
	memoire += infos_types_structures.memoire_utilisee();
	memoire += infos_types_tableaux.memoire_utilisee();
	memoire += infos_types_unions.memoire_utilisee();

	return memoire;
}
