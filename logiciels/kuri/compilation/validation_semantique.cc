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
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "erreur.h"
#include "outils_lexemes.hh"
#include "profilage.hh"
#include "portee.hh"
#include "validation_expression_appel.hh"

using dls::outils::possede_drapeau;

/* ************************************************************************** */

#define VERIFIE_INTERFACE_KURI_CHARGEE(nom) \
	if (m_compilatrice.interface_kuri.nom == nullptr) {\
		unite->attend_sur_interface_kuri(); \
		return true; \
	}

ContexteValidationCode::ContexteValidationCode(Compilatrice &compilatrice)
	: m_compilatrice(compilatrice)
{}

void ContexteValidationCode::commence_fonction(NoeudDeclarationFonction *fonction)
{
	donnees_dependance.types_utilises.efface();
	donnees_dependance.fonctions_utilisees.efface();
	donnees_dependance.globales_utilisees.efface();
	fonction_courante = fonction;
}

void ContexteValidationCode::termine_fonction()
{
	fonction_courante = nullptr;
}

dls::vue_chaine_compacte ContexteValidationCode::trouve_membre_actif(const dls::vue_chaine_compacte &nom_union)
{
	for (auto const &paire : membres_actifs) {
		if (paire.first == nom_union) {
			return paire.second;
		}
	}

	return "";
}

void ContexteValidationCode::renseigne_membre_actif(const dls::vue_chaine_compacte &nom_union, const dls::vue_chaine_compacte &nom_membre)
{
	for (auto &paire : membres_actifs) {
		if (paire.first == nom_union) {
			paire.second = nom_membre;
			return;
		}
	}

	membres_actifs.pousse({ nom_union, nom_membre });
}

bool ContexteValidationCode::valide_semantique_noeud(NoeudExpression *noeud)
{
	PROFILE_FONCTION;

	auto &graphe = m_compilatrice.graphe_dependance;

	switch (noeud->genre) {
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(noeud);

			if (decl->est_declaration_type) {
				POUR (decl->arbre_aplatis_entete) {
					// voir commentaire plus bas
					if (it->genre == GenreNoeud::DECLARATION_VARIABLE) {
						auto valeur = static_cast<NoeudDeclarationVariable *>(it)->valeur;
						if (valide_semantique_noeud(valeur)) {
							return true;
						}
					}
					else {
						if (valide_semantique_noeud(it)) {
							return true;
						}
					}
				}

				auto requiers_contexte = !possede_drapeau(decl->drapeaux, FORCE_NULCTX);
				auto types_entrees = kuri::tableau<Type *>(decl->params.taille + requiers_contexte);

				if (requiers_contexte) {
					types_entrees[0] = m_compilatrice.type_contexte;
				}

				for (auto i = 0; i < decl->params.taille; ++i) {
					// le syntaxage des expressions des entrées des fonctions est commune
					// aux déclarations des fonctions et des types de fonctions faisant
					// qu'une chaine de caractère seule (référence d'un type) est considérée
					// comme étant une déclaration de variable, il nous faut donc extraire
					// le noeud de référence à la variable afin de valider correctement le type
					NoeudExpression *type_entree = decl->params[i];

					if (type_entree->genre == GenreNoeud::DECLARATION_VARIABLE) {
						type_entree = static_cast<NoeudDeclarationVariable *>(type_entree)->valeur;
					}

					if (resoud_type_final(type_entree, types_entrees[i + requiers_contexte])) {
						return true;
					}
				}

				auto types_sorties = kuri::tableau<Type *>(decl->params_sorties.taille);

				for (auto i = 0; i < decl->params_sorties.taille; ++i) {
					if (resoud_type_final(decl->params_sorties[i], types_sorties[i])) {
						return true;
					}
				}

				auto type_fonction = m_compilatrice.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
				decl->type = m_compilatrice.typeuse.type_type_de_donnees(type_fonction);
				return false;
			}

			return valide_fonction(decl);
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			return valide_operateur(static_cast<NoeudDeclarationFonction *>(noeud));
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(noeud);
			expr->genre_valeur = GenreValeur::DROITE;
			// @réinitialise en cas d'erreurs passées
			expr->exprs = kuri::tableau<NoeudExpression *>();
			return valide_appel_fonction(m_compilatrice, *this, expr);
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto noeud_directive = static_cast<NoeudDirectiveExecution *>(noeud);

			// crée une fonction pour l'exécution
			auto noeud_decl = static_cast<NoeudDeclarationFonction *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_FONCTION, noeud->lexeme));
			noeud_decl->bloc_parent = noeud->bloc_parent;

			noeud_decl->nom_broye = "metaprogamme" + dls::vers_chaine(noeud_directive);

			// le type de la fonction est (contexte) -> (type_expression)
			auto types_entrees = kuri::tableau<Type *>(1);
			types_entrees[0] = m_compilatrice.type_contexte;

			auto types_sorties = kuri::tableau<Type *>(1);
			types_sorties[0] = noeud_directive->expr->type;

			auto type_fonction = m_compilatrice.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
			noeud_decl->type = type_fonction;

			noeud_decl->bloc = static_cast<NoeudBloc *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, noeud->lexeme));
			noeud_decl->bloc->expressions.pousse(noeud_directive->expr);
			// À FAIRE : instruction de retour

			auto noeud_dep = m_compilatrice.graphe_dependance.cree_noeud_fonction(noeud_decl);
			m_compilatrice.graphe_dependance.ajoute_dependances(*noeud_dep, donnees_dependance);

			noeud_directive->fonction = noeud_decl;
			noeud_decl->drapeaux |= DECLARATION_FUT_VALIDEE;

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto expr = static_cast<NoeudExpressionReference *>(noeud);

			if (expr->drapeaux & DECLARATION_TYPE_POLYMORPHIQUE) {
				expr->genre_valeur = GenreValeur::DROITE;

				if (fonction_courante && fonction_courante->est_instantiation_gabarit) {
					auto type_instantie = static_cast<Type *>(nullptr);

					for (auto &paire : fonction_courante->paires_expansion_gabarit) {
						if (paire.first == expr->ident->nom) {
							type_instantie = paire.second;
						}
					}

					if (type_instantie == nullptr) {
						rapporte_erreur("impossible de définir le type de l'instantiation polymorphique", expr);
						return true;
					}

					expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_instantie);
					return false;
				}

				auto type_poly = m_compilatrice.typeuse.cree_polymorphique(expr->ident);
				expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_poly);
				return false;
			}

			expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto bloc = expr->bloc_parent;
			assert(bloc != nullptr);

			/* À FAIRE : pour une fonction, trouve la selon le type */
			auto fichier = m_compilatrice.fichier(static_cast<size_t>(expr->lexeme->fichier));
			auto decl = trouve_dans_bloc_ou_module(m_compilatrice, bloc, expr->ident, fichier);

			if (decl == nullptr) {
				unite->attend_sur_symbole(expr->lexeme);
				// À FAIRE(erreur) : détermine comment proprement lancer l'erreur (si la compilation n'est pas terminée)
				//rapporte_erreur("Variable inconnue", noeud, erreur::type_erreur::VARIABLE_INCONNUE);
				return true;
			}

			if (decl->lexeme->fichier == expr->lexeme->fichier && decl->genre == GenreNoeud::DECLARATION_VARIABLE && ((decl->drapeaux_decl & EST_GLOBALE) == 0)) {
				if (decl->lexeme->ligne > expr->lexeme->ligne) {
					rapporte_erreur("Utilisation d'une variable avant sa déclaration", expr);
					return true;
				}
			}

			if (dls::outils::est_element(decl->genre, GenreNoeud::DECLARATION_ENUM, GenreNoeud::DECLARATION_STRUCTURE) && expr->aide_generation_code != EST_NOEUD_ACCES) {
				expr->type = m_compilatrice.typeuse.type_type_de_donnees(decl->type);
			}
			else {				
				if ((decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
					unite->attend_sur_declaration(decl);
					return true;
				}

				// les fonctions peuvent ne pas avoir de type au moment si elles sont des appels polymorphiques
				assert(decl->type || decl->genre == GenreNoeud::DECLARATION_FONCTION);
				expr->decl = decl;
				expr->type = decl->type;
			}

			if (decl->drapeaux & EST_VAR_BOUCLE) {
				expr->drapeaux |= EST_VAR_BOUCLE;
			}
			else if (decl->drapeaux & EST_CONSTANTE) {
				expr->genre_valeur = GenreValeur::DROITE;
			}

			// uniquement pour les fonctions polymorphiques
			if (expr->type) {
				donnees_dependance.types_utilises.insere(expr->type);
			}

			if (decl->genre == GenreNoeud::DECLARATION_FONCTION && !static_cast<NoeudDeclarationFonction *>(decl)->est_gabarit) {
				auto decl_fonc = static_cast<NoeudDeclarationFonction *>(decl);
				donnees_dependance.fonctions_utilisees.insere(decl_fonc);
			}
			else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (decl->drapeaux & EST_GLOBALE) {
					donnees_dependance.globales_utilisees.insere(static_cast<NoeudDeclarationVariable *>(decl));
				}
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			auto type_connu = m_compilatrice.typeuse.type_pour_lexeme(noeud->lexeme->genre);
			auto type_type = m_compilatrice.typeuse.type_type_de_donnees(type_connu);
			noeud->type = type_type;
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto inst = static_cast<NoeudExpressionMembre *>(noeud);
			auto enfant1 = inst->accede;
			//auto enfant2 = inst->membre;
			noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;

			if (enfant1->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto fichier = m_compilatrice.fichier(static_cast<size_t>(noeud->lexeme->fichier));

				auto const nom_symbole = enfant1->ident->nom;
				if (fichier->importe_module(nom_symbole)) {
					/* À FAIRE(réusinage arbre) */
//					auto module_importe = m_compilatrice.module(nom_symbole);

//					if (module_importe == nullptr) {
//						rapporte_erreur(
//									"module inconnu",
//									compilatrice,
//									enfant1->lexeme,
//									erreur::type_erreur::MODULE_INCONNU);
//					}

//					auto const nom_fonction = enfant2->ident->nom;

//					if (!module_importe->possede_fonction(nom_fonction)) {
//						rapporte_erreur(
//									"Le module ne possède pas la fonction",
//									compilatrice,
//									enfant2->lexeme,
//									erreur::type_erreur::FONCTION_INCONNUE);
//					}

//					enfant2->module_appel = static_cast<int>(module_importe->id);

//					performe_validation_semantique(enfant2, m_compilatrice);

//					noeud->type = enfant2->type;
//					noeud->aide_generation_code = ACCEDE_MODULE;

					return false;
				}
			}

			if (valide_acces_membre(inst)) {
				return true;
			}

			donnees_dependance.types_utilises.insere(inst->type);
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(noeud);
			auto variable = inst->expr1;
			auto expression = inst->expr2;

			if (expression->genre == GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
				rapporte_erreur("Impossible d'utiliser '---' dans une expression d'assignation", expression);
				return true;
			}

			if (expression->type == nullptr) {
				rapporte_erreur("Impossible de définir le type de la variable !", noeud, erreur::type_erreur::TYPE_INCONNU);
				return true;
			}

			if (expression->type->genre == GenreType::RIEN) {
				rapporte_erreur("Impossible d'assigner une expression de type 'rien' à une variable !", noeud, erreur::type_erreur::ASSIGNATION_RIEN);
				return true;
			}

			/* a, b = foo() */
			if (variable->lexeme->genre == GenreLexeme::VIRGULE) {
				if (expression->genre != GenreNoeud::EXPRESSION_APPEL_FONCTION) {
					rapporte_erreur("Une virgule ne peut se trouver qu'à gauche d'un appel de fonction.", variable, erreur::type_erreur::NORMAL);
					return true;
				}

				dls::tablet<NoeudExpression *, 10> feuilles;
				rassemble_feuilles(variable, feuilles);

				/* Utilisation du type de la fonction et non
				 * DonneesFonction::idx_types_retour car les pointeurs de
				 * fonctions n'ont pas de DonneesFonction. */
				auto type_fonc = static_cast<TypeFonction *>(expression->type);

				if (feuilles.taille() != type_fonc->types_sorties.taille) {
					rapporte_erreur("L'ignorance d'une valeur de retour non implémentée.", variable, erreur::type_erreur::NORMAL);
					return true;
				}

				for (auto i = 0l; i < feuilles.taille(); ++i) {
					auto &f = feuilles[i];

					if (f->type == nullptr) {
						f->type = type_fonc->types_sorties[i];
					}
				}

				return false;
			}

			if (!est_valeur_gauche(variable->genre_valeur)) {
				rapporte_erreur("Impossible d'assigner une expression à une valeur-droite !", noeud, erreur::type_erreur::ASSIGNATION_INVALIDE);
				return true;
			}

			auto transformation = TransformationType();
			if (cherche_transformation(m_compilatrice, *this, expression->type, variable->type, transformation)) {
				return true;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				rapporte_erreur_assignation_type_differents(variable->type, expression->type, noeud);
				return true;
			}

			expression->transformation = transformation;

			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto decl = static_cast<NoeudDeclarationVariable *>(noeud);
			auto variable = decl->valeur;
			auto expression = decl->expression;

			// À FAIRE : cas où nous avons plusieurs variables déclarées
			if (variable->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				rapporte_erreur("Expression inattendue à gauche de la déclaration", variable);
				return true;
			}

			decl->ident = variable->ident;

			auto decl_prec = trouve_dans_bloc(variable->bloc_parent, decl);

			if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
				if (decl->lexeme->ligne > decl_prec->lexeme->ligne) {
					rapporte_erreur_redefinition_symbole(variable, decl_prec);
					return true;
				}
			}

			if (resoud_type_final(variable->expression_type, variable->type)) {
				return true;
			}

			if ((decl->drapeaux & EST_CONSTANTE) && expression != nullptr && expression->genre == GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
				rapporte_erreur("Impossible de ne pas initialiser une constante", expression);
				return true;
			}

			if (expression != nullptr && expression->genre != GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
				if (expression->type == nullptr) {
					rapporte_erreur("impossible de définir le type de l'expression", expression);
					return true;
				}
				else if (variable->type == nullptr) {
					if (expression->type->genre == GenreType::ENTIER_CONSTANT) {
						variable->type = m_compilatrice.typeuse[TypeBase::Z32];
						expression->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, variable->type };
					}
					else if (expression->type->genre == GenreType::RIEN) {
						rapporte_erreur("impossible d'assigner une expression de type « rien » à une variable", expression, erreur::type_erreur::ASSIGNATION_RIEN);
						return true;
					}
					else {
						variable->type = expression->type;
					}
				}
				else {
					auto transformation = TransformationType();

					if (cherche_transformation(m_compilatrice, *this, expression->type, variable->type, transformation)) {
						return true;
					}

					if (transformation.type == TypeTransformation::IMPOSSIBLE) {
						rapporte_erreur_assignation_type_differents(variable->type, expression->type, noeud);
						return true;
					}

					expression->transformation = transformation;
				}

				if (decl->drapeaux & EST_CONSTANTE && expression->type->genre != GenreType::TYPE_DE_DONNEES) {
					auto res_exec = evalue_expression(m_compilatrice, decl->bloc_parent, expression);

					if (res_exec.est_errone) {
						rapporte_erreur("Impossible d'évaluer l'expression de la constante", expression);
						return true;
					}

					decl->valeur_expression = res_exec;
				}
			}
			else {
				if (variable->type == nullptr) {
					rapporte_erreur("variable déclarée sans type", variable);
					return true;
				}
			}

			decl->type = variable->type;

			if (decl->drapeaux_decl & EST_EXTERNE) {
				rapporte_erreur("Ne peut pas assigner une variable globale externe dans sa déclaration", noeud);
				return true;
			}

			if (decl->drapeaux & EST_GLOBALE) {
				graphe.cree_noeud_globale(decl);
			}

			if (variable->drapeaux & EMPLOYE) {
				if (decl->type->genre != GenreType::STRUCTURE) {
					rapporte_erreur("impossible d'employé une variable n'étant pas une structure", decl);
					return true;
				}

				if ((decl->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
					unite->attend_sur_type(decl->type);
					return true;
				}

				auto type_structure = static_cast<TypeStructure *>(decl->type);

				POUR (type_structure->membres) {
					auto decl_membre = m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme);
					decl_membre->ident = m_compilatrice.table_identifiants.identifiant_pour_chaine(it.nom);
					decl_membre->type = it.type;
					decl_membre->bloc_parent = decl->bloc_parent;
					decl_membre->drapeaux |= DECLARATION_FUT_VALIDEE;

					decl->bloc_parent->membres.pousse(static_cast<NoeudDeclaration *>(decl_membre));
				}
			}

			decl->drapeaux |= DECLARATION_FUT_VALIDEE;
			donnees_dependance.types_utilises.insere(decl->type);

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = m_compilatrice.typeuse[TypeBase::R32];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = m_compilatrice.typeuse[TypeBase::ENTIER_CONSTANT];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(noeud);
			expr->genre_valeur = GenreValeur::DROITE;

			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			if (expr->lexeme->genre == GenreLexeme::TABLEAU) {
				auto expression_taille = enfant1;
				auto expression_type = enfant2;

				auto type2 = expression_type->type;

				if (type2->genre != GenreType::TYPE_DE_DONNEES) {
					rapporte_erreur("Attendu une expression de type après la déclaration de type tableau", enfant2);
					return true;
				}

				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type2);
				auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;

				auto taille_tableau = 0l;

				if (expression_taille) {
					auto res = evalue_expression(m_compilatrice, expression_taille->bloc_parent, expression_taille);

					if (res.est_errone) {
						rapporte_erreur("Impossible d'évaluer la taille du tableau", expression_taille);
						return true;
					}

					if (res.type != type_expression::ENTIER) {
						rapporte_erreur("L'expression n'est pas de type entier", expression_taille);
						return true;
					}

					taille_tableau = res.entier;
				}

				if (taille_tableau != 0) {
					auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(type_connu, taille_tableau);
					expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
					donnees_dependance.types_utilises.insere(type_tableau);
				}
				else {
					auto type_tableau = m_compilatrice.typeuse.type_tableau_dynamique(type_connu);
					expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
					donnees_dependance.types_utilises.insere(type_tableau);
				}

				return false;
			}

			auto type_op = expr->lexeme->genre;

			auto assignation_composee = est_assignation_composee(type_op);

			auto type1 = enfant1->type;
			auto type2 = enfant2->type;

			if (type1->genre == GenreType::TYPE_DE_DONNEES) {
				if (type2->genre != GenreType::TYPE_DE_DONNEES) {
					rapporte_erreur("Opération impossible entre un type et autre chose", expr);
					return true;
				}

				auto type_type1 = static_cast<TypeTypeDeDonnees *>(type1);
				auto type_type2 = static_cast<TypeTypeDeDonnees *>(type2);

				if (type_type1->type_connu == nullptr) {
					rapporte_erreur("Opération impossible car le type n'est pas connu", enfant1);
					return true;
				}

				if (type_type2->type_connu == nullptr) {
					rapporte_erreur("Opération impossible car le type n'est pas connu", enfant2);
					return true;
				}

				switch (expr->lexeme->genre) {
					default:
					{
						rapporte_erreur("Opérateur inapplicable sur des types", expr);
						return true;
					}
					case GenreLexeme::BARRE:
					{
						if (type_type1->type_connu == type_type2->type_connu) {
							rapporte_erreur("Impossible de créer une union depuis des types similaires\n", expr);
							return true;
						}

						auto membres = kuri::tableau<TypeCompose::Membre>(2);
						membres[0] = { type_type1->type_connu, "0" };
						membres[1] = { type_type2->type_connu, "1" };

						auto type_union = m_compilatrice.typeuse.union_anonyme(std::move(membres));
						expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_union);
						donnees_dependance.types_utilises.insere(type_union);
						return false;
					}
					case GenreLexeme::EGALITE:
					{
						auto op = m_compilatrice.operateurs.op_comp_egal_types;
						expr->type = op->type_resultat;
						expr->op = op;
						donnees_dependance.types_utilises.insere(expr->type);
						return false;
					}
					case GenreLexeme::DIFFERENCE:
					{
						auto op = m_compilatrice.operateurs.op_comp_diff_types;
						expr->type = op->type_resultat;
						expr->op = op;
						donnees_dependance.types_utilises.insere(expr->type);
						return false;
					}
				}
			}

			/* détecte a comp b comp c */
			if (est_operateur_comp(type_op) && est_operateur_comp(enfant1->lexeme->genre)) {
				expr->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
				expr->type = m_compilatrice.typeuse[TypeBase::BOOL];

				auto enfant_expr = static_cast<NoeudExpressionBinaire *>(enfant1);
				type1 = enfant_expr->expr2->type;

				auto candidats = dls::tablet<OperateurCandidat, 10>();
				if (cherche_candidats_operateurs(m_compilatrice, *this, type1, type2, type_op, candidats)) {
					return true;
				}
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					rapporte_erreur_type_operation(noeud);
					return true;
				}

				expr->op = meilleur_candidat->op;
				enfant1->transformation = meilleur_candidat->transformation_type1;
				enfant2->transformation = meilleur_candidat->transformation_type2;

				if (!expr->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(expr->op->decl);
				}
			}
			else {
				if (assignation_composee) {
					type_op = operateur_pour_assignation_composee(type_op);
					expr->drapeaux |= EST_ASSIGNATION_COMPOSEE;

					// exclue les arithmétiques de pointeur
					if (!(type1->genre == GenreType::POINTEUR && (est_type_entier(type2) || type2->genre == GenreType::ENTIER_CONSTANT))) {

						auto transformation = TransformationType();
						if (cherche_transformation(m_compilatrice, *this, type2, type1, transformation)) {
							return true;
						}

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							rapporte_erreur_assignation_type_differents(type1, type2, enfant2);
							return true;
						}
					}
				}

				auto candidats = dls::tablet<OperateurCandidat, 10>();
				if (cherche_candidats_operateurs(m_compilatrice, *this, type1, type2, type_op, candidats)) {
					return true;
				}
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					rapporte_erreur_type_operation(noeud);
					return true;
				}

				expr->type = meilleur_candidat->op->type_resultat;
				expr->op = meilleur_candidat->op;
				enfant1->transformation = meilleur_candidat->transformation_type1;
				enfant2->transformation = meilleur_candidat->transformation_type2;

				if (!expr->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(expr->op->decl);
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);
			expr->genre_valeur = GenreValeur::DROITE;

			auto enfant = expr->expr;
			auto type = enfant->type;

			if (dls::outils::est_element(expr->lexeme->genre, GenreLexeme::FOIS_UNAIRE, GenreLexeme::ESP_UNAIRE)) {
				if (type->genre != GenreType::TYPE_DE_DONNEES) {
					rapporte_erreur("attendu l'expression d'un type", enfant);
					return true;
				}

				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type);
				auto type_connu = type_de_donnees->type_connu;

				if (type_connu == nullptr) {
					type_connu = type_de_donnees;
				}

				if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
					type_connu = m_compilatrice.typeuse.type_pointeur_pour(type_connu);
				}
				else if (expr->lexeme->genre == GenreLexeme::ESP_UNAIRE) {
					type_connu = m_compilatrice.typeuse.type_reference_pour(type_connu);
				}

				noeud->type = m_compilatrice.typeuse.type_type_de_donnees(type_connu);
				break;
			}

			if (type->genre == GenreType::REFERENCE) {
				enfant->transformation = TypeTransformation::DEREFERENCE;
				type = type_dereference_pour(type);
			}

			if (expr->type == nullptr) {
				if (expr->lexeme->genre == GenreLexeme::AROBASE) {
					if (!est_valeur_gauche(enfant->genre_valeur)) {
						rapporte_erreur("Ne peut pas prendre l'adresse d'une valeur-droite.", enfant);
						return true;
					}

					expr->type = m_compilatrice.typeuse.type_pointeur_pour(type);
				}
				else if (expr->lexeme->genre == GenreLexeme::EXCLAMATION) {
					if (!est_type_conditionnable(enfant->type)) {
						rapporte_erreur("Ne peut pas appliquer l'opérateur « ! » au type de l'expression", enfant);
						return true;
					}

					expr->type = m_compilatrice.typeuse[TypeBase::BOOL];
				}
				else {
					if (type->genre == GenreType::ENTIER_CONSTANT) {
						type = m_compilatrice.typeuse[TypeBase::Z32];
						enfant->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type };
					}

					auto op = cherche_operateur_unaire(m_compilatrice.operateurs, type, expr->lexeme->genre);

					if (op == nullptr) {
						rapporte_erreur_type_operation_unaire(expr);
						return true;
					}

					expr->type = op->type_resultat;
					expr->op = op;
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(noeud);
			expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;
			auto type1 = enfant1->type;

			if (type1->genre == GenreType::REFERENCE) {
				enfant1->transformation = TypeTransformation::DEREFERENCE;
				type1 = type_dereference_pour(type1);
			}

			switch (type1->genre) {
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					expr->type = type_dereference_pour(type1);
					VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_tableau);
					donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_tableau);
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					auto type_tabl = static_cast<TypeTableauFixe *>(type1);
					expr->type = type_dereference_pour(type1);

					auto res = evalue_expression(m_compilatrice, enfant2->bloc_parent, enfant2);

					if (!res.est_errone) {
						if (res.entier >= type_tabl->taille) {
							rapporte_erreur_acces_hors_limites(enfant2, type_tabl, res.entier);
							return true;
						}

						/* nous savons que l'accès est dans les limites,
						 * évite d'émettre le code de vérification */
						expr->aide_generation_code = IGNORE_VERIFICATION;
					}

					VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_tableau);
					donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_tableau);
					break;
				}
				case GenreType::POINTEUR:
				{
					expr->type = type_dereference_pour(type1);
					break;
				}
				case GenreType::CHAINE:
				{
					expr->type = m_compilatrice.typeuse[TypeBase::Z8];
					VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_chaine);
					donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_chaine);
					break;
				}
				default:
				{
					dls::flux_chaine ss;
					ss << "Le type '" << chaine_type(type1)
					   << "' ne peut être déréférencé par opérateur[] !";
					rapporte_erreur(ss.chn().c_str(), noeud, erreur::type_erreur::TYPE_DIFFERENTS);
					return true;
				}
			}

			auto type_cible = m_compilatrice.typeuse[TypeBase::Z64];
			auto type_index = enfant2->type;

			if (type_index->genre == GenreType::ENUM) {
				type_index = static_cast<TypeEnum *>(type_index)->type_donnees;
			}

			auto transformation = TransformationType();

			if (cherche_transformation(m_compilatrice, *this, type_index, type_cible, transformation)) {
				return true;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				if (enfant2->type->genre == GenreType::ENTIER_NATUREL) {
					transformation = TransformationType(TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible);
				}
				else {
					rapporte_erreur_type_indexage(enfant2);
					return true;
				}
			}

			enfant2->transformation = transformation;

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
			noeud->genre_valeur = GenreValeur::DROITE;

			auto type_fonc = static_cast<TypeFonction *>(fonction_courante->type);

			if (inst->expr == nullptr) {
				noeud->type = m_compilatrice.typeuse[TypeBase::RIEN];

				if (!fonction_courante->est_coroutine && (type_fonc->types_sorties[0] != noeud->type)) {
					rapporte_erreur("Expression de retour manquante", noeud);
					return true;
				}

				donnees_dependance.types_utilises.insere(noeud->type);
				return false;
			}

			auto enfant = inst->expr;
			auto nombre_retour = type_fonc->types_sorties.taille;

			if (nombre_retour > 1) {
				if (enfant->lexeme->genre == GenreLexeme::VIRGULE) {
					dls::tablet<NoeudExpression *, 10> feuilles;
					rassemble_feuilles(enfant, feuilles);

					if (feuilles.taille() != nombre_retour) {
						rapporte_erreur("Le compte d'expression de retour est invalide", noeud);
						return true;
					}

					for (auto i = 0l; i < feuilles.taille(); ++i) {
						auto f = feuilles[i];

						auto transformation = TransformationType();
						if (cherche_transformation(m_compilatrice, *this, f->type, type_fonc->types_sorties[i], transformation)) {
							return true;
						}

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							rapporte_erreur_type_retour(type_fonc->types_sorties[i], f->type, noeud);
							return true;
						}

						f->transformation = transformation;

						donnees_dependance.types_utilises.insere(f->type);
					}

					/* À FAIRE : multiples types de retour */
					noeud->type = feuilles[0]->type;
					noeud->genre = GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE;
				}
				else if (enfant->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
					/* À FAIRE : multiples types de retour, confirmation typage */
					noeud->type = enfant->type;
					noeud->genre = GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE;
				}
				else {
					rapporte_erreur("Le compte d'expression de retour est invalide", noeud);
					return true;
				}
			}
			else {
				noeud->type = type_fonc->types_sorties[0];
				noeud->genre = GenreNoeud::INSTRUCTION_RETOUR_SIMPLE;

				auto transformation = TransformationType();
				if (cherche_transformation(m_compilatrice, *this, enfant->type, noeud->type, transformation)) {
					return true;
				}

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					rapporte_erreur_type_retour(noeud->type, enfant->type, noeud);
					return true;
				}

				enfant->transformation = transformation;
			}

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			noeud->type = m_compilatrice.typeuse[TypeBase::CHAINE];
			noeud->genre_valeur = GenreValeur::DROITE;
			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = m_compilatrice.typeuse[TypeBase::BOOL];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = m_compilatrice.typeuse[TypeBase::Z8];
			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst = static_cast<NoeudSi *>(noeud);

			auto type_condition = inst->condition->type;

			if (type_condition == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
				return true;
			}

			if (!est_type_conditionnable(type_condition)) {
				rapporte_erreur("Impossible de conditionner le type de l'expression 'si'", inst->condition, erreur::type_erreur::TYPE_DIFFERENTS);
				return true;
			}

			/* pour les expressions x = si y { z } sinon { w } */
			inst->type = inst->bloc_si_vrai->type;

			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto inst = static_cast<NoeudBloc *>(noeud);

			if (inst->expressions.est_vide()) {
				noeud->type = m_compilatrice.typeuse[TypeBase::RIEN];
			}
			else {
				noeud->type = inst->expressions[inst->expressions.taille - 1]->type;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto inst = static_cast<NoeudPour *>(noeud);

			/* on génère d'abord le type de la variable */
			auto enfant1 = inst->variable;
			auto enfant2 = inst->expression;
			auto enfant3 = inst->bloc;

			/* À FAIRE : utilisation du type */
//			auto df = static_cast<DonneesFonction *>(nullptr);

			auto feuilles = dls::tablet<NoeudExpression *, 10>{};
			rassemble_feuilles(enfant1, feuilles);

			for (auto f : feuilles) {
				auto decl_f = trouve_dans_bloc(noeud->bloc_parent, f->ident);

				if (decl_f != nullptr) {
					if (f->lexeme->ligne > decl_f->lexeme->ligne) {
						rapporte_erreur("Redéfinition de la variable", f, erreur::type_erreur::VARIABLE_REDEFINIE);
						return true;
					}
				}
			}

			auto variable = feuilles[0];
			inst->ident = variable->ident;

			auto requiers_index = feuilles.taille() == 2;

			auto type = enfant2->type;

			/* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
			 * type de retour d'une coroutine n'interfère avec le type d'une
			 * variable (par exemple quand nous retournons une chaine). */
			if (enfant2->genre == GenreNoeud::EXPRESSION_PLAGE) {
				if (requiers_index) {
					noeud->aide_generation_code = GENERE_BOUCLE_PLAGE_INDEX;
				}
				else {
					noeud->aide_generation_code = GENERE_BOUCLE_PLAGE;
				}
			}
			// À FAIRE (réusinage arbre)
//			else if (enfant2->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION && enfant2->df->est_coroutine) {
//				enfant1->type = enfant2->type;

//				df = enfant2->df;
//				auto nombre_vars_ret = df->idx_types_retours.taille();

//				if (feuilles.taille() == nombre_vars_ret) {
//					requiers_index = false;
//					noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE;
//				}
//				else if (feuilles.taille() == nombre_vars_ret + 1) {
//					requiers_index = true;
//					noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
//				}
//				else {
//					rapporte_erreur(
//								"Mauvais compte d'arguments à déployer",
//								compilatrice,
//								*enfant1->lexeme);
//				}
//			}
			else {
				if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::VARIADIQUE) {
					type = type_dereference_pour(type);

					if (requiers_index) {
						noeud->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						noeud->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}
				}
				else if (type->genre == GenreType::CHAINE) {
					type = m_compilatrice.typeuse[TypeBase::Z8];
					enfant1->type = type;

					if (requiers_index) {
						noeud->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						noeud->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}
				}
				else {
					rapporte_erreur("La variable n'est ni un argument variadic, ni un tableau, ni une chaine", enfant2);
					return true;
				}
			}

			donnees_dependance.types_utilises.insere(type);
			enfant3->membres.reserve(feuilles.taille());

			auto nombre_feuilles = feuilles.taille() - requiers_index;

			for (auto i = 0l; i < nombre_feuilles; ++i) {
				auto f = feuilles[i];

				auto decl_f = static_cast<NoeudDeclarationVariable *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme));
				decl_f->bloc_parent = noeud->bloc_parent;
				decl_f->valeur = f;
				decl_f->type = type;
				decl_f->ident = f->ident;
				decl_f->lexeme = f->lexeme;
				decl_f->drapeaux |= DECLARATION_FUT_VALIDEE;

				if (enfant2->genre != GenreNoeud::EXPRESSION_PLAGE) {
					decl_f->drapeaux |= EST_VAR_BOUCLE;
					f->drapeaux |= EST_VAR_BOUCLE;
				}

				enfant3->membres.pousse(decl_f);
			}

			if (requiers_index) {
				auto idx = feuilles.back();

				auto decl_idx = static_cast<NoeudDeclarationVariable *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme));
				decl_idx->bloc_parent = noeud->bloc_parent;
				decl_idx->valeur = idx;

				if (noeud->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
					decl_idx->type = m_compilatrice.typeuse[TypeBase::Z32];
				}
				else {
					decl_idx->type = m_compilatrice.typeuse[TypeBase::Z64];
				}

				decl_idx->ident = idx->ident;
				decl_idx->lexeme = idx->lexeme;
				decl_idx->drapeaux |= DECLARATION_FUT_VALIDEE;

				enfant3->membres.pousse(decl_idx);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(noeud);
			expr->genre_valeur = GenreValeur::DROITE;

			if (resoud_type_final(expr->expr2, expr->type)) {
				return true;
			}

			if (noeud->type == nullptr) {
				rapporte_erreur("Ne peut transtyper vers un type invalide", expr, erreur::type_erreur::TYPE_INCONNU);
				return true;
			}

			donnees_dependance.types_utilises.insere(noeud->type);

			auto enfant = expr->expr1;
			if (enfant->type == nullptr) {
				rapporte_erreur("Ne peut calculer le type d'origine", enfant, erreur::type_erreur::TYPE_INCONNU);
				return true;
			}

			auto transformation = TransformationType();

			if (cherche_transformation_pour_transtypage(m_compilatrice, *this, expr->expr1->type, noeud->type, transformation)) {
				return true;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				rapporte_erreur_type_arguments(noeud, expr->expr1);
				return true;
			}

			expr->expr1->transformation = transformation;

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = m_compilatrice.typeuse[TypeBase::PTR_NUL];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);
			expr->genre_valeur = GenreValeur::DROITE;
			expr->type = m_compilatrice.typeuse[TypeBase::N32];

			auto expr_type = expr->expr;
			if (resoud_type_final(expr_type, expr_type->type)) {
				return true;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(noeud);
			auto enfant1 = inst->expr1;
			auto enfant2 = inst->expr2;

			auto type_debut = enfant1->type;
			auto type_fin   = enfant2->type;

			if (type_debut == nullptr || type_fin == nullptr) {
				rapporte_erreur("Les types de l'expression sont invalides !", noeud, erreur::type_erreur::TYPE_INCONNU);
				return true;
			}

			if (type_debut != type_fin) {
				if (type_debut->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_fin)) {
					type_debut = type_fin;
					enfant1->type = type_debut;
					enfant1->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut };
				}
				else if (type_fin->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_debut)) {
					type_fin = type_debut;
					enfant2->type = type_fin;
					enfant2->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_fin };
				}
				else {
					rapporte_erreur_type_operation(type_debut, type_fin, noeud);
					return true;
				}
			}
			else if (type_debut->genre == GenreType::ENTIER_CONSTANT) {
				type_debut = m_compilatrice.typeuse[TypeBase::Z32];
				enfant1->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut };
				enfant2->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut };
			}

			if (type_debut->genre != GenreType::ENTIER_NATUREL && type_debut->genre != GenreType::ENTIER_RELATIF && type_debut->genre != GenreType::REEL) {
				rapporte_erreur("Attendu des types réguliers dans la plage de la boucle 'pour'", noeud, erreur::type_erreur::TYPE_DIFFERENTS);
				return true;
			}

			noeud->type = type_debut;

			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
			auto chaine_var = inst->expr == nullptr ? nullptr : inst->expr->ident;
			auto ok = bloc_est_dans_boucle(noeud->bloc_parent, chaine_var);

			if (ok == false) {
				if (chaine_var == nullptr) {
					rapporte_erreur("'continue' ou 'arrête' en dehors d'une boucle", noeud, erreur::type_erreur::CONTROLE_INVALIDE);
					return true;
				}

				rapporte_erreur("Variable inconnue", inst->expr, erreur::type_erreur::VARIABLE_INCONNUE);
				return true;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = static_cast<NoeudBoucle *>(noeud);
			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
				return true;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst = static_cast<NoeudBoucle *>(noeud);

			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
				return true;
			}

			if (inst->condition->type->genre != GenreType::BOOL) {
				rapporte_erreur("Une expression booléenne est requise pour la boucle 'tantque'", inst->condition, erreur::type_erreur::TYPE_ARGUMENT);
				return true;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);
			noeud->genre_valeur = GenreValeur::DROITE;

			dls::tablet<NoeudExpression *, 10> feuilles;
			rassemble_feuilles(expr->expr, feuilles);

			if (feuilles.est_vide()) {
				return false;
			}

			auto premiere_feuille = feuilles.front();

			auto type_feuille = premiere_feuille->type;

			if (type_feuille->genre == GenreType::ENTIER_CONSTANT) {
				type_feuille = m_compilatrice.typeuse[TypeBase::Z32];
				premiere_feuille->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_feuille };
			}

			for (auto f : feuilles) {
				auto transformation = TransformationType();
				if (cherche_transformation(m_compilatrice, *this, f->type, type_feuille, transformation)) {
					return true;
				}

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					rapporte_erreur_assignation_type_differents(f->type, type_feuille, f);
					return true;
				}

				f->transformation = transformation;
			}

			noeud->type = m_compilatrice.typeuse.type_tableau_fixe(type_feuille, feuilles.taille());

			/* ajoute également le type de pointeur pour la génération de code C */
			auto type_ptr = m_compilatrice.typeuse.type_pointeur_pour(type_feuille);

			donnees_dependance.types_utilises.insere(noeud->type);
			donnees_dependance.types_utilises.insere(type_ptr);
			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto noeud_expr = static_cast<NoeudExpressionUnaire *>(noeud);
			auto expr = noeud_expr->expr;

			if (resoud_type_final(noeud_expr->expr, expr->type)) {
				return true;
			}

			auto type_info_type = static_cast<Type *>(nullptr);

			switch (expr->type->genre) {
				case GenreType::INVALIDE:
				case GenreType::POLYMORPHIQUE:
				{
					break;
				}
				case GenreType::EINI:
				case GenreType::CHAINE:
				case GenreType::RIEN:
				case GenreType::BOOL:
				case GenreType::OCTET:
				case GenreType::TYPE_DE_DONNEES:
				case GenreType::REEL:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_;
					break;
				}
				case GenreType::ENTIER_CONSTANT:
				case GenreType::ENTIER_NATUREL:
				case GenreType::ENTIER_RELATIF:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_entier;
					break;
				}
				case GenreType::REFERENCE:
				case GenreType::POINTEUR:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_pointeur;
					break;
				}
				case GenreType::STRUCTURE:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_structure;
					break;
				}
				case GenreType::UNION:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_union;
					break;
				}
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				case GenreType::TABLEAU_FIXE:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_tableau;
					break;
				}
				case GenreType::FONCTION:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_fonction;
					break;
				}
				case GenreType::ENUM:
				case GenreType::ERREUR:
				{
					type_info_type = m_compilatrice.typeuse.type_info_type_enum;
					break;
				}
			}

			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = m_compilatrice.typeuse.type_pointeur_pour(type_info_type);

			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			Type *type = nullptr;

			if (resoud_type_final(noeud->expression_type, type)) {
				rapporte_erreur("impossible de définir le type de init_de", noeud);
				return true;
			}

			if (type->genre != GenreType::STRUCTURE && type->genre != GenreType::UNION) {
				rapporte_erreur("init_de doit prendre le type d'une structure ou d'une union", noeud);
				return true;
			}

			auto types_entrees = kuri::tableau<Type *>(2);
			types_entrees[0] = m_compilatrice.type_contexte;
			types_entrees[1] = m_compilatrice.typeuse.type_pointeur_pour(type);

			auto types_sorties = kuri::tableau<Type *>(1);
			types_sorties[0] = m_compilatrice.typeuse[TypeBase::RIEN];

			auto type_fonction = m_compilatrice.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
			noeud->type = type_fonction;

			donnees_dependance.types_utilises.insere(noeud->type);

			break;
		}
		case GenreNoeud::EXPRESSION_TYPE_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);
			auto expr_type = expr->expr;

			if (expr_type->type == nullptr) {
				rapporte_erreur("impossible de définir le type de l'expression de type_de", expr_type);
				return true;
			}

			if (expr_type->type->genre == GenreType::TYPE_DE_DONNEES) {
				noeud->type = expr_type->type;
			}
			else {
				noeud->type = m_compilatrice.typeuse.type_type_de_donnees(expr_type->type);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);

			auto type = expr->expr->type;

			if (type->genre != GenreType::POINTEUR) {
				rapporte_erreur("Un pointeur est requis pour le déréférencement via 'mémoire'", expr->expr, erreur::type_erreur::TYPE_DIFFERENTS);
				return true;
			}

			auto type_pointeur = static_cast<TypePointeur *>(type);
			noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;
			noeud->type = type_pointeur->type_pointe;

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(noeud);
			expr_loge->genre_valeur = GenreValeur::DROITE;

			if (resoud_type_final(expr_loge->expression_type, expr_loge->type)) {
				return true;
			}

			if (dls::outils::est_element(expr_loge->type->genre, GenreType::CHAINE, GenreType::TABLEAU_DYNAMIQUE)) {
				if (expr_loge->expr_taille == nullptr) {
					rapporte_erreur("Attendu une expression pour définir la taille du tableau à loger", noeud);
					return true;
				}

				auto type_cible = m_compilatrice.typeuse[TypeBase::Z64];
				auto transformation = TransformationType();
				if (cherche_transformation(m_compilatrice, *this, expr_loge->expr_taille->type, type_cible, transformation)) {
					return true;
				}

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					rapporte_erreur_assignation_type_differents(expr_loge->expr_taille->type, type_cible, expr_loge->expr_taille);
					return true;
				}

				expr_loge->expr_taille->transformation = transformation;
			}
			else {
				auto type_loge = expr_loge->type;

				/* attend sur le type car nous avons besoin de sa validation pour la génération de RI pour l'expression de logement */
				if ((type_loge->drapeaux & TYPE_FUT_VALIDE) == 0) {
					unite->attend_sur_type(type_loge);
					return true;
				}

				expr_loge->type = m_compilatrice.typeuse.type_pointeur_pour(type_loge);
			}

			if (expr_loge->bloc != nullptr) {
				auto di = derniere_instruction(expr_loge->bloc);
				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					rapporte_erreur("Le bloc sinon d'une instruction « loge » doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter", expr_loge);
					return true;
				}
			}
			else {
				VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_memoire);
				donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_memoire);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);

			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(noeud);

			if (resoud_type_final(expr_loge->expression_type, expr_loge->type)) {
				return true;
			}

			if (dls::outils::est_element(expr_loge->type->genre, GenreType::CHAINE, GenreType::TABLEAU_DYNAMIQUE)) {
				if (expr_loge->expr_taille == nullptr) {
					rapporte_erreur("Attendu une expression pour définir la taille à reloger", noeud);
					return true;
				}

				auto type_cible = m_compilatrice.typeuse[TypeBase::Z64];				
				auto transformation = TransformationType();
				if (cherche_transformation(m_compilatrice, *this, expr_loge->expr_taille->type, type_cible, transformation)) {
					return true;
				}

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					rapporte_erreur_assignation_type_differents(expr_loge->expr_taille->type, type_cible, expr_loge->expr_taille);
					return true;
				}

				expr_loge->expr_taille->transformation = transformation;
			}
			else {
				auto type_loge = expr_loge->type;

				/* attend sur le type car nous avons besoin de sa validation pour la génération de RI pour l'expression de logement */
				if ((type_loge->drapeaux & TYPE_FUT_VALIDE) == 0) {
					unite->attend_sur_type(type_loge);
					return true;
				}

				expr_loge->type = m_compilatrice.typeuse.type_pointeur_pour(type_loge);
			}

			/* pour les références */
			auto transformation = TransformationType();
			if (cherche_transformation(m_compilatrice, *this, expr_loge->expr->type, expr_loge->type, transformation)) {
				return true;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				rapporte_erreur_type_arguments(expr_loge, expr_loge->expr);
				return true;
			}

			expr_loge->expr->transformation = transformation;

			if (expr_loge->bloc != nullptr) {
				auto di = derniere_instruction(expr_loge->bloc);
				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					rapporte_erreur("Le bloc sinon d'une instruction « reloge » doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter", expr_loge);
					return true;
				}
			}
			else {
				VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_memoire);
				donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_memoire);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);

			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(noeud);
			auto type = expr_loge->expr->type;

			if (type->genre == GenreType::REFERENCE) {
				expr_loge->expr->transformation = TypeTransformation::DEREFERENCE;
				type = static_cast<TypeReference *>(type)->type_pointe;
			}

			if (!dls::outils::est_element(type->genre, GenreType::POINTEUR, GenreType::TABLEAU_DYNAMIQUE, GenreType::CHAINE)) {
				rapporte_erreur("Le type n'est pas délogeable", noeud);
				return true;
			}

			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct *>(noeud);
			return valide_structure(decl);
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			return valide_enum(static_cast<NoeudEnum *>(noeud));
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			auto inst = static_cast<NoeudDiscr *>(noeud);

			auto expression = inst->expr;

			auto type = expression->type;

			if (type->genre == GenreType::REFERENCE) {
				type = static_cast<TypeReference *>(type)->type_pointe;
				noeud->transformation = TypeTransformation::DEREFERENCE;
			}

			if (type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(type);
				auto decl = type_union->decl;

				if (decl->est_nonsure) {
					rapporte_erreur("« discr » ne peut prendre une union nonsûre", expression);
					return true;
				}

				auto membres_rencontres = dls::ensemblon<IdentifiantCode *, 16>();

				auto valide_presence_membres = [&membres_rencontres, &decl, this, &expression]() {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					POUR (decl->bloc->membres) {
						if (!membres_rencontres.possede(it->ident)) {
							valeurs_manquantes.insere(it->lexeme->chaine);
						}
					}

					if (valeurs_manquantes.taille() != 0) {
						rapporte_erreur_valeur_manquante_discr(expression, valeurs_manquantes);
						return true;
					}

					return false;
				};

				noeud->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;
					auto bloc_paire = inst->paires_discr[i].second;

					/* vérifie que toutes les expressions des paires sont bel et
					 * bien des membres */
					if (expr_paire->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
						rapporte_erreur("Attendu une variable membre de l'union nonsûre", expr_paire);
						return true;
					}

					auto nom_membre = expr_paire->ident->nom;

					if (membres_rencontres.possede(expr_paire->ident)) {
						rapporte_erreur("Redéfinition de l'expression", expr_paire);
						return true;
					}

					membres_rencontres.insere(expr_paire->ident);

					auto decl_var = trouve_dans_bloc_seul(decl->bloc, expr_paire);

					if (decl_var == nullptr) {
						rapporte_erreur_membre_inconnu(noeud, expression, expr_paire, type_union);
						return true;
					}

					renseigne_membre_actif(expression->ident->nom, nom_membre);

					auto decl_prec = trouve_dans_bloc(inst->bloc_parent, expression->ident);

					/* Pousse la variable comme étant employée, puisque nous savons ce qu'elle est */
					if (decl_prec != nullptr) {
						rapporte_erreur("Ne peut pas utiliser implicitement le membre car une variable de ce nom existe déjà", expr_paire);
						return true;
					}

					/* pousse la variable dans le bloc suivant */
					auto decl_expr = static_cast<NoeudDeclaration *>(nullptr);
					decl_expr->ident = expression->ident;
					decl_expr->lexeme = expression->lexeme;
					decl_expr->bloc_parent = bloc_paire;
					decl_expr->drapeaux_decl |= EMPLOYE;
					decl_expr->type = expression->type;
					// À FAIRE: mise en place des informations d'emploie

					bloc_paire->membres.pousse(decl_expr);
				}

				if (inst->bloc_sinon == nullptr) {
					return valide_presence_membres();
				}
			}
			else if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
				auto type_enum = static_cast<TypeEnum *>(type);

				auto membres_rencontres = dls::ensemblon<dls::vue_chaine_compacte, 16>();
				noeud->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;

					auto feuilles = dls::tablet<NoeudExpression *, 10>();
					rassemble_feuilles(expr_paire, feuilles);

					for (auto f : feuilles) {
						if (f->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
							rapporte_erreur("expression inattendue dans la discrimination, seules les références de déclarations sont supportées pour le moment", f);
							return true;
						}

						auto nom_membre = f->ident->nom;

						auto nom_trouve = false;

						POUR (type_enum->membres) {
							if (it.nom == nom_membre) {
								nom_trouve = true;
								break;
							}
						}

						if (!nom_trouve) {
							rapporte_erreur_membre_inconnu(noeud, expression, expr_paire, type_enum);
							return true;
						}

						if (membres_rencontres.possede(nom_membre)) {
							rapporte_erreur("Redéfinition de l'expression", f);
							return true;
						}

						membres_rencontres.insere(nom_membre);
					}
				}

				if (inst->bloc_sinon == nullptr) {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					POUR (type_enum->membres) {
						if (!membres_rencontres.possede(it.nom)) {
							valeurs_manquantes.insere(it.nom);
						}
					}

					if (valeurs_manquantes.taille() != 0) {
						rapporte_erreur_valeur_manquante_discr(expression, valeurs_manquantes);
						return true;
					}
				}
			}
			else {
				auto candidats = dls::tablet<OperateurCandidat, 10>();
				if (cherche_candidats_operateurs(m_compilatrice, *this, type, type, GenreLexeme::EGALITE, candidats)) {
					return true;
				}

				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					rapporte_erreur_type_operation(noeud);
					return true;
				}

				inst->op = meilleur_candidat->op;

				if (!inst->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(inst->op->decl);
				}

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;

					auto feuilles = dls::tablet<NoeudExpression *, 10>();
					rassemble_feuilles(expr_paire, feuilles);

					for (auto f : feuilles) {
						if (valide_semantique_noeud(f)) {
							return true;
						}

						auto transformation = TransformationType();
						if (cherche_transformation(m_compilatrice, *this, f->type, expression->type, transformation)) {
							return true;
						}

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							rapporte_erreur_type_arguments(expression, f);
							return true;
						}

						f->transformation = transformation;
					}
				}
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			if (!fonction_courante->est_coroutine) {
				rapporte_erreur("'retiens' hors d'une coroutine", noeud);
				return true;
			}

			auto type_fonc = static_cast<TypeFonction *>(fonction_courante->type);

			auto inst = static_cast<NoeudExpressionUnaire *>(noeud);

			/* À FAIRE : multiple types retours. */
			auto type_retour = type_fonc->types_sorties[0];			
			auto transformation = TransformationType();
			if (cherche_transformation(m_compilatrice, *this, inst->expr->type, type_retour, transformation)) {
				return true;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				rapporte_erreur_type_retour(type_retour, inst->expr->type, noeud);
				return true;
			}

			inst->transformation = transformation;

			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionParenthese *>(noeud);
			noeud->type = expr->expr->type;
			noeud->genre_valeur = expr->expr->genre_valeur;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto inst = static_cast<NoeudPousseContexte *>(noeud);
			auto variable = inst->expr;

			// @vérifie si redondant
			auto decl = trouve_dans_bloc(inst->bloc_parent, variable->ident);

			if (decl == nullptr) {
				rapporte_erreur("variable inconnu", variable);
				return true;
			}

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);

			if (expr->expr == nullptr) {
				// nous avons un type variadique
				auto type_var = m_compilatrice.typeuse.type_variadique(nullptr);
				expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_var);
				return false;
			}

			auto type_expr = expr->expr->type;

			if (type_expr->genre == GenreType::TYPE_DE_DONNEES) {
				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type_expr);
				auto type_var = m_compilatrice.typeuse.type_variadique(type_de_donnees->type_connu);
				expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_var);
			}
			else {
				expr->type = type_expr;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(noeud);
			inst->type = inst->expr_appel->type;
			inst->genre_valeur = GenreValeur::DROITE;

			auto type_de_l_erreur = static_cast<Type *>(nullptr);

			// voir ce que l'on retourne
			// - si aucun type erreur -> erreur ?
			// - si erreur seule -> il faudra vérifier l'erreur
			// - si union -> voir si l'union est sûre et contient une erreur, dépaquete celle-ci dans le génération de code

			if (inst->type->genre == GenreType::ERREUR) {
				type_de_l_erreur = inst->type;
			}
			else if (inst->type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(inst->type);
				auto possede_type_erreur = false;

				POUR (type_union->membres) {
					if (it.type->genre == GenreType::ERREUR) {
						possede_type_erreur = true;
					}
				}

				if (!possede_type_erreur) {
					rapporte_erreur("Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur", inst);
					return true;
				}

				if (type_union->membres.taille == 2) {
					if (type_union->membres[0].type->genre == GenreType::ERREUR) {
						type_de_l_erreur = type_union->membres[0].type;
						inst->type = type_union->membres[1].type;
					}
					else {
						inst->type = type_union->membres[0].type;
						type_de_l_erreur = type_union->membres[1].type;
					}
				}
			}
			else {
				rapporte_erreur("Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur", inst);
				return true;
			}

			if (inst->expr_piege) {
				if (inst->expr_piege->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					rapporte_erreur("Expression inattendu dans l'expression de piège, nous devons avoir une référence à une variable", inst->expr_piege);
					return true;
				}

				auto var_piege = static_cast<NoeudExpressionReference *>(inst->expr_piege);

				auto decl = trouve_dans_bloc(var_piege->bloc_parent, var_piege->ident);

				if (decl != nullptr) {
					rapporte_erreur_redefinition_symbole(var_piege, decl);
				}

				var_piege->type = type_de_l_erreur;

				auto decl_var_piege = static_cast<NoeudDeclarationVariable *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, var_piege->lexeme));
				decl_var_piege->bloc_parent = inst->bloc;
				decl_var_piege->valeur = var_piege;
				decl_var_piege->type = var_piege->type;
				decl_var_piege->ident = var_piege->ident;
				decl_var_piege->drapeaux |= DECLARATION_FUT_VALIDEE;

				// ne l'ajoute pas aux expressions, car nous devons l'initialiser manuellement
				inst->bloc->membres.pousse_front(decl_var_piege);

				auto di = derniere_instruction(inst->bloc);

				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					rapporte_erreur("Un bloc de piège doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter", inst);
					return true;
				}
			}
			else {
				VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_erreur);
				donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_erreur);
			}

			break;
		}
	}

	return false;
}

bool ContexteValidationCode::valide_acces_membre(NoeudExpressionMembre *expression_membre)
{
	PROFILE_FONCTION;

	auto structure = expression_membre->accede;
	auto membre = expression_membre->membre;
	auto type = structure->type;

	/* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
	while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
		type = static_cast<TypePointeur *>(type)->type_pointe;
	}

	// Il est possible d'avoir une chaine de type : Struct1.Struct2.Struct3...
	if (type->genre == GenreType::TYPE_DE_DONNEES) {
		auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type);

		if (type_de_donnees->type_connu != nullptr) {
			type = type_de_donnees->type_connu;
			// change le type de la structure également pour simplifier la génération
			// de la RI (nous nous basons sur le type pour ça)
			structure->type = type;
		}
	}

	if (est_type_compose(type)) {
		if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
			unite->attend_sur_type(type);
			return true;
		}

		auto type_compose = static_cast<TypeCompose *>(type);

		auto membre_trouve = false;
		auto index_membre = 0;
		auto membre_est_constant = false;;

		POUR (type_compose->membres) {
			if (it.nom == membre->ident->nom) {
				expression_membre->type = it.type;
				membre_trouve = true;
				membre_est_constant = it.drapeaux == TypeCompose::Membre::EST_CONSTANT;
				break;
			}

			index_membre += 1;
		}

		if (membre_trouve == false) {
			rapporte_erreur_membre_inconnu(expression_membre, structure, membre, type_compose);
			return true;
		}

		expression_membre->index_membre = index_membre;

		if (membre_est_constant || type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
			expression_membre->genre_valeur = GenreValeur::DROITE;
		}
		else if (type->genre == GenreType::UNION) {
			auto noeud_struct = static_cast<TypeUnion *>(type)->decl;
			if (!noeud_struct->est_nonsure) {
				expression_membre->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;

				if ((expression_membre->drapeaux & DROITE_ASSIGNATION) == 0) {
					renseigne_membre_actif(structure->ident->nom, membre->ident->nom);
				}
				else {
					auto membre_actif = trouve_membre_actif(structure->ident->nom);

					VERIFIE_INTERFACE_KURI_CHARGEE(decl_panique_membre_union);
					donnees_dependance.fonctions_utilisees.insere(m_compilatrice.interface_kuri.decl_panique_membre_union);

					/* si l'union vient d'un retour ou d'un paramètre, le membre actif sera inconnu */
					if (membre_actif != "") {
						if (membre_actif != membre->ident->nom) {
							rapporte_erreur_membre_inactif(expression_membre, structure, membre);
							return true;
						}

						/* nous savons que nous avons le bon membre actif */
						expression_membre->aide_generation_code = IGNORE_VERIFICATION;
					}
				}
			}
		}

		return false;
	}

	auto flux = dls::flux_chaine();
	flux << "Impossible d'accéder au membre d'un objet n'étant pas une structure";
	flux << ", le type est ";
	flux << chaine_type(type);

	rapporte_erreur(flux.chn().c_str(), structure, erreur::type_erreur::TYPE_DIFFERENTS);
	return true;
}

bool ContexteValidationCode::valide_type_fonction(NoeudDeclarationFonction *decl)
{
	PROFILE_FONCTION;

	commence_fonction(decl);

	auto &graphe = m_compilatrice.graphe_dependance;
	auto noeud_dep = graphe.cree_noeud_fonction(decl);

	if (decl->est_coroutine) {
		decl->genre = GenreNoeud::DECLARATION_COROUTINE;
	}

	for (auto i = unite->index_reprise; i < decl->arbre_aplatis_entete.taille; ++i) {
		if (valide_semantique_noeud(decl->arbre_aplatis_entete[i])) {
			unite->index_reprise = i;
			graphe.ajoute_dependances(*noeud_dep, donnees_dependance);
			return true;
		}
	}

	// -----------------------------------
	if (!decl->est_instantiation_gabarit) {
		auto noms = dls::ensemblon<IdentifiantCode *, 16>();
		auto dernier_est_variadic = false;

		POUR (decl->params) {
			auto param = static_cast<NoeudDeclarationVariable *>(it);
			auto variable = param->valeur;
			auto expression = param->expression;

			if (noms.possede(variable->ident)) {
				rapporte_erreur("Redéfinition de l'argument", variable, erreur::type_erreur::ARGUMENT_REDEFINI);
				return true;
			}

			if (dernier_est_variadic) {
				rapporte_erreur("Argument déclaré après un argument variadic", variable);
				return true;
			}

			if (variable->type && variable->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				rassemble_noms_type_polymorphique(variable->type, decl->noms_types_gabarits);
				decl->est_gabarit = true;
			}
			else {
				if (expression != nullptr) {
					if (decl->genre == GenreNoeud::DECLARATION_OPERATEUR) {
						rapporte_erreur("Un paramètre d'une surcharge d'opérateur ne peut avoir de valeur par défaut", param);
						return true;
					}
				}
			}

			noms.insere(variable->ident);

			if (it->type->genre == GenreType::VARIADIQUE) {
				it->drapeaux |= EST_VARIADIQUE;
				decl->est_variadique = true;
				dernier_est_variadic = true;

				auto type_var = static_cast<TypeVariadique *>(it->type);

				if (!decl->est_externe && type_var->type_pointe == nullptr) {
					rapporte_erreur(
								"La déclaration de fonction variadique sans type n'est"
								" implémentée que pour les fonctions externes",
								it);
					return true;
				}
			}
		}

		if (decl->est_gabarit) {
			decl->drapeaux |= DECLARATION_FUT_VALIDEE;
			return false;
		}
	}
	else {
		POUR (decl->params) {
			auto variable = static_cast<NoeudDeclarationVariable *>(it)->valeur;
			if (resoud_type_final(it->expression_type, variable->type)) {
				return true;
			}
			it->type = variable->type;
		}
	}

	// -----------------------------------

	kuri::tableau<Type *> types_entrees;
	auto possede_contexte = !decl->est_externe && !possede_drapeau(decl->drapeaux, FORCE_NULCTX);
	types_entrees.reserve(decl->params.taille + possede_contexte);

	if (possede_contexte) {
		types_entrees.pousse(m_compilatrice.type_contexte);
	}

	POUR (decl->params) {
		types_entrees.pousse(it->type);
	}

	kuri::tableau<Type *> types_sorties;
	types_sorties.reserve(decl->params_sorties.taille);

	for (auto &type_declare : decl->params_sorties) {
		Type *type_sortie = nullptr;
		if (resoud_type_final(type_declare, type_sortie)) {
			return true;
		}
		types_sorties.pousse(type_sortie);
	}

	auto type_fonc = m_compilatrice.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
	decl->type = type_fonc;
	donnees_dependance.types_utilises.insere(decl->type);

	if (decl->genre == GenreNoeud::DECLARATION_OPERATEUR) {
		auto type_resultat = type_fonc->types_sorties[0];

		if (type_resultat == m_compilatrice.typeuse[TypeBase::RIEN]) {
			rapporte_erreur("Un opérateur ne peut retourner 'rien'", decl);
			return true;
		}

		if (est_operateur_bool(decl->lexeme->genre) && type_resultat != m_compilatrice.typeuse[TypeBase::BOOL]) {
			rapporte_erreur("Un opérateur de comparaison doit retourner 'bool'", decl);
			return true;
		}

		auto fichier = m_compilatrice.fichier(static_cast<size_t>(decl->lexeme->fichier));
		decl->nom_broye = broye_nom_fonction(decl, fichier->module->nom);

		if (decl->params.taille == 1) {
			auto &iter_op = m_compilatrice.operateurs.trouve_unaire(decl->lexeme->genre);
			auto type1 = type_fonc->types_entrees[0 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type_operande == type1) {
					if (op->est_basique) {
						rapporte_erreur("redéfinition de l'opérateur basique", decl);
						return true;
					}

					// À FAIRE : inclus la position où l'opérateur fut défini
					rapporte_erreur("redéfinition de l'opérateur", decl);
					return true;
				}
			}

			m_compilatrice.operateurs.ajoute_perso_unaire(
						decl->lexeme->genre,
						type1,
						type_resultat,
						decl);
		}
		else if (decl->params.taille == 2) {
			auto &iter_op = m_compilatrice.operateurs.trouve_binaire(decl->lexeme->genre);
			auto type1 = type_fonc->types_entrees[0 + possede_contexte];
			auto type2 = type_fonc->types_entrees[1 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type1 == type1 && op->type2 == type2) {
					if (op->est_basique) {
						rapporte_erreur("redéfinition de l'opérateur basique", decl);
						return true;
					}

					// À FAIRE : inclus la position où l'opérateur fut défini
					rapporte_erreur("redéfinition de l'opérateur", decl);
					return true;
				}
			}

			m_compilatrice.operateurs.ajoute_perso(
						decl->lexeme->genre,
						type1,
						type2,
						type_resultat,
						decl);
		}
	}
	else {
		POUR (decl->bloc_parent->membres) {
			if (it == decl) {
				continue;
			}

			if (it->genre != GenreNoeud::DECLARATION_FONCTION && it->genre != GenreNoeud::DECLARATION_COROUTINE) {
				continue;
			}

			if (it->ident != decl->ident) {
				continue;
			}

			if (it->type == decl->type) {
				rapporte_erreur_redefinition_fonction(decl, it);
				return true;
			}
		}

		/* nous devons attendre d'avoir les types des arguments avant de
		 * pouvoir broyer le nom de la fonction */
		if (decl->lexeme->chaine != "principale" && !possede_drapeau(decl->drapeaux, EST_EXTERNE)) {
			auto fichier = m_compilatrice.fichier(static_cast<size_t>(decl->lexeme->fichier));
			decl->nom_broye = broye_nom_fonction(decl, fichier->module->nom);
		}
		else {
			decl->nom_broye = decl->lexeme->chaine;
		}
	}

	graphe.ajoute_dependances(*noeud_dep, donnees_dependance);
	decl->drapeaux |= DECLARATION_FUT_VALIDEE;
	return false;
}

bool ContexteValidationCode::valide_fonction(NoeudDeclarationFonction *decl)
{
	if (decl->est_gabarit && !decl->est_instantiation_gabarit) {
		// nous ferons l'analyse sémantique plus tard
		return false;
	}

	commence_fonction(decl);

	auto &graphe = m_compilatrice.graphe_dependance;
	auto noeud_dep = graphe.cree_noeud_fonction(decl);

	if (unite->index_reprise == 0) {
		auto requiers_contexte = !possede_drapeau(decl->drapeaux, FORCE_NULCTX);

		decl->bloc->membres.reserve(decl->params.taille + requiers_contexte);

		if (requiers_contexte) {
			auto val_ctx = static_cast<NoeudExpressionReference *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, decl->lexeme));
			val_ctx->type = m_compilatrice.type_contexte;
			val_ctx->bloc_parent = decl->bloc_parent;
			val_ctx->ident = m_compilatrice.table_identifiants.identifiant_pour_chaine("contexte");

			auto decl_ctx = static_cast<NoeudDeclarationVariable *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme));
			decl_ctx->bloc_parent = decl->bloc_parent;
			decl_ctx->valeur = val_ctx;
			decl_ctx->type = val_ctx->type;
			decl_ctx->ident = val_ctx->ident;
			decl_ctx->drapeaux |= DECLARATION_FUT_VALIDEE;

			decl->bloc->membres.pousse(decl_ctx);
		}

		POUR (decl->params) {
			auto argument = static_cast<NoeudDeclarationVariable *>(it);
			decl->bloc->membres.pousse(argument);
		}
	}

	for (auto i = unite->index_reprise; i < decl->arbre_aplatis.taille; ++i) {
		if (valide_semantique_noeud(decl->arbre_aplatis[i])) {
			unite->index_reprise = i;
			graphe.ajoute_dependances(*noeud_dep, donnees_dependance);
			return true;
		}
	}

	auto bloc = decl->bloc;
	auto inst_ret = derniere_instruction(bloc);

	/* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
	if (inst_ret == nullptr) {
		assert(decl->type->genre == GenreType::FONCTION);
		auto type_fonc = static_cast<TypeFonction *>(decl->type);

		if (type_fonc->types_sorties[0]->genre != GenreType::RIEN && !decl->est_coroutine) {
			rapporte_erreur("Instruction de retour manquante", decl, erreur::type_erreur::TYPE_DIFFERENTS);
			return true;
		}

		decl->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;
	}

	graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

	termine_fonction();
	return false;
}

bool ContexteValidationCode::valide_operateur(NoeudDeclarationFonction *decl)
{
	commence_fonction(decl);

	auto &graphe = m_compilatrice.graphe_dependance;
	auto noeud_dep = graphe.cree_noeud_fonction(decl);

	if (unite->index_reprise == 0) {
		auto requiers_contexte = !possede_drapeau(decl->drapeaux, FORCE_NULCTX);

		decl->bloc->membres.reserve(decl->params.taille + requiers_contexte);

		if (requiers_contexte) {
			auto val_ctx = static_cast<NoeudExpressionReference *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, decl->lexeme));
			val_ctx->type = m_compilatrice.type_contexte;
			val_ctx->bloc_parent = decl->bloc_parent;
			val_ctx->ident = m_compilatrice.table_identifiants.identifiant_pour_chaine("contexte");

			auto decl_ctx = static_cast<NoeudDeclarationVariable *>(m_compilatrice.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme));
			decl_ctx->bloc_parent = decl->bloc_parent;
			decl_ctx->valeur = val_ctx;
			decl_ctx->type = val_ctx->type;
			decl_ctx->ident = val_ctx->ident;
			decl_ctx->drapeaux |= DECLARATION_FUT_VALIDEE;

			decl->bloc->membres.pousse(decl_ctx);
		}

		POUR (decl->params) {
			auto argument = static_cast<NoeudDeclarationVariable *>(it);
			decl->bloc->membres.pousse(argument);
		}
	}

	for (auto i = unite->index_reprise; i < decl->arbre_aplatis.taille; ++i) {
		if (valide_semantique_noeud(decl->arbre_aplatis[i])) {
			unite->index_reprise = i;
			graphe.ajoute_dependances(*noeud_dep, donnees_dependance);
			return true;
		}
	}

	auto inst_ret = derniere_instruction(decl->bloc);

	if (inst_ret == nullptr) {
		rapporte_erreur("Instruction de retour manquante", decl, erreur::type_erreur::TYPE_DIFFERENTS);
		return true;
	}

	graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

	termine_fonction();
	return false;
}

bool ContexteValidationCode::valide_enum(NoeudEnum *decl)
{
	auto type_enum = static_cast<TypeEnum *>(decl->type);
	auto &membres = type_enum->membres;

	if (type_enum->est_erreur) {
		type_enum->type_donnees = m_compilatrice.typeuse[TypeBase::Z32];
	}
	else if (decl->expression_type != nullptr) {
		if (valide_semantique_noeud(decl->expression_type)) {
			return true;
		}

		if (resoud_type_final(decl->expression_type, type_enum->type_donnees)) {
			return true;
		}
	}
	else {
		type_enum->type_donnees = m_compilatrice.typeuse[TypeBase::Z32];
	}

	type_enum->taille_octet = type_enum->type_donnees->taille_octet;
	type_enum->alignement = type_enum->type_donnees->alignement;

	m_compilatrice.operateurs.ajoute_operateur_basique_enum(decl->type);

	auto noms_rencontres = dls::ensemblon<IdentifiantCode *, 32>();

	auto dernier_res = ResultatExpression();
	/* utilise est_errone pour indiquer que nous sommes à la première valeur */
	dernier_res.est_errone = true;

	membres.reserve(decl->bloc->expressions.taille);

	POUR (decl->bloc->expressions) {
		if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
			rapporte_erreur("Type d'expression inattendu dans l'énum", it);
			return true;
		}

		auto decl_expr = static_cast<NoeudDeclarationVariable *>(it);
		decl_expr->type = type_enum->type_donnees;

		auto var = decl_expr->valeur;

		if (var->expression_type != nullptr) {
			rapporte_erreur("Expression d'énumération déclarée avec un type", it);
			return true;
		}

		if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			rapporte_erreur("Expression invalide dans la déclaration du membre de l'énumération", var);
			return true;
		}

		if (noms_rencontres.possede(var->ident)) {
			rapporte_erreur("Redéfinition du membre", var);
			return true;
		}

		noms_rencontres.insere(var->ident);

		auto expr = decl_expr->expression;

		it->ident = var->ident;

		auto res = ResultatExpression();

		// À FAIRE(erreur) : vérifie qu'aucune expression s'évalue à zéro
		if (expr != nullptr) {
			res = evalue_expression(m_compilatrice, decl->bloc, expr);

			if (res.est_errone) {
				rapporte_erreur(res.message_erreur, res.noeud_erreur, erreur::type_erreur::VARIABLE_REDEFINIE);
				return true;
			}
		}
		else {
			if (dernier_res.est_errone) {
				/* première valeur, laisse à zéro si énum normal */
				dernier_res.est_errone = false;

				if (type_enum->est_drapeau || type_enum->est_erreur) {
					res.type = type_expression::ENTIER;
					res.entier = 1;
				}
			}
			else {
				if (dernier_res.type == type_expression::ENTIER) {
					if (type_enum->est_drapeau) {
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

		membres.pousse({ type_enum, var->ident->nom, 0, static_cast<int>(res.entier) });

		dernier_res = res;
	}

	decl->drapeaux |= DECLARATION_FUT_VALIDEE;
	decl->type->drapeaux |= TYPE_FUT_VALIDE;
	return false;
}

bool ContexteValidationCode::valide_structure(NoeudStruct *decl)
{
	auto &graphe = m_compilatrice.graphe_dependance;

	auto noeud_dependance = graphe.cree_noeud_type(decl->type);
	noeud_dependance->noeud_syntactique = decl;
	decl->noeud_dependance = noeud_dependance;

	if (decl->est_externe && decl->bloc == nullptr) {
		return false;
	}

	if (decl->bloc->membres.est_vide()) {
		rapporte_erreur("Bloc vide pour la déclaration de structure", decl);
		return true;
	}

	auto decl_precedente = trouve_dans_bloc(decl->bloc_parent, decl);

	// la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les structres (p.e. stat)
	// @vérifie si utile
	if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
		rapporte_erreur_redefinition_symbole(decl, decl_precedente);
		return true;
	}

	auto type_struct = static_cast<TypeStructure *>(decl->type);
	// @réinitialise en cas d'erreurs passées
	type_struct->membres = kuri::tableau<TypeCompose::Membre>();
	type_struct->membres.reserve(decl->bloc->membres.taille);

	auto verifie_inclusion_valeur = [&decl, this](NoeudExpression *enf)
	{
		if (enf->type == decl->type) {
			rapporte_erreur("Ne peut inclure la structure dans elle-même par valeur", enf, erreur::type_erreur::TYPE_ARGUMENT);
			return true;
		}

		auto type_base = enf->type;

		if (type_base->genre == GenreType::TABLEAU_FIXE) {
			auto type_deref = type_dereference_pour(type_base);

			if (type_deref == decl->type) {
				rapporte_erreur("Ne peut inclure la structure dans elle-même par valeur", enf, erreur::type_erreur::TYPE_ARGUMENT);
				return true;
			}
		}

		return false;
	};

	auto decalage = 0u;
	auto max_alignement = 0u;

	auto ajoute_donnees_membre = [&, this](NoeudExpression *enfant, NoeudExpression *expr_valeur)
	{
		auto type_membre = enfant->type;
		auto align_type = type_membre->alignement;

		if (align_type == 0) {
			rapporte_erreur("impossible de définir l'alignement du type", enfant);
			return true;
		}

		if (type_membre->taille_octet == 0) {
			rapporte_erreur("impossible de définir la taille du type", enfant);
			return true;
		}

		max_alignement = std::max(align_type, max_alignement);
		auto padding = (align_type - (decalage % align_type)) % align_type;
		decalage += padding;

		type_struct->membres.pousse({ enfant->type, enfant->ident->nom, decalage, 0, expr_valeur });

		donnees_dependance.types_utilises.insere(type_membre);

		decalage += type_membre->taille_octet;
		return false;
	};

	for (auto i = unite->index_reprise; i < decl->arbre_aplatis.taille; ++i) {
		if (valide_semantique_noeud(decl->arbre_aplatis[i])) {
			unite->index_reprise = i;
			graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
			return true;
		}
	}

	if (decl->est_union) {
		auto type_union = static_cast<TypeUnion *>(decl->type);
		type_union->est_nonsure = decl->est_nonsure;

		POUR (decl->bloc->membres) {
			auto decl_var = static_cast<NoeudDeclarationVariable *>(it);
			auto decl_membre = decl_var->valeur;

			if (decl_membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				rapporte_erreur("Expression invalide dans la déclaration du membre de l'union", decl_membre);
				return true;
			}

			if (decl_membre->type->genre == GenreType::RIEN) {
				rapporte_erreur("Ne peut avoir un type « rien » dans une union", decl_membre, erreur::type_erreur::TYPE_DIFFERENTS);
				return true;
			}

			if (decl_membre->type->genre == GenreType::STRUCTURE || decl_membre->type->genre == GenreType::UNION) {
				if ((decl_membre->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
					unite->etat = UniteCompilation::Etat::ATTEND_SUR_TYPE;
					unite->type_attendu = decl_membre->type;
					return true;
				}
			}

			if (verifie_inclusion_valeur(decl_var)) {
				return true;
			}

			if (ajoute_donnees_membre(decl_membre, decl_var->expression)) {
				return true;
			}
		}

		auto taille_union = 0u;

		POUR (decl->bloc->membres) {
			auto type_membre = it->type;
			auto taille = type_membre->taille_octet;

			if (taille > taille_union) {
				type_union->type_le_plus_grand = type_membre;
				taille_union = taille;
			}
		}

		/* Pour les unions sûres, il nous faut prendre en compte le
		 * membre supplémentaire. */
		if (!decl->est_nonsure) {
			/* ajoute une marge d'alignement */
			auto padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
			taille_union += padding;

			type_union->decalage_index = taille_union;

			/* ajoute la taille du membre actif */
			taille_union += static_cast<unsigned>(sizeof(int));

			/* ajoute une marge d'alignement finale */
			padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
			taille_union += padding;
		}

		decl->type->taille_octet = taille_union;
		decl->type->alignement = max_alignement;
		decl->type->drapeaux |= TYPE_FUT_VALIDE;

		graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
		return false;
	}

	POUR (decl->bloc->expressions) {
		if (dls::outils::est_element(it->genre, GenreNoeud::DECLARATION_STRUCTURE, GenreNoeud::DECLARATION_ENUM)) {
			// utilisation d'un type de données afin de pouvoir automatiquement déterminer un type
			auto type_de_donnees = m_compilatrice.typeuse.type_type_de_donnees(it->type);
			type_struct->membres.pousse({ type_de_donnees, it->ident->nom, decalage, 0, nullptr, TypeCompose::Membre::EST_CONSTANT });

			// l'utilisation d'un type de données brise le graphe de dépendance
			donnees_dependance.types_utilises.insere(it->type);
			continue;
		}

		if (it->genre != GenreNoeud::DECLARATION_VARIABLE && it->genre != GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
			rapporte_erreur("Déclaration inattendu dans le bloc de la structure", it);
			return true;
		}

		if (it->genre == GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
			auto expr_assign = static_cast<NoeudExpressionBinaire *>(it);
			auto variable = expr_assign->expr1;

			for (auto &membre : type_struct->membres) {
				if (membre.nom == variable->ident->nom) {
					membre.expression_valeur_defaut = expr_assign->expr2;
				}
			}

			continue;
		}

		auto decl_var = static_cast<NoeudDeclarationVariable *>(it);
		auto decl_membre = decl_var->valeur;

		if (decl_var->drapeaux & EST_CONSTANTE) {
			type_struct->membres.pousse({ it->type, it->ident->nom, decalage, 0, decl_var->expression, TypeCompose::Membre::EST_CONSTANT });
			continue;
		}

		if (decl_membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			rapporte_erreur("Expression invalide dans la déclaration du membre de la structure", decl_membre);
			return true;
		}

		if (decl_membre->type->genre == GenreType::RIEN) {
			rapporte_erreur("Ne peut avoir un type « rien » dans une structure", decl_membre, erreur::type_erreur::TYPE_DIFFERENTS);
			return true;
		}

		if (decl_membre->type->genre == GenreType::STRUCTURE || decl_membre->type->genre == GenreType::UNION) {
			if ((decl_membre->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
				unite->etat = UniteCompilation::Etat::ATTEND_SUR_TYPE;
				unite->type_attendu = decl_membre->type;
				return true;
			}
		}

		if (verifie_inclusion_valeur(decl_membre)) {
			return true;
		}

		// À FAIRE : préserve l'emploi dans les données types
		if (decl_membre->drapeaux & EMPLOYE) {
			if (decl_membre->type->genre != GenreType::STRUCTURE) {
				rapporte_erreur("Ne peut employer un type n'étant pas une structure", decl_membre);
				return true;
			}

			for (auto it_type : type_struct->types_employes) {
				if (decl_membre->type == it_type) {
					rapporte_erreur("Ne peut employer plusieurs fois le même type", decl_membre);
					return true;
				}
			}

			auto type_struct_empl = static_cast<TypeStructure *>(decl_membre->type);
			type_struct->types_employes.pousse(type_struct_empl);

			auto decl_struct_empl = type_struct_empl->decl;

			type_struct->membres.reserve(type_struct->membres.taille + decl_struct_empl->bloc->membres.taille);

			for (auto decl_it_empl : decl_struct_empl->bloc->membres) {
				auto it_empl = static_cast<NoeudDeclarationVariable *>(decl_it_empl);
				if (ajoute_donnees_membre(it_empl->valeur, it_empl->expression)) {
					return true;
				}
			}
		}
		else {
			if (ajoute_donnees_membre(decl_membre, decl_var->expression)) {
				return true;
			}
		}
	}

	auto padding = (max_alignement - (decalage % max_alignement)) % max_alignement;
	decalage += padding;
	decl->type->taille_octet = decalage;
	decl->type->alignement = max_alignement;
	decl->type->drapeaux |= TYPE_FUT_VALIDE;
	decl->drapeaux |= DECLARATION_FUT_VALIDEE;

	graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
	return false;
}

/* ************************************************************************** */

bool ContexteValidationCode::resoud_type_final(NoeudExpression *expression_type, Type *&type_final)
{
	PROFILE_FONCTION;

	if (expression_type == nullptr) {
		type_final = nullptr;
		return false;
	}

	auto type_var = expression_type->type;

	if (type_var->genre != GenreType::TYPE_DE_DONNEES) {
		rapporte_erreur("attendu un type de données", expression_type);
		return true;
	}

	auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type_var);

	if (type_de_donnees->type_connu == nullptr) {
		rapporte_erreur("impossible de définir le type selon l'expression", expression_type);
		return true;
	}

	type_final = type_de_donnees->type_connu;
	return false;
}

void ContexteValidationCode::rapporte_erreur(const char *message, NoeudExpression *noeud)
{
	erreur::lance_erreur(message, m_compilatrice, noeud->lexeme);
}

void ContexteValidationCode::rapporte_erreur(const char *message, NoeudExpression *noeud, erreur::type_erreur type_erreur)
{
	erreur::lance_erreur(message, m_compilatrice, noeud->lexeme, type_erreur);
}

void ContexteValidationCode::rapporte_erreur_redefinition_symbole(NoeudExpression *decl, NoeudDeclaration *decl_prec)
{
	erreur::redefinition_symbole(m_compilatrice, decl->lexeme, decl_prec->lexeme);
}

void ContexteValidationCode::rapporte_erreur_redefinition_fonction(NoeudDeclarationFonction *decl, NoeudDeclaration *decl_prec)
{
	erreur::redefinition_fonction(m_compilatrice, decl_prec->lexeme, decl->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf)
{
	erreur::lance_erreur_type_arguments(type_arg->type, type_enf->type, m_compilatrice, type_enf->lexeme, type_arg->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_retour(const Type *type_arg, const Type *type_enf, NoeudBase *racine)
{
	erreur::lance_erreur_type_retour(type_arg, type_enf, m_compilatrice, racine);
}

void ContexteValidationCode::rapporte_erreur_assignation_type_differents(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud)
{
	erreur::lance_erreur_assignation_type_differents(type_gauche, type_droite, m_compilatrice, noeud->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_operation(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud)
{
	erreur::lance_erreur_type_operation(type_gauche, type_droite, m_compilatrice, noeud->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_indexage(NoeudExpression *noeud)
{
	erreur::type_indexage(m_compilatrice, noeud);
}

void ContexteValidationCode::rapporte_erreur_type_operation(NoeudExpression *noeud)
{
	erreur::lance_erreur_type_operation(m_compilatrice, noeud);
}

void ContexteValidationCode::rapporte_erreur_type_operation_unaire(NoeudExpression *noeud)
{
	erreur::lance_erreur_type_operation_unaire(m_compilatrice, noeud);
}

void ContexteValidationCode::rapporte_erreur_acces_hors_limites(NoeudBase *b, TypeTableauFixe *type_tableau, long index_acces)
{
	erreur::lance_erreur_acces_hors_limites(m_compilatrice, b, type_tableau->taille, type_tableau, index_acces);
}

void ContexteValidationCode::rapporte_erreur_membre_inconnu(NoeudBase *acces, NoeudBase *structure, NoeudBase *membre, TypeCompose *type)
{
	erreur::membre_inconnu(m_compilatrice, acces, structure, membre, type);
}

void ContexteValidationCode::rapporte_erreur_membre_inactif(NoeudBase *acces, NoeudBase *structure, NoeudBase *membre)
{
	erreur::membre_inactif(m_compilatrice, *this, acces, structure, membre);
}

void ContexteValidationCode::rapporte_erreur_valeur_manquante_discr(NoeudBase *expression, dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes)
{
	erreur::valeur_manquante_discr(m_compilatrice, expression, valeurs_manquantes);
}

void ContexteValidationCode::rapporte_erreur_fonction_inconnue(NoeudBase *b, const dls::tablet<DonneesCandidate, 10> &candidates)
{
	erreur::lance_erreur_fonction_inconnue(m_compilatrice, b, candidates);
}

void ContexteValidationCode::rapporte_erreur_fonction_nulctx(const NoeudBase *appl_fonc, const NoeudBase *decl_fonc, const NoeudBase *decl_appel)
{
	erreur::lance_erreur_fonction_nulctx(m_compilatrice, appl_fonc, decl_fonc, decl_appel);
}
