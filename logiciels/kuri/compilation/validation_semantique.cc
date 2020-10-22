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
#include "biblinternes/outils/assert.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntaxique.hh"
#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "compilatrice.hh"
#include "erreur.h"
#include "outils_lexemes.hh"
#include "profilage.hh"
#include "portee.hh"
#include "tacheronne.hh"
#include "validation_expression_appel.hh"

using dls::outils::possede_drapeau;

/* ************************************************************************** */

#define VERIFIE_INTERFACE_KURI_CHARGEE(nom) \
	if (espace->interface_kuri->decl_##nom == nullptr) {\
		unite->attend_sur_interface_kuri(#nom); \
		return true; \
	} \
	else if (espace->interface_kuri->decl_##nom->corps->unite == nullptr) { \
		m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, espace->interface_kuri->decl_##nom->corps); \
	}

#define VERIFIE_UNITE_TYPAGE(type) \
	if (type->est_enum() || type->est_erreur()) { \
		if (static_cast<TypeEnum *>(type)->decl->unite == nullptr) { \
			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, static_cast<TypeEnum *>(type)->decl); \
		} \
	} \
	else if (type->est_structure()) { \
		if (type->comme_structure()->decl->unite == nullptr) { \
			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, type->comme_structure()->decl); \
		} \
	} \
	else if (type->est_union()) { \
		if (type->comme_union()->decl->unite == nullptr) { \
			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, type->comme_union()->decl); \
		} \
	}

ContexteValidationCode::ContexteValidationCode(Compilatrice &compilatrice, Tacheronne &tacheronne, UniteCompilation &u)
	: m_compilatrice(compilatrice)
	, m_tacheronne(tacheronne)
	, unite(&u)
	, espace(unite->espace)
{}

void ContexteValidationCode::commence_fonction(NoeudDeclarationEnteteFonction *fonction)
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
	Prof(valide_semantique_noeud);

	switch (noeud->genre) {
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_CHARGE:
		{
			auto inst = noeud->comme_charge();
			auto lexeme = inst->expr->lexeme;
			auto fichier = espace->fichier(inst->lexeme->fichier);
			auto temps = dls::chrono::compte_seconde();
			m_compilatrice.ajoute_fichier_a_la_compilation(espace, lexeme->chaine, fichier->module, *lexeme);
			temps_chargement += temps.temps();
			break;
		}
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			auto inst = noeud->comme_importe();
			auto lexeme = inst->expr->lexeme;
			auto fichier = espace->fichier(inst->lexeme->fichier);
			auto temps = dls::chrono::compte_seconde();
			auto module = m_compilatrice.importe_module(espace, dls::chaine(lexeme->chaine), *lexeme);
			temps_chargement += temps.temps();
			// @concurrence critique
			fichier->modules_importes.insere(module->nom);
			break;
		}
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		{
			auto decl = noeud->comme_entete_fonction();

			if (decl->est_declaration_type) {
				POUR (decl->arbre_aplatis) {
					// voir commentaire plus bas
					if (it->est_decl_var()) {
						auto valeur = it->comme_decl_var()->valeur;
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

				auto requiers_contexte = !decl->possede_drapeau(FORCE_NULCTX);
				auto types_entrees = dls::tablet<Type *, 6>(decl->params.taille + requiers_contexte);

				if (requiers_contexte) {
					types_entrees[0] = espace->typeuse.type_contexte;
				}

				for (auto i = 0; i < decl->params.taille; ++i) {
					// le syntaxage des expressions des entrées des fonctions est commune
					// aux déclarations des fonctions et des types de fonctions faisant
					// qu'une chaine de caractère seule (référence d'un type) est considérée
					// comme étant une déclaration de variable, il nous faut donc extraire
					// le noeud de référence à la variable afin de valider correctement le type
					NoeudExpression *type_entree = decl->params[i];

					if (type_entree->est_decl_var()) {
						type_entree = type_entree->comme_decl_var()->valeur;
					}

					if (resoud_type_final(type_entree, types_entrees[i + requiers_contexte])) {
						return true;
					}
				}

				auto types_sorties = dls::tablet<Type *, 6>(decl->params_sorties.taille);

				for (auto i = 0; i < decl->params_sorties.taille; ++i) {
					if (resoud_type_final(decl->params_sorties[i], types_sorties[i])) {
						return true;
					}
				}

				auto type_fonction = espace->typeuse.type_fonction(types_entrees, types_sorties);
				decl->type = espace->typeuse.type_type_de_donnees(type_fonction);
				return false;
			}

			break;
		}
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			auto decl = noeud->comme_corps_fonction();

			if (decl->entete->est_operateur) {
				return valide_operateur(decl);
			}

			return valide_fonction(decl);
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = noeud->comme_appel();
			expr->genre_valeur = GenreValeur::DROITE;
			// @réinitialise en cas d'erreurs passées
			expr->exprs = kuri::tableau<NoeudExpression *>();
			return valide_appel_fonction(m_compilatrice, *espace, *this, expr);
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto noeud_directive = noeud->comme_execute();

			// crée une fonction pour l'exécution
			auto decl_entete = static_cast<NoeudDeclarationEnteteFonction *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_ENTETE_FONCTION, noeud->lexeme));
			auto decl_corps  = decl_entete->corps;

			decl_entete->bloc_parent = noeud->bloc_parent;
			decl_corps->bloc_parent = noeud->bloc_parent;

			decl_entete->drapeaux |= FORCE_NULCTX;
			decl_entete->est_metaprogramme = true;

			// le type de la fonction est fonc () -> (type_expression)
			auto decl_sortie = m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme)->comme_decl_var();
			decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
			decl_sortie->type = noeud_directive->expr->type;
			decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;

			decl_entete->params_sorties.pousse(decl_sortie);

			auto types_entrees = dls::tablet<Type *, 6>(0);

			auto types_sorties = dls::tablet<Type *, 6>(1);
			types_sorties[0] = noeud_directive->expr->type;

			auto type_fonction = espace->typeuse.type_fonction(types_entrees, types_sorties);
			decl_entete->type = type_fonction;

			decl_corps->bloc = static_cast<NoeudBloc *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, noeud->lexeme));

			static Lexeme lexeme_retourne = { "retourne", {}, GenreLexeme::RETOURNE, 0, 0, 0 };
			auto expr_ret = static_cast<NoeudRetour *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::INSTRUCTION_RETOUR, &lexeme_retourne));

			if (noeud_directive->expr->type != espace->typeuse[TypeBase::RIEN]) {
				expr_ret->genre = GenreNoeud::INSTRUCTION_RETOUR;
				expr_ret->expr = noeud_directive->expr;

				/* besoin de valider pour mettre en place les informations de retour */
				auto ancienne_fonction_courante = fonction_courante;
				fonction_courante = decl_entete;
				valide_expression_retour(expr_ret);
				fonction_courante = ancienne_fonction_courante;
			}
			else {
				decl_corps->bloc->expressions->pousse(noeud_directive->expr);
			}

			decl_corps->bloc->expressions->pousse(expr_ret);

			auto graphe = espace->graphe_dependance.verrou_ecriture();
			auto noeud_dep = graphe->cree_noeud_fonction(decl_entete);
			graphe->ajoute_dependances(*noeud_dep, donnees_dependance);

			decl_entete->drapeaux |= DECLARATION_FUT_VALIDEE;

			auto metaprogramme = espace->cree_metaprogramme();
			metaprogramme->directive = noeud_directive;
			metaprogramme->fonction = decl_entete;

			m_compilatrice.ordonnanceuse->cree_tache_pour_execution(espace, metaprogramme);
			m_compilatrice.ordonnanceuse->cree_tache_pour_generation_ri(espace, decl_corps);

			if (fonction_courante) {
				/* avance l'index car il est inutile de revalidé ce noeud */
				unite->index_courant += 1;
				unite->attend_sur_metaprogramme(metaprogramme);
				return true;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			CHRONO_TYPAGE(m_tacheronne.stats_typage.ref_decl, "valide référence déclaration");
			auto expr = noeud->comme_ref_decl();

			if (expr->possede_drapeau(DECLARATION_TYPE_POLYMORPHIQUE)) {
				expr->genre_valeur = GenreValeur::DROITE;

				if (fonction_courante && fonction_courante->est_monomorphisation) {
					auto type_instantie = Type::nul();

					for (auto &paire : fonction_courante->paires_expansion_gabarit) {
						if (paire.first == expr->ident->nom) {
							type_instantie = paire.second;
						}
					}

					if (type_instantie == nullptr) {
						rapporte_erreur("impossible de définir le type de l'instantiation polymorphique", expr);
						return true;
					}

					expr->type = espace->typeuse.type_type_de_donnees(type_instantie);
					return false;
				}

				auto type_poly = espace->typeuse.cree_polymorphique(expr->ident);
				expr->type = espace->typeuse.type_type_de_donnees(type_poly);
				return false;
			}

			expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto bloc = expr->bloc_parent;
			assert(bloc != nullptr);

			auto fichier = espace->fichier(expr->lexeme->fichier);
			auto decl = trouve_dans_bloc_ou_module(*espace, bloc, expr->ident, fichier);

			if (decl == nullptr) {
				unite->attend_sur_symbole(expr->lexeme);
				return true;
			}

			if (decl->lexeme->fichier == expr->lexeme->fichier && decl->genre == GenreNoeud::DECLARATION_VARIABLE && !decl->possede_drapeau(EST_GLOBALE)) {
				if (decl->lexeme->ligne > expr->lexeme->ligne) {
					rapporte_erreur("Utilisation d'une variable avant sa déclaration", expr);
					return true;
				}
			}

			if (dls::outils::est_element(decl->genre, GenreNoeud::DECLARATION_ENUM, GenreNoeud::DECLARATION_STRUCTURE) && expr->aide_generation_code != EST_NOEUD_ACCES) {
				if (decl->unite == nullptr) {
					m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl);
				}
				expr->type = espace->typeuse.type_type_de_donnees(decl->type);
				expr->decl = decl;
			}
			else {				
				if (!decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
					if (decl->unite == nullptr) {
						m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl);
					}
					unite->attend_sur_declaration(decl);
					return true;
				}

				// les fonctions peuvent ne pas avoir de type au moment si elles sont des appels polymorphiques
				assert(decl->type || decl->genre == GenreNoeud::DECLARATION_ENTETE_FONCTION);
				expr->decl = decl;
				expr->type = decl->type;
			}

			if (decl->possede_drapeau(EST_CONSTANTE)) {
				expr->genre_valeur = GenreValeur::DROITE;
			}

			// uniquement pour les fonctions polymorphiques
			if (expr->type) {
				donnees_dependance.types_utilises.insere(expr->type);
			}

			if (decl->est_entete_fonction() && !decl->comme_entete_fonction()->est_polymorphe) {
				noeud->genre_valeur = GenreValeur::DROITE;
				auto decl_fonc = decl->comme_entete_fonction();
				donnees_dependance.fonctions_utilisees.insere(decl_fonc);

				if (decl_fonc->corps->unite == nullptr && !decl_fonc->est_externe) {
					m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl_fonc->corps);
				}
			}
			else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (decl->possede_drapeau(EST_GLOBALE)) {
					donnees_dependance.globales_utilisees.insere(decl->comme_decl_var());
				}
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			auto type_connu = espace->typeuse.type_pour_lexeme(noeud->lexeme->genre);
			auto type_type = espace->typeuse.type_type_de_donnees(type_connu);
			noeud->type = type_type;
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto inst = noeud->comme_ref_membre();
			auto enfant1 = inst->accede;
			//auto enfant2 = inst->membre;
			noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;

			if (enfant1->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto fichier = espace->fichier(noeud->lexeme->fichier);

				auto const nom_symbole = enfant1->ident->nom;
				if (fichier->importe_module(nom_symbole)) {
					/* À FAIRE(réusinage arbre) */
//					auto module_importe = m_compilatrice.module(nom_symbole);

//					if (module_importe == nullptr) {
//						rapporte_erreur(
//									"module inconnu",
//									compilatrice,
//									enfant1->lexeme,
//									erreur::Genre::MODULE_INCONNU);
//					}

//					auto const nom_fonction = enfant2->ident->nom;

//					if (!module_importe->possede_fonction(nom_fonction)) {
//						rapporte_erreur(
//									"Le module ne possède pas la fonction",
//									compilatrice,
//									enfant2->lexeme,
//									erreur::Genre::FONCTION_INCONNUE);
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
			auto inst = noeud->comme_assignation();
			return valide_assignation(inst);
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			return valide_declaration_variable(noeud->comme_decl_var());
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = espace->typeuse[TypeBase::R32];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = espace->typeuse[TypeBase::ENTIER_CONSTANT];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(noeud);
			expr->genre_valeur = GenreValeur::DROITE;
			CHRONO_TYPAGE(m_tacheronne.stats_typage.operateurs_binaire, "opérateur binaire");

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

				auto type_de_donnees = type2->comme_type_de_donnees();
				auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;

				auto taille_tableau = 0l;

				if (expression_taille) {
					auto res = evalue_expression(espace, expression_taille->bloc_parent, expression_taille);

					if (res.est_errone) {
						rapporte_erreur("Impossible d'évaluer la taille du tableau", expression_taille);
						return true;
					}

					if (res.type != TypeExpression::ENTIER) {
						rapporte_erreur("L'expression n'est pas de type entier", expression_taille);
						return true;
					}

					taille_tableau = res.entier;
				}

				if (taille_tableau != 0) {
					auto type_tableau = espace->typeuse.type_tableau_fixe(type_connu, taille_tableau);
					expr->type = espace->typeuse.type_type_de_donnees(type_tableau);
					donnees_dependance.types_utilises.insere(type_tableau);
				}
				else {
					auto type_tableau = espace->typeuse.type_tableau_dynamique(type_connu);
					expr->type = espace->typeuse.type_type_de_donnees(type_tableau);
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

				auto type_type1 = type1->comme_type_de_donnees();
				auto type_type2 = type2->comme_type_de_donnees();

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

						auto type_union = espace->typeuse.union_anonyme(std::move(membres));
						expr->type = espace->typeuse.type_type_de_donnees(type_union);
						donnees_dependance.types_utilises.insere(type_union);

						// @concurrence critique
						if (type_union->decl == nullptr) {
							static Lexeme lexeme_union = { "anonyme", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
							auto decl_struct = static_cast<NoeudStruct *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_STRUCTURE, &lexeme_union));
							decl_struct->type = type_union;

							type_union->decl = decl_struct;

							m_compilatrice.ordonnanceuse->cree_tache_pour_generation_ri(espace, decl_struct);
						}

						return false;
					}
					case GenreLexeme::EGALITE:
					{
						// XXX - aucune raison de prendre un verrou ici
						auto op = espace->operateurs->op_comp_egal_types;
						expr->type = op->type_resultat;
						expr->op = op;
						donnees_dependance.types_utilises.insere(expr->type);
						return false;
					}
					case GenreLexeme::DIFFERENCE:
					{
						// XXX - aucune raison de prendre un verrou ici
						auto op = espace->operateurs->op_comp_diff_types;
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
				expr->type = espace->typeuse[TypeBase::BOOL];

				auto enfant_expr = static_cast<NoeudExpressionBinaire *>(enfant1);
				type1 = enfant_expr->expr2->type;

				auto candidats = dls::tablet<OperateurCandidat, 10>();
				if (cherche_candidats_operateurs(*espace, *this, type1, type2, type_op, candidats)) {
					return true;
				}
				auto meilleur_candidat = OperateurCandidat::nul_const();
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					unite->attend_sur_operateur(noeud);
					return true;
				}

				expr->op = meilleur_candidat->op;
				transtype_si_necessaire(expr->expr1, meilleur_candidat->transformation_type1);
				transtype_si_necessaire(expr->expr2, meilleur_candidat->transformation_type2);

				if (!expr->op->est_basique) {
					auto decl_op = expr->op->decl;
					donnees_dependance.fonctions_utilisees.insere(expr->op->decl);

					if (decl_op->corps->unite == nullptr) {
						m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl_op->corps);
					}
				}
			}
			else {
				if (assignation_composee) {
					type_op = operateur_pour_assignation_composee(type_op);
					expr->drapeaux |= EST_ASSIGNATION_COMPOSEE;

					// exclue les arithmétiques de pointeur
					if (!(type1->genre == GenreType::POINTEUR && (est_type_entier(type2) || type2->genre == GenreType::ENTIER_CONSTANT))) {

						auto transformation = TransformationType();
						if (cherche_transformation(*espace, *this, type2, type1, transformation)) {
							return true;
						}

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							rapporte_erreur_assignation_type_differents(type1, type2, enfant2);
							return true;
						}
					}
				}

				auto candidats = dls::tablet<OperateurCandidat, 10>();
				if (cherche_candidats_operateurs(*espace, *this, type1, type2, type_op, candidats)) {
					return true;
				}
				auto meilleur_candidat = OperateurCandidat::nul_const();
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					unite->attend_sur_operateur(noeud);
					return true;
				}

				expr->type = meilleur_candidat->op->type_resultat;
				expr->op = meilleur_candidat->op;
				transtype_si_necessaire(expr->expr1, meilleur_candidat->transformation_type1);
				transtype_si_necessaire(expr->expr2, meilleur_candidat->transformation_type2);

				if (!expr->op->est_basique) {
					auto decl_op = expr->op->decl;
					donnees_dependance.fonctions_utilisees.insere(decl_op);

					if (decl_op->corps->unite == nullptr) {
						m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl_op->corps);
					}
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			CHRONO_TYPAGE(m_tacheronne.stats_typage.operateurs_unaire, "opérateur unaire");
			auto expr = noeud->comme_operateur_unaire();
			expr->genre_valeur = GenreValeur::DROITE;

			auto enfant = expr->expr;
			auto type = enfant->type;

			if (type->est_type_de_donnees() && dls::outils::est_element(expr->lexeme->genre, GenreLexeme::FOIS_UNAIRE, GenreLexeme::ESP_UNAIRE)) {
				auto type_de_donnees = type->comme_type_de_donnees();
				auto type_connu = type_de_donnees->type_connu;

				if (type_connu == nullptr) {
					type_connu = type_de_donnees;
				}

				if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
					type_connu = espace->typeuse.type_pointeur_pour(type_connu);
				}
				else if (expr->lexeme->genre == GenreLexeme::ESP_UNAIRE) {
					type_connu = espace->typeuse.type_reference_pour(type_connu);
				}

				noeud->type = espace->typeuse.type_type_de_donnees(type_connu);
				break;
			}

			if (type->genre == GenreType::REFERENCE) {
				type = type_dereference_pour(type);
				transtype_si_necessaire(expr->expr, TypeTransformation::DEREFERENCE);
			}

			if (expr->type == nullptr) {
				if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
					if (!est_valeur_gauche(enfant->genre_valeur)) {
						rapporte_erreur("Ne peut pas prendre l'adresse d'une valeur-droite.", enfant);
						return true;
					}

					expr->type = espace->typeuse.type_pointeur_pour(type);
				}
				else if (expr->lexeme->genre == GenreLexeme::EXCLAMATION) {
					if (!est_type_conditionnable(enfant->type)) {
						rapporte_erreur("Ne peut pas appliquer l'opérateur « ! » au type de l'expression", enfant);
						return true;
					}

					expr->type = espace->typeuse[TypeBase::BOOL];
				}
				else {
					if (type->genre == GenreType::ENTIER_CONSTANT) {
						type = espace->typeuse[TypeBase::Z32];
						transtype_si_necessaire(expr->expr, { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type });
					}

					auto operateurs = espace->operateurs.verrou_lecture();
					auto op = cherche_operateur_unaire(*operateurs, type, expr->lexeme->genre);

					if (op == nullptr) {
						unite->attend_sur_operateur(noeud);
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
				transtype_si_necessaire(expr->expr1, TypeTransformation::DEREFERENCE);
				type1 = type_dereference_pour(type1);
			}

			switch (type1->genre) {
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					expr->type = type_dereference_pour(type1);
					VERIFIE_INTERFACE_KURI_CHARGEE(panique_tableau);
					donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_tableau);
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					auto type_tabl = type1->comme_tableau_fixe();
					expr->type = type_dereference_pour(type1);

					auto res = evalue_expression(espace, enfant2->bloc_parent, enfant2);

					if (!res.est_errone) {
						if (res.entier >= type_tabl->taille) {
							rapporte_erreur_acces_hors_limites(enfant2, type_tabl, res.entier);
							return true;
						}

						/* nous savons que l'accès est dans les limites,
						 * évite d'émettre le code de vérification */
						expr->aide_generation_code = IGNORE_VERIFICATION;
					}

					if (expr->aide_generation_code != IGNORE_VERIFICATION) {
						VERIFIE_INTERFACE_KURI_CHARGEE(panique_tableau);
						donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_tableau);
					}

					break;
				}
				case GenreType::POINTEUR:
				{
					expr->type = type_dereference_pour(type1);
					break;
				}
				case GenreType::CHAINE:
				{
					expr->type = espace->typeuse[TypeBase::Z8];
					VERIFIE_INTERFACE_KURI_CHARGEE(panique_chaine);
					donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_chaine);
					break;
				}
				default:
				{
					dls::flux_chaine ss;
					ss << "Le type '" << chaine_type(type1)
					   << "' ne peut être déréférencé par opérateur[] !";
					rapporte_erreur(ss.chn().c_str(), noeud, erreur::Genre::TYPE_DIFFERENTS);
					return true;
				}
			}

			auto type_cible = espace->typeuse[TypeBase::Z64];
			auto type_index = enfant2->type;

			if (type_index->est_entier_naturel() || type_index->est_octet()) {
				transtype_si_necessaire(expr->expr2, { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible });
			}
			else {
				if (type_index->genre == GenreType::ENUM) {
					type_index = type_index->comme_enum()->type_donnees;
				}

				if (!transtype_si_necessaire(expr->expr2, type_cible)) {
					return true;
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = noeud->comme_retour();
			noeud->genre_valeur = GenreValeur::DROITE;
			return valide_expression_retour(inst);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			noeud->type = espace->typeuse[TypeBase::CHAINE];
			noeud->genre_valeur = GenreValeur::DROITE;
			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = espace->typeuse[TypeBase::BOOL];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = espace->typeuse[TypeBase::Z8];
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
				rapporte_erreur("Impossible de conditionner le type de l'expression 'si'", inst->condition, erreur::Genre::TYPE_DIFFERENTS);
				return true;
			}

			/* pour les expressions x = si y { z } sinon { w } */
			inst->type = inst->bloc_si_vrai->type;

			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = noeud->comme_si_statique();

			if (inst->visite == false) {
				auto res = evalue_expression(espace, inst->bloc_parent, inst->condition);

				if (res.est_errone) {
					rapporte_erreur(res.message_erreur, res.noeud_erreur, erreur::Genre::VARIABLE_REDEFINIE);
					return true;
				}

				auto condition_est_vraie = res.entier != 0;
				inst->condition_est_vraie = condition_est_vraie;

				if (!condition_est_vraie) {
					// dis à l'unité de sauter les instructions jusqu'au prochain point
					unite->index_courant = inst->index_bloc_si_faux;
				}
			}
			else {
				// dis à l'unité de sauter les instructions jusqu'au prochain point
				unite->index_courant = inst->index_apres;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto inst = noeud->comme_bloc();

			auto expressions = inst->expressions.verrou_lecture();

			if (expressions->est_vide()) {
				noeud->type = espace->typeuse[TypeBase::RIEN];
			}
			else {
				noeud->type = expressions->a(expressions->taille - 1)->type;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto inst = noeud->comme_pour();

			auto enfant1 = inst->variable;
			auto enfant2 = inst->expression;
			auto enfant3 = inst->bloc;
			auto feuilles = enfant1->comme_virgule();

			for (auto f : feuilles->expressions) {
				auto decl_f = trouve_dans_bloc(noeud->bloc_parent, f->ident);

				if (decl_f != nullptr) {
					if (f->lexeme->ligne > decl_f->lexeme->ligne) {
						rapporte_erreur("Redéfinition de la variable", f, erreur::Genre::VARIABLE_REDEFINIE);
						return true;
					}
				}
			}

			auto variable = feuilles->expressions[0];
			inst->ident = variable->ident;

			auto requiers_index = feuilles->expressions.taille == 2;

			auto type = enfant2->type;

			auto determine_iterande = [&, this](NoeudExpression *iterand) -> char {
				/* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
				 * type de retour d'une coroutine n'interfère avec le type d'une
				 * variable (par exemple quand nous retournons une chaine). */
				if (iterand->est_plage()) {
					if (requiers_index) {
						return GENERE_BOUCLE_PLAGE_INDEX;
					}

					return GENERE_BOUCLE_PLAGE;
				}

				if (iterand->est_appel()) {
					auto appel = iterand->comme_appel();
					auto fonction_appelee = appel->noeud_fonction_appelee;

					if (fonction_appelee->est_entete_fonction()) {
						auto entete = fonction_appelee->comme_entete_fonction();

						if (entete->est_coroutine) {
							::rapporte_erreur(espace, enfant2, "Les coroutines ne sont plus supportés dans le langage pour le moment");
//							enfant1->type = enfant2->type;

//							df = enfant2->df;
//							auto nombre_vars_ret = df->idx_types_retours.taille();

//							if (feuilles.taille() == nombre_vars_ret) {
//								requiers_index = false;
//								noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE;
//							}
//							else if (feuilles.taille() == nombre_vars_ret + 1) {
//								requiers_index = true;
//								noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
//							}
//							else {
//								rapporte_erreur(
//											"Mauvais compte d'arguments à déployer",
//											compilatrice,
//											*enfant1->lexeme);
//							}
						}
					}
				}

				if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::VARIADIQUE) {
					type = type_dereference_pour(type);

					if (requiers_index) {
						return GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						return GENERE_BOUCLE_TABLEAU;
					}
				}
				else if (type->genre == GenreType::CHAINE) {
					type = espace->typeuse[TypeBase::Z8];
					enfant1->type = type;

					if (requiers_index) {
						return GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						return GENERE_BOUCLE_TABLEAU;
					}
				}
				else {
					::rapporte_erreur(espace, enfant2, "Le type de la variable n'est pas itérable")
							.ajoute_message("Note : le type de la variable est ")
							.ajoute_message(chaine_type(type))
							.ajoute_message("\n");
					return -1;
				}
			};

			auto aide_generation_code = determine_iterande(enfant2);

			if (aide_generation_code == -1) {
				return true;
			}

			noeud->aide_generation_code = aide_generation_code;

			donnees_dependance.types_utilises.insere(type);
			enfant3->membres->reserve(feuilles->expressions.taille);

			auto nombre_feuilles = feuilles->expressions.taille - requiers_index;

			for (auto i = 0l; i < nombre_feuilles; ++i) {
				auto decl_f = feuilles->expressions[i]->comme_decl_var();

				decl_f->type = type;
				decl_f->drapeaux |= DECLARATION_FUT_VALIDEE;

				enfant3->membres->pousse(decl_f);
			}

			if (requiers_index) {
				auto decl_idx = feuilles->expressions[feuilles->expressions.taille - 1]->comme_decl_var();

				if (noeud->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
					decl_idx->type = espace->typeuse[TypeBase::Z32];
				}
				else {
					decl_idx->type = espace->typeuse[TypeBase::Z64];
				}

				decl_idx->drapeaux |= DECLARATION_FUT_VALIDEE;
				enfant3->membres->pousse(decl_idx);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = noeud->comme_comme();
			expr->genre_valeur = GenreValeur::DROITE;

			if (resoud_type_final(expr->expression_type, expr->type)) {
				return true;
			}

			if (noeud->type == nullptr) {
				rapporte_erreur("Ne peut transtyper vers un type invalide", expr, erreur::Genre::TYPE_INCONNU);
				return true;
			}

			donnees_dependance.types_utilises.insere(noeud->type);

			auto enfant = expr->expression;
			if (enfant->type == nullptr) {
				rapporte_erreur("Ne peut calculer le type d'origine", enfant, erreur::Genre::TYPE_INCONNU);
				return true;
			}

			auto transformation = TransformationType();

			if (cherche_transformation_pour_transtypage(*espace, *this, expr->expression->type, noeud->type, transformation)) {
				return true;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				rapporte_erreur_type_arguments(noeud, expr->expression);
				return true;
			}

			expr->transformation = transformation;

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = espace->typeuse[TypeBase::PTR_NUL];

			donnees_dependance.types_utilises.insere(noeud->type);
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr = noeud->comme_taille();
			expr->genre_valeur = GenreValeur::DROITE;
			expr->type = espace->typeuse[TypeBase::N32];

			auto expr_type = expr->expr;
			if (resoud_type_final(expr_type, expr_type->type)) {
				return true;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			auto inst = noeud->comme_plage();
			auto enfant1 = inst->expr1;
			auto enfant2 = inst->expr2;

			auto type_debut = enfant1->type;
			auto type_fin   = enfant2->type;

			if (type_debut == nullptr || type_fin == nullptr) {
				rapporte_erreur("Les types de l'expression sont invalides !", noeud, erreur::Genre::TYPE_INCONNU);
				return true;
			}

			if (type_debut != type_fin) {
				if (type_debut->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_fin)) {
					type_debut = type_fin;
					enfant1->type = type_debut;
					transtype_si_necessaire(inst->expr1, { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut });
				}
				else if (type_fin->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_debut)) {
					type_fin = type_debut;
					enfant2->type = type_fin;
					transtype_si_necessaire(inst->expr2, { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_fin });
				}
				else {
					rapporte_erreur_type_operation(type_debut, type_fin, noeud);
					return true;
				}
			}
			else if (type_debut->genre == GenreType::ENTIER_CONSTANT) {
				type_debut = espace->typeuse[TypeBase::Z32];
				transtype_si_necessaire(inst->expr1, { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut });
				transtype_si_necessaire(inst->expr2, { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut });
			}

			if (type_debut->genre != GenreType::ENTIER_NATUREL && type_debut->genre != GenreType::ENTIER_RELATIF && type_debut->genre != GenreType::REEL) {
				rapporte_erreur("Attendu des types réguliers dans la plage de la boucle 'pour'", noeud, erreur::Genre::TYPE_DIFFERENTS);
				return true;
			}

			noeud->type = type_debut;

			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = noeud->comme_controle_boucle();
			auto chaine_var = inst->expr == nullptr ? nullptr : inst->expr->ident;
			auto ok = bloc_est_dans_boucle(noeud->bloc_parent, chaine_var);

			if (ok == false) {
				if (chaine_var == nullptr) {
					rapporte_erreur("'continue' ou 'arrête' en dehors d'une boucle", noeud, erreur::Genre::CONTROLE_INVALIDE);
					return true;
				}

				rapporte_erreur("Variable inconnue", inst->expr, erreur::Genre::VARIABLE_INCONNUE);
				return true;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = noeud->comme_repete();
			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
				return true;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst = noeud->comme_tantque();

			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
				return true;
			}

			if (inst->condition->type->genre != GenreType::BOOL) {
				rapporte_erreur("Une expression booléenne est requise pour la boucle 'tantque'", inst->condition, erreur::Genre::TYPE_ARGUMENT);
				return true;
			}

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto expr = noeud->comme_construction_tableau();
			noeud->genre_valeur = GenreValeur::DROITE;

			auto feuilles = expr->expr->comme_virgule();

			if (feuilles->expressions.est_vide()) {
				return false;
			}

			auto premiere_feuille = feuilles->expressions[0];

			auto type_feuille = premiere_feuille->type;

			if (type_feuille->genre == GenreType::ENTIER_CONSTANT) {
				type_feuille = espace->typeuse[TypeBase::Z32];
				transtype_si_necessaire(feuilles->expressions[0], { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_feuille });
			}

			for (auto i = 1; i < feuilles->expressions.taille; ++i) {
				if (!transtype_si_necessaire(feuilles->expressions[i], type_feuille)) {
					return true;
				}
			}

			noeud->type = espace->typeuse.type_tableau_fixe(type_feuille, feuilles->expressions.taille);

			/* ajoute également le type de pointeur pour la génération de code C */
			auto type_ptr = espace->typeuse.type_pointeur_pour(type_feuille);

			donnees_dependance.types_utilises.insere(noeud->type);
			donnees_dependance.types_utilises.insere(type_ptr);
			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto noeud_expr = noeud->comme_info_de();
			auto expr = noeud_expr->expr;

			if (resoud_type_final(noeud_expr->expr, expr->type)) {
				return true;
			}

			auto type_info_type = Type::nul();

			switch (expr->type->genre) {
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
					type_info_type = espace->typeuse.type_info_type_;
					break;
				}
				case GenreType::ENTIER_CONSTANT:
				case GenreType::ENTIER_NATUREL:
				case GenreType::ENTIER_RELATIF:
				{
					type_info_type = espace->typeuse.type_info_type_entier;
					break;
				}
				case GenreType::REFERENCE:
				case GenreType::POINTEUR:
				{
					type_info_type = espace->typeuse.type_info_type_pointeur;
					break;
				}
				case GenreType::STRUCTURE:
				{
					type_info_type = espace->typeuse.type_info_type_structure;
					break;
				}
				case GenreType::UNION:
				{
					type_info_type = espace->typeuse.type_info_type_union;
					break;
				}
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				case GenreType::TABLEAU_FIXE:
				{
					type_info_type = espace->typeuse.type_info_type_tableau;
					break;
				}
				case GenreType::FONCTION:
				{
					type_info_type = espace->typeuse.type_info_type_fonction;
					break;
				}
				case GenreType::ENUM:
				case GenreType::ERREUR:
				{
					type_info_type = espace->typeuse.type_info_type_enum;
					break;
				}
			}

			noeud->genre_valeur = GenreValeur::DROITE;
			noeud->type = espace->typeuse.type_pointeur_pour(type_info_type);

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

			auto types_entrees = dls::tablet<Type *, 6>(2);
			types_entrees[0] = espace->typeuse.type_contexte;
			types_entrees[1] = espace->typeuse.type_pointeur_pour(type);

			auto types_sorties = dls::tablet<Type *, 6>(1);
			types_sorties[0] = espace->typeuse[TypeBase::RIEN];

			auto type_fonction = espace->typeuse.type_fonction(types_entrees, types_sorties);
			noeud->type = type_fonction;

			donnees_dependance.types_utilises.insere(noeud->type);

			noeud->genre_valeur = GenreValeur::DROITE;
			break;
		}
		case GenreNoeud::EXPRESSION_TYPE_DE:
		{
			auto expr = noeud->comme_type_de();
			auto expr_type = expr->expr;

			if (expr_type->type == nullptr) {
				rapporte_erreur("impossible de définir le type de l'expression de type_de", expr_type);
				return true;
			}

			if (expr_type->type->genre == GenreType::TYPE_DE_DONNEES) {
				noeud->type = expr_type->type;
			}
			else {
				noeud->type = espace->typeuse.type_type_de_donnees(expr_type->type);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto expr = noeud->comme_memoire();

			auto type = expr->expr->type;

			if (type->genre != GenreType::POINTEUR) {
				rapporte_erreur("Un pointeur est requis pour le déréférencement via 'mémoire'", expr->expr, erreur::Genre::TYPE_DIFFERENTS);
				return true;
			}

			auto type_pointeur = type->comme_pointeur();
			noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;
			noeud->type = type_pointeur->type_pointe;

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr_loge = noeud->comme_loge();
			expr_loge->genre_valeur = GenreValeur::DROITE;

			if (resoud_type_final(expr_loge->expression_type, expr_loge->type)) {
				return true;
			}

			if (!espace->typeuse.type_contexte || (espace->typeuse.type_contexte->drapeaux & TYPE_FUT_VALIDE) == 0) {
				unite->attend_sur_type(espace->typeuse.type_contexte);
				return true;
			}

			if (dls::outils::est_element(expr_loge->type->genre, GenreType::CHAINE, GenreType::TABLEAU_DYNAMIQUE)) {
				if (expr_loge->expr_taille == nullptr) {
					rapporte_erreur("Attendu une expression pour définir la taille du tableau à loger", noeud);
					return true;
				}

				auto type_cible = espace->typeuse[TypeBase::Z64];
				auto type_index = expr_loge->expr_taille->type;
				if (type_index->est_entier_naturel() || type_index->est_octet()) {
					transtype_si_necessaire(expr_loge->expr_taille, { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible });
				}
				else {
					if (!transtype_si_necessaire(expr_loge->expr_taille, type_cible)) {
						return true;
					}
				}
			}
			else {
				auto type_loge = expr_loge->type;

				/* attend sur le type car nous avons besoin de sa validation pour la génération de RI pour l'expression de logement */
				if ((type_loge->drapeaux & TYPE_FUT_VALIDE) == 0) {
					VERIFIE_UNITE_TYPAGE(type_loge)
					unite->attend_sur_type(type_loge);
					return true;
				}

				expr_loge->type = espace->typeuse.type_pointeur_pour(type_loge);
			}

			if (expr_loge->bloc != nullptr) {
				auto di = derniere_instruction(expr_loge->bloc);
				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					rapporte_erreur("Le bloc sinon d'une instruction « loge » doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter", expr_loge);
					return true;
				}
			}
			else {
				VERIFIE_INTERFACE_KURI_CHARGEE(panique_memoire);
				donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_memoire);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);
			donnees_dependance.types_utilises.insere(espace->typeuse.type_contexte);

			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr_loge = noeud->comme_reloge();

			if (resoud_type_final(expr_loge->expression_type, expr_loge->type)) {
				return true;
			}

			if (!espace->typeuse.type_contexte || (espace->typeuse.type_contexte->drapeaux & TYPE_FUT_VALIDE) == 0) {
				unite->attend_sur_type(espace->typeuse.type_contexte);
				return true;
			}

			if (dls::outils::est_element(expr_loge->type->genre, GenreType::CHAINE, GenreType::TABLEAU_DYNAMIQUE)) {
				if (expr_loge->expr_taille == nullptr) {
					rapporte_erreur("Attendu une expression pour définir la taille à reloger", noeud);
					return true;
				}

				auto type_cible = espace->typeuse[TypeBase::Z64];
				auto type_index = expr_loge->expr_taille->type;
				if (type_index->est_entier_naturel() || type_index->est_octet()) {
					transtype_si_necessaire(expr_loge->expr_taille, { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible });
				}
				else {
					if (!transtype_si_necessaire(expr_loge->expr_taille, type_cible)) {
						return true;
					}
				}
			}
			else {
				auto type_loge = expr_loge->type;

				/* attend sur le type car nous avons besoin de sa validation pour la génération de RI pour l'expression de logement */
				if ((type_loge->drapeaux & TYPE_FUT_VALIDE) == 0) {
					VERIFIE_UNITE_TYPAGE(type_loge)
					unite->attend_sur_type(type_loge);
					return true;
				}

				expr_loge->type = espace->typeuse.type_pointeur_pour(type_loge);
			}

			/* pour les références */
			if (!transtype_si_necessaire(expr_loge->expr, expr_loge->type)) {
				return true;
			}

			if (expr_loge->bloc != nullptr) {
				auto di = derniere_instruction(expr_loge->bloc);
				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					rapporte_erreur("Le bloc sinon d'une instruction « reloge » doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter", expr_loge);
					return true;
				}
			}
			else {
				VERIFIE_INTERFACE_KURI_CHARGEE(panique_memoire);
				donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_memoire);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);
			donnees_dependance.types_utilises.insere(espace->typeuse.type_contexte);

			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr_loge = noeud->comme_deloge();
			auto type = expr_loge->expr->type;

			if (!espace->typeuse.type_contexte || (espace->typeuse.type_contexte->drapeaux & TYPE_FUT_VALIDE) == 0) {
				unite->attend_sur_type(espace->typeuse.type_contexte);
				return true;
			}

			if (type->genre == GenreType::REFERENCE) {
				transtype_si_necessaire(expr_loge->expr, TypeTransformation::DEREFERENCE);
				type = type->comme_reference()->type_pointe;
			}

			if (!dls::outils::est_element(type->genre, GenreType::POINTEUR, GenreType::TABLEAU_DYNAMIQUE, GenreType::CHAINE)) {
				rapporte_erreur("Le type n'est pas délogeable", noeud);
				return true;
			}

			donnees_dependance.types_utilises.insere(espace->typeuse.type_contexte);

			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			return valide_structure(noeud->comme_structure());
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			return valide_enum(noeud->comme_enum());
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto inst = noeud->comme_discr();

			auto expression = inst->expr;

			auto type = expression->type;

			if (type->genre == GenreType::REFERENCE) {
				transtype_si_necessaire(inst->expr, TypeTransformation::DEREFERENCE);
				type = type->comme_reference()->type_pointe;
			}

			if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
				VERIFIE_UNITE_TYPAGE(type)
				unite->attend_sur_type(type);
				return true;
			}

			if (type->genre == GenreType::UNION) {
				auto type_union = type->comme_union();
				auto decl = type_union->decl;

				if (decl->est_nonsure) {
					rapporte_erreur("« discr » ne peut prendre une union nonsûre", expression);
					return true;
				}

				auto membres_rencontres = dls::ensemblon<IdentifiantCode *, 16>();

				auto valide_presence_membres = [&membres_rencontres, &decl, this, &expression]() {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					POUR (*decl->bloc->membres.verrou_lecture()) {
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

					auto decl_prec = trouve_dans_bloc(inst->bloc_parent, expr_paire->ident);

					/* Pousse la variable comme étant employée, puisque nous savons ce qu'elle est */
					if (decl_prec != nullptr) {
						rapporte_erreur("Ne peut pas utiliser implicitement le membre car une variable de ce nom existe déjà", expr_paire);
						return true;
					}

					/* pousse la variable dans le bloc suivant */
					auto decl_expr = static_cast<NoeudDeclaration *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, expr_paire->lexeme));
					decl_expr->ident = expr_paire->ident;
					decl_expr->lexeme = expr_paire->lexeme;
					decl_expr->bloc_parent = bloc_paire;
					decl_expr->drapeaux |= EMPLOYE;
					decl_expr->type = expr_paire->type;
					// À FAIRE: mise en place des informations d'emploi

					bloc_paire->membres->pousse(decl_expr);
				}

				if (inst->bloc_sinon == nullptr) {
					return valide_presence_membres();
				}
			}
			else if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
				auto type_enum = type->comme_enum();

				auto membres_rencontres = dls::ensemblon<dls::vue_chaine_compacte, 16>();
				noeud->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;

					auto feuilles = expr_paire->comme_virgule();

					for (auto f : feuilles->expressions) {
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
							rapporte_erreur_membre_inconnu(noeud, expression, f, type_enum);
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
						if (!membres_rencontres.possede(it.nom) && (it.drapeaux & TypeCompose::Membre::EST_IMPLICITE) == 0) {
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
				auto type_pour_la_recherche = type;
				if (type->genre == GenreType::TYPE_DE_DONNEES) {
					type_pour_la_recherche = espace->typeuse.type_type_de_donnees_;
				}

				auto candidats = dls::tablet<OperateurCandidat, 10>();
				if (cherche_candidats_operateurs(*espace, *this, type_pour_la_recherche, type_pour_la_recherche, GenreLexeme::EGALITE, candidats)) {
					return true;
				}

				auto meilleur_candidat = OperateurCandidat::nul_const();
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					::rapporte_erreur(unite->espace, noeud,
									  "Je ne peux pas valider l'expression de discrimination car je n'arrive à trouver un opérateur de comparaison pour le "
									  "type de l'expression. ")
							.ajoute_message("Le type de l'expression est : ")
							.ajoute_message(chaine_type(type))
							.ajoute_message(".\n\n")
							.ajoute_conseil("Les discriminations ont besoin d'un opérateur « == » défini pour le type afin de pouvoir comparer les valeurs,"
											" donc si vous voulez utiliser une discrimination sur un type personnalisé, vous pouvez définir l'opérateur comme ceci :\n\n"
											"\topérateur == :: fonc (a: MonType, b: MonType) -> bool\n\t{\n\t\t /* logique de comparaison */\n\t}\n");
					return true;
				}

				inst->op = meilleur_candidat->op;

				if (!inst->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(inst->op->decl);
				}

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;

					auto feuilles = expr_paire->comme_virgule();

					for (auto j = 0; j < feuilles->expressions.taille; ++j) {
						if (valide_semantique_noeud(feuilles->expressions[j])) {
							return true;
						}

						if (!transtype_si_necessaire(feuilles->expressions[j], expression->type)) {
							return true;
						}
					}
				}

				if (inst->bloc_sinon == nullptr) {
					::rapporte_erreur(espace, noeud, "Les discriminations de valeurs scalaires doivent avoir un bloc « sinon »");
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

			return valide_expression_retour(noeud->comme_retiens());
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = noeud->comme_parenthese();
			noeud->type = expr->expr->type;
			noeud->genre_valeur = expr->expr->genre_valeur;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto inst = noeud->comme_pousse_contexte();
			auto variable = inst->expr;

			if (variable->type != espace->typeuse.type_contexte) {
				::rapporte_erreur(espace, variable, "La variable doit être de type ContexteProgramme")
						.ajoute_message("Note : la variable est de type ")
						.ajoute_message(chaine_type(variable->type))
						.ajoute_message("\n");
				return true;
			}

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = noeud->comme_expansion_variadique();

			if (expr->expr == nullptr) {
				// nous avons un type variadique
				auto type_var = espace->typeuse.type_variadique(nullptr);
				expr->type = espace->typeuse.type_type_de_donnees(type_var);
				return false;
			}

			auto type_expr = expr->expr->type;

			if (type_expr->genre == GenreType::TYPE_DE_DONNEES) {
				auto type_de_donnees = type_expr->comme_type_de_donnees();
				auto type_var = espace->typeuse.type_variadique(type_de_donnees->type_connu);
				expr->type = espace->typeuse.type_type_de_donnees(type_var);
			}
			else {
				if (!dls::outils::est_element(type_expr->genre, GenreType::TABLEAU_FIXE, GenreType::TABLEAU_DYNAMIQUE, GenreType::VARIADIQUE)) {
					::rapporte_erreur(espace, expr, "Type invalide pour l'expansion variadique, je requiers un type de tableau ou un type variadique")
							.ajoute_message("Note : le type de l'expression est ")
							.ajoute_message(chaine_type(type_expr))
							.ajoute_message("\n");
				}

				if (type_expr->est_tableau_fixe()) {
					auto type_tableau_fixe = type_expr->comme_tableau_fixe();
					type_expr = espace->typeuse.type_tableau_dynamique(type_tableau_fixe->type_pointe);
					transtype_si_necessaire(expr->expr, { TypeTransformation::CONVERTI_TABLEAU, type_expr });
				}

				expr->type = type_expr;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = noeud->comme_tente();
			inst->type = inst->expr_appel->type;
			inst->genre_valeur = GenreValeur::DROITE;

			auto type_de_l_erreur = Type::nul();

			// voir ce que l'on retourne
			// - si aucun type erreur -> erreur ?
			// - si erreur seule -> il faudra vérifier l'erreur
			// - si union -> voir si l'union est sûre et contient une erreur, dépaquete celle-ci dans le génération de code

			if ((inst->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
				VERIFIE_UNITE_TYPAGE(inst->type)
				unite->attend_sur_type(inst->type);
				return true;
			}

			if (inst->type->genre == GenreType::ERREUR) {
				type_de_l_erreur = inst->type;
			}
			else if (inst->type->genre == GenreType::UNION) {
				auto type_union = inst->type->comme_union();
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
				else {
					::rapporte_erreur(espace, inst, "Les instructions tentes ne sont pas encore définies pour les unions n'ayant pas 2 membres uniquement.")
							.ajoute_message("Le type du l'union est ")
							.ajoute_message(chaine_type(type_union))
							.ajoute_message("\n");
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

				auto var_piege = inst->expr_piege->comme_ref_decl();

				auto decl = trouve_dans_bloc(var_piege->bloc_parent, var_piege->ident);

				if (decl != nullptr) {
					rapporte_erreur_redefinition_symbole(var_piege, decl);
				}

				var_piege->type = type_de_l_erreur;

				auto decl_var_piege = static_cast<NoeudDeclarationVariable *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, var_piege->lexeme));
				decl_var_piege->bloc_parent = inst->bloc;
				decl_var_piege->valeur = var_piege;
				decl_var_piege->type = var_piege->type;
				decl_var_piege->ident = var_piege->ident;
				decl_var_piege->drapeaux |= DECLARATION_FUT_VALIDEE;

				// ne l'ajoute pas aux expressions, car nous devons l'initialiser manuellement
				inst->bloc->membres->pousse_front(decl_var_piege);

				auto di = derniere_instruction(inst->bloc);

				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					rapporte_erreur("Un bloc de piège doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter", inst);
					return true;
				}
			}
			else {
				VERIFIE_INTERFACE_KURI_CHARGEE(panique_erreur);
				donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_erreur);
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_EMPL:
		{
			auto empl = noeud->comme_empl();
			auto decl = empl->expr->comme_decl_var();

			empl->type = decl->type;
			decl->drapeaux |= EMPLOYE;
			auto type_employe = decl->type;

			// permet le déréférencement de pointeur, mais uniquement sur un niveau
			if (type_employe->genre == GenreType::POINTEUR) {
				type_employe = type_employe->comme_pointeur()->type_pointe;
			}

			if (type_employe->genre != GenreType::STRUCTURE) {
				::rapporte_erreur(unite->espace, decl, "Impossible d'employer une variable n'étant pas une structure.")
						.ajoute_message("Le type de la variable est : ")
						.ajoute_message(chaine_type(type_employe))
						.ajoute_message(".\n\n");
				return true;
			}

			if ((type_employe->drapeaux & TYPE_FUT_VALIDE) == 0) {
				VERIFIE_UNITE_TYPAGE(type_employe)
				unite->attend_sur_type(type_employe);
				return true;
			}

			auto type_structure = type_employe->comme_structure();

			auto index_membre = 0;

			// pour les structures, prend le bloc_parent qui sera celui de la structure
			auto bloc_parent = decl->bloc_parent;

			// pour les fonctions, utilisent leurs blocs si le bloc_parent est le bloc_parent de la fonction (ce qui est le cas pour les paramètres...)
			if (fonction_courante && bloc_parent == fonction_courante->corps->bloc->bloc_parent) {
				bloc_parent = fonction_courante->corps->bloc;
			}

			POUR (type_structure->membres) {
				auto decl_membre = static_cast<NoeudDeclarationVariable *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme));
				decl_membre->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine(it.nom);
				decl_membre->type = it.type;
				decl_membre->bloc_parent = bloc_parent;
				decl_membre->drapeaux |= DECLARATION_FUT_VALIDEE;
				decl_membre->declaration_vient_d_un_emploi = decl;
				decl_membre->index_membre_employe = index_membre++;
				decl_membre->expression = it.expression_valeur_defaut;

				bloc_parent->membres->pousse(decl_membre);
			}
			break;
		}
	}

	return false;
}

bool ContexteValidationCode::valide_acces_membre(NoeudExpressionMembre *expression_membre)
{
	Prof(valide_acces_membre);

	auto structure = expression_membre->accede;
	auto membre = expression_membre->membre;
	auto type = structure->type;

	/* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
	while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
		type = type_dereference_pour(type);
	}

	// Il est possible d'avoir une chaine de type : Struct1.Struct2.Struct3...
	if (type->genre == GenreType::TYPE_DE_DONNEES) {
		auto type_de_donnees = type->comme_type_de_donnees();

		if (type_de_donnees->type_connu != nullptr) {
			type = type_de_donnees->type_connu;
			// change le type de la structure également pour simplifier la génération
			// de la RI (nous nous basons sur le type pour ça)
			structure->type = type;
		}
	}

	if (est_type_compose(type)) {
		if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
			VERIFIE_UNITE_TYPAGE(type)
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
			auto noeud_struct = type->comme_union()->decl;
			expression_membre->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;

			if (!noeud_struct->est_nonsure) {
				if ((expression_membre->drapeaux & DROITE_ASSIGNATION) == 0) {
					renseigne_membre_actif(structure->ident->nom, membre->ident->nom);
				}
				else {
					auto membre_actif = trouve_membre_actif(structure->ident->nom);

					VERIFIE_INTERFACE_KURI_CHARGEE(panique_membre_union);
					donnees_dependance.fonctions_utilisees.insere(espace->interface_kuri->decl_panique_membre_union);

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

	rapporte_erreur(flux.chn().c_str(), structure, erreur::Genre::TYPE_DIFFERENTS);
	return true;
}

bool ContexteValidationCode::valide_type_fonction(NoeudDeclarationEnteteFonction *decl)
{
	Prof(valide_type_fonction);

#ifdef CHRONOMETRE_TYPAGE
	auto possede_erreur = true;
	dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
		if (possede_erreur) {
			m_tacheronne.stats_typage.fonctions.fusionne_entree({ "tentatives râtées", temps });
		}
	});
#endif

	CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction");
	commence_fonction(decl);

	auto &graphe = espace->graphe_dependance;
	auto noeud_dep = graphe->cree_noeud_fonction(decl);

	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (arbre aplatis)");
		if (valide_arbre_aplatis(decl->arbre_aplatis)) {
			graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
			return true;
		}
	}

	// -----------------------------------
	if (!decl->est_monomorphisation) {
		CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (validation paramètres)");
		auto noms = dls::ensemblon<IdentifiantCode *, 16>();
		auto dernier_est_variadic = false;

		for (auto i = 0; i < decl->params.taille; ++i) {
			auto param = decl->parametre_entree(i);
			auto variable = param->valeur;
			auto expression = param->expression;

			if (noms.possede(variable->ident)) {
				rapporte_erreur("Redéfinition de l'argument", variable, erreur::Genre::ARGUMENT_REDEFINI);
				return true;
			}

			if (dernier_est_variadic) {
				rapporte_erreur("Argument déclaré après un argument variadic", variable);
				return true;
			}

			if (variable->type && variable->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				decl->est_polymorphe = true;
			}
			else {
				if (expression != nullptr) {
					if (decl->est_operateur) {
						rapporte_erreur("Un paramètre d'une surcharge d'opérateur ne peut avoir de valeur par défaut", param);
						return true;
					}
				}
			}

			noms.insere(variable->ident);

			if (param->type->genre == GenreType::VARIADIQUE) {
				param->drapeaux |= EST_VARIADIQUE;
				decl->est_variadique = true;
				dernier_est_variadic = true;

				auto type_var = param->type->comme_variadique();

				if (!decl->est_externe && type_var->type_pointe == nullptr) {
					rapporte_erreur(
								"La déclaration de fonction variadique sans type n'est"
								" implémentée que pour les fonctions externes",
								param);
					return true;
				}
			}
		}

		if (decl->est_polymorphe) {
			decl->drapeaux |= DECLARATION_FUT_VALIDEE;
			return false;
		}
	}
	else {
		for (auto i = 0; i < decl->params.taille; ++i) {
			auto param = decl->parametre_entree(i);
			auto variable = param->valeur;

			/* ne résoud que les types polymorphiques, les autres doivent l'avoir été durant la première passe
			 * À FAIRE : manière plus robuste de faire ceci (par exemple en mettant en cache les types et leurs résolutions) */
			if (param->type->est_polymorphe()) {
				if (resoud_type_final(param->expression_type, variable->type)) {
					return true;
				}
			}

			param->type = variable->type;
		}

		// À FAIRE : crash dans la validation des expressions d'appels lors de la copie des tablets
		//           de toute manière ceci n'est pas nécessaire
		//decl->bloc_parent->membres->pousse(decl);
	}

	// -----------------------------------

	TypeFonction *type_fonc = nullptr;
	auto possede_contexte = !decl->est_externe && !decl->possede_drapeau(FORCE_NULCTX);
	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (typage)");

		dls::tablet<Type *, 6> types_entrees;
		types_entrees.reserve(decl->params.taille + possede_contexte);

		if (possede_contexte) {
			types_entrees.pousse(espace->typeuse.type_contexte);
		}

		POUR (decl->params) {
			types_entrees.pousse(it->type);
		}

		dls::tablet<Type *, 6> types_sorties;
		types_sorties.reserve(decl->params_sorties.taille);

		for (auto &type_declare : decl->params_sorties) {
			Type *type_sortie = nullptr;
			if (resoud_type_final(type_declare->expression_type, type_sortie)) {
				return true;
			}
			types_sorties.pousse(type_sortie);
		}

		CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (type_fonction)");
		type_fonc = espace->typeuse.type_fonction(types_entrees, types_sorties);
		decl->type = type_fonc;
		donnees_dependance.types_utilises.insere(decl->type);
	}

	if (decl->est_operateur) {
		CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (opérateurs)");
		auto type_resultat = type_fonc->types_sorties[0];

		if (type_resultat == espace->typeuse[TypeBase::RIEN]) {
			rapporte_erreur("Un opérateur ne peut retourner 'rien'", decl);
			return true;
		}

		if (est_operateur_bool(decl->lexeme->genre) && type_resultat != espace->typeuse[TypeBase::BOOL]) {
			rapporte_erreur("Un opérateur de comparaison doit retourner 'bool'", decl);
			return true;
		}

		auto operateurs = espace->operateurs.verrou_ecriture();

		if (decl->params.taille == 1) {			
			auto &iter_op = operateurs->trouve_unaire(decl->lexeme->genre);
			auto type1 = type_fonc->types_entrees[0 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type_operande == type1) {
					if (op->est_basique) {
						rapporte_erreur("redéfinition de l'opérateur basique", decl);
						return true;
					}

					::rapporte_erreur(espace, decl, "Redéfinition de l'opérateur")
							.ajoute_message("L'opérateur fut déjà défini ici :\n")
							.ajoute_site(op->decl);
					return true;
				}
			}

			operateurs->ajoute_perso_unaire(
						decl->lexeme->genre,
						type1,
						type_resultat,
						decl);
		}
		else if (decl->params.taille == 2) {
			auto &iter_op = operateurs->trouve_binaire(decl->lexeme->genre);
			auto type1 = type_fonc->types_entrees[0 + possede_contexte];
			auto type2 = type_fonc->types_entrees[1 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type1 == type1 && op->type2 == type2) {
					if (op->est_basique) {
						rapporte_erreur("redéfinition de l'opérateur basique", decl);
						return true;
					}

					::rapporte_erreur(espace, decl, "Redéfinition de l'opérateur")
							.ajoute_message("L'opérateur fut déjà défini ici :\n")
							.ajoute_site(op->decl);
					return true;
				}
			}

			operateurs->ajoute_perso(
						decl->lexeme->genre,
						type1,
						type2,
						type_resultat,
						decl);
		}
	}
	else {
		// À FAIRE(moultfilage) : vérifie l'utilisation des synchrones pour les tableaux
		{
			CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (redéfinition)");
			POUR (*decl->bloc_parent->membres.verrou_lecture()) {
				if (it == decl) {
					continue;
				}

				if (it->genre != GenreNoeud::DECLARATION_ENTETE_FONCTION) {
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
		}
	}

	graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
	decl->drapeaux |= DECLARATION_FUT_VALIDEE;

#ifdef CHRONOMETRE_TYPAGE
	possede_erreur = false;
#endif

	return false;
}

bool ContexteValidationCode::valide_arbre_aplatis(kuri::tableau<NoeudExpression *> &arbre_aplatis)
{
	for (; unite->index_courant < arbre_aplatis.taille; ++unite->index_courant) {
		auto noeud_enfant = arbre_aplatis[unite->index_courant];

		if (noeud_enfant->est_structure()) {
			// les structures ont leurs propres unités de compilation
			if (!noeud_enfant->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
				if (noeud_enfant->comme_structure()->unite == nullptr) {
					m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, noeud_enfant->comme_structure());
				}
				unite->attend_sur_declaration(noeud_enfant->comme_structure());
				return true;
			}

			continue;
		}

		if (valide_semantique_noeud(noeud_enfant)) {
			return true;
		}
	}

	return false;
}

static void rassemble_expressions(NoeudExpression *expr, dls::tablet<NoeudExpression *, 6> &expressions)
{
	if (expr == nullptr) {
		return;
	}

	if (expr->est_virgule()) {
		auto virgule = expr->comme_virgule();

		POUR (virgule->expressions) {
			expressions.pousse(it);
		}
	}
	else {
		expressions.pousse(expr);
	}
}

struct VariableEtExpression {
	IdentifiantCode *ident = nullptr;
	NoeudExpression *expression = nullptr;
};

static void rassemble_expressions(NoeudExpression *expr, dls::tablet<VariableEtExpression, 6> &expressions)
{
	if (expr == nullptr) {
		return;
	}

	if (expr->est_virgule()) {
		auto virgule = expr->comme_virgule();

		POUR (virgule->expressions) {
			if (it->est_assignation()) {
				auto assignation = it->comme_assignation();
				expressions.pousse({ assignation->variable->ident, assignation->expression });
			}
			else {
				expressions.pousse({ nullptr, it });
			}
		}
	}
	else {
		if (expr->est_assignation()) {
			auto assignation = expr->comme_assignation();
			expressions.pousse({ assignation->variable->ident, assignation->expression });
		}
		else {
			expressions.pousse({ nullptr, expr });
		}
	}
}

bool ContexteValidationCode::valide_expression_retour(NoeudRetour *inst)
{
	auto type_fonc = fonction_courante->type->comme_fonction();
	auto est_corps_texte = fonction_courante->corps->est_corps_texte;

	if (inst->expr == nullptr) {
		inst->type = espace->typeuse[TypeBase::RIEN];

		if ((!fonction_courante->est_coroutine && type_fonc->types_sorties[0] != inst->type) || est_corps_texte) {
			rapporte_erreur("Expression de retour manquante", inst);
			return true;
		}

		donnees_dependance.types_utilises.insere(inst->type);
		return false;
	}

	if (est_corps_texte) {
		if (inst->expr->est_virgule()) {
			rapporte_erreur("Trop d'expression de retour pour le corps texte", inst->expr);
			return true;
		}

		auto expr = inst->expr;

		if (expr->est_assignation()) {
			rapporte_erreur("Impossible d'assigner la valeur de retour pour un #corps_texte", inst->expr);
			return true;
		}

		if (!expr->type->est_chaine()) {
			rapporte_erreur("Attendu un type chaine pour le retour de #corps_texte", inst->expr);
			return true;
		}

		inst->type = espace->typeuse[TypeBase::CHAINE];

		DonneesAssignations donnees;
		donnees.expression = inst->expr;
		donnees.variables.pousse(fonction_courante->params_sorties[0]);
		donnees.transformations.pousse({});

		inst->donnees_exprs.pousse(donnees);

		donnees_dependance.types_utilises.insere(inst->type);

		return false;
	}

	auto nombre_retour = type_fonc->types_sorties.taille;

	dls::file<NoeudExpression *> variables;

	POUR (fonction_courante->params_sorties) {
		variables.enfile(it);
	}

	/* tri les expressions selon les noms */
	dls::tablet<VariableEtExpression, 6> vars_et_exprs;
	rassemble_expressions(inst->expr, vars_et_exprs);

	dls::tablet<NoeudExpression *, 6> expressions;
	expressions.redimensionne(vars_et_exprs.taille());

	POUR (expressions) {
		it = nullptr;
	}

	auto index_courant = 0;
	auto eu_nom = false;
	POUR (vars_et_exprs) {
		auto expr = it.expression;

		if (it.ident) {
			eu_nom = true;

			if (expr->est_appel() && expr->comme_appel()->noeud_fonction_appelee->type->est_fonction()) {
				auto type_fonction = expr->comme_appel()->noeud_fonction_appelee->type->comme_fonction();

				if (type_fonction->types_sorties.taille > 1) {
					::rapporte_erreur(espace, it.expression, "Impossible de nommer les variables de retours si l'expression est une fonction retrounant plusieurs valeurs");
					return true;
				}
			}

			for (auto i = 0; i < fonction_courante->params_sorties.taille; ++i) {
				if (it.ident == fonction_courante->params_sorties[i]->ident) {
					if (expressions[i] != nullptr) {
						::rapporte_erreur(espace, it.expression, "Redéfinition d'une expression pour un paramètre de retour");
						return true;
					}

					expressions[i] = it.expression;
					break;
				}
			}
		}
		else {
			if (eu_nom) {
				::rapporte_erreur(espace, it.expression, "L'expressoin doit avoir un nom si elle suit une autre ayant déjà un nom");
			}

			if (expressions[index_courant] != nullptr) {
				::rapporte_erreur(espace, it.expression, "Redéfinition d'une expression pour un paramètre de retour");
				return true;
			}

			expressions[index_courant] = it.expression;
		}

		index_courant += 1;
	}

	auto valide_typage_et_ajoute = [this](DonneesAssignations &donnees, NoeudExpression *variable, NoeudExpression *expression, Type *type_de_l_expression)
	{
		auto transformation = TransformationType();
		if (cherche_transformation(*espace, *this, type_de_l_expression, variable->type, transformation)) {
			return false;
		}

		if (transformation.type == TypeTransformation::IMPOSSIBLE) {
			rapporte_erreur_assignation_type_differents(variable->type, type_de_l_expression, expression);
			return false;
		}

		donnees.variables.pousse(variable);
		donnees.transformations.pousse(transformation);
		return true;
	};

	dls::tablet<DonneesAssignations, 6> donnees_retour;

	POUR (expressions) {
		DonneesAssignations donnees;
		donnees.expression = it;

		if (it->est_appel() && it->comme_appel()->noeud_fonction_appelee && it->comme_appel()->noeud_fonction_appelee->type->est_fonction()) {
			auto type_fonction = it->comme_appel()->noeud_fonction_appelee->type->comme_fonction();

			donnees.multiple_retour = type_fonction->types_sorties.taille > 1;

			for (auto type : type_fonction->types_sorties) {
				if (variables.est_vide()) {
					::rapporte_erreur(espace, it, "Trop d'expressions de retour");
					break;
				}

				if (!valide_typage_et_ajoute(donnees, variables.defile(), it, type)) {
					return true;
				}
			}
		}
		else if (it->type->est_rien()) {
			rapporte_erreur("impossible de retourner une expression de type « rien » à une variable", it, erreur::Genre::ASSIGNATION_RIEN);
			return true;
		}
		else {
			if (variables.est_vide()) {
				::rapporte_erreur(espace, it, "Trop d'expressions de retour");
				return true;
			}

			if (!valide_typage_et_ajoute(donnees, variables.defile(), it, it->type)) {
				return true;
			}
		}

		donnees_retour.pousse(donnees);
	}

	// À FAIRE : valeur par défaut des expressions
	if (!variables.est_vide()) {
		::rapporte_erreur(espace, inst, "Expressions de retour manquante");
		return true;
	}

	if (nombre_retour > 1) {
		inst->type = espace->typeuse[TypeBase::RIEN];
	}
	else {
		inst->type = type_fonc->types_sorties[0];
	}

	inst->donnees_exprs.reserve(donnees_retour.taille());
	POUR (donnees_retour) {
		inst->donnees_exprs.pousse(it);
	}

	donnees_dependance.types_utilises.insere(inst->type);
	return false;
}

bool ContexteValidationCode::valide_fonction(NoeudDeclarationCorpsFonction *decl)
{
	auto entete = decl->entete;

	if (entete->est_polymorphe && !entete->est_monomorphisation) {
		// nous ferons l'analyse sémantique plus tard
		return false;
	}

	decl->type = entete->type;

	auto est_corps_texte = decl->est_corps_texte;
	MetaProgramme *metaprogramme = nullptr;

	if (est_corps_texte) {
		auto fonction = m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_ENTETE_FONCTION, decl->lexeme)->comme_entete_fonction();
		auto nouveau_corps = fonction->corps;

		fonction->bloc_parent = entete->bloc_parent;
		nouveau_corps->bloc_parent = decl->bloc_parent;

		/* échange les corps */
		entete->corps = nouveau_corps;
		nouveau_corps->entete = entete;

		fonction->corps = decl;
		decl->entete = fonction;

		/* mise en place du type de la fonction : () -> chaine */
		fonction->est_metaprogramme = true;

		auto decl_sortie = m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme)->comme_decl_var();
		decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
		decl_sortie->type = espace->typeuse[TypeBase::CHAINE];
		decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;

		fonction->params_sorties.pousse(decl_sortie);

		auto types_entrees = dls::tablet<Type *, 6>(0);

		auto types_sorties = dls::tablet<Type *, 6>(1);
		types_sorties[0] = espace->typeuse[TypeBase::CHAINE];

		fonction->type = espace->typeuse.type_fonction(types_entrees, types_sorties);
		fonction->drapeaux |= DECLARATION_FUT_VALIDEE;

		metaprogramme = espace->cree_metaprogramme();
		metaprogramme->corps_texte = decl;
		metaprogramme->recipiente_corps_texte = entete;
		metaprogramme->fonction = fonction;

		fonction->est_monomorphisation = entete->est_monomorphisation;
		fonction->paires_expansion_gabarit  = entete->paires_expansion_gabarit;

		entete = fonction;
	}

	auto &graphe = espace->graphe_dependance;
	auto noeud_dep = graphe->cree_noeud_fonction(entete);

	commence_fonction(entete);

	if (unite->index_courant == 0) {
		auto requiers_contexte = !decl->entete->possede_drapeau(FORCE_NULCTX);

		if (requiers_contexte) {
			auto val_ctx = static_cast<NoeudExpressionReference *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, decl->lexeme));
			val_ctx->type = espace->typeuse.type_contexte;
			val_ctx->bloc_parent = decl->bloc_parent;
			val_ctx->ident = ID::contexte;

			auto decl_ctx = static_cast<NoeudDeclarationVariable *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme));
			decl_ctx->bloc_parent = decl->bloc_parent;
			decl_ctx->valeur = val_ctx;
			decl_ctx->type = val_ctx->type;
			decl_ctx->ident = val_ctx->ident;
			decl_ctx->drapeaux |= DECLARATION_FUT_VALIDEE;

			decl->bloc->membres->pousse(decl_ctx);
		}
	}

	CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide fonction");

	if (valide_arbre_aplatis(decl->arbre_aplatis)) {
		graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
		return true;
	}

	auto bloc = decl->bloc;
	auto inst_ret = derniere_instruction(bloc);

	/* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
	if (inst_ret == nullptr) {
		auto type_fonc = entete->type->comme_fonction();

		if ((type_fonc->types_sorties[0]->genre != GenreType::RIEN && !entete->est_coroutine) || est_corps_texte) {
			rapporte_erreur("Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
			return true;
		}

		if (entete != espace->interface_kuri->decl_creation_contexte) {
			decl->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;
		}
	}

	graphe->ajoute_dependances(*noeud_dep, donnees_dependance);

	if (est_corps_texte) {
		/* Le dreapeaux nulctx est pour la génération de RI de l'entête, donc il
		 * faut le mettre après avoir validé le corps, la création d'un contexte
		 * au début de la fonction sera ajouté avant l'exécution du code donc il
		 * est possible d'utiliser le contexte dans le métaprogramme. */
		entete->drapeaux |= FORCE_NULCTX;
		m_compilatrice.ordonnanceuse->cree_tache_pour_execution(espace, metaprogramme);
	}

	termine_fonction();
	return false;
}

bool ContexteValidationCode::valide_operateur(NoeudDeclarationCorpsFonction *decl)
{
	auto entete = decl->entete;
	commence_fonction(entete);

	decl->type = entete->type;

	auto &graphe = espace->graphe_dependance;
	auto noeud_dep = graphe->cree_noeud_fonction(entete);

	if (unite->index_courant == 0) {
		auto requiers_contexte = !decl->possede_drapeau(FORCE_NULCTX);

		decl->bloc->membres->reserve(entete->params.taille + requiers_contexte);

		if (requiers_contexte) {
			auto val_ctx = static_cast<NoeudExpressionReference *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, decl->lexeme));
			val_ctx->type = espace->typeuse.type_contexte;
			val_ctx->bloc_parent = decl->bloc_parent;
			val_ctx->ident = ID::contexte;

			auto decl_ctx = static_cast<NoeudDeclarationVariable *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl->lexeme));
			decl_ctx->bloc_parent = decl->bloc_parent;
			decl_ctx->valeur = val_ctx;
			decl_ctx->type = val_ctx->type;
			decl_ctx->ident = val_ctx->ident;
			decl_ctx->drapeaux |= DECLARATION_FUT_VALIDEE;

			decl->bloc->membres->pousse(decl_ctx);
		}
	}

	if (valide_arbre_aplatis(decl->arbre_aplatis)) {
		graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
		return true;
	}

	auto inst_ret = derniere_instruction(decl->bloc);

	if (inst_ret == nullptr) {
		rapporte_erreur("Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
		return true;
	}

	graphe->ajoute_dependances(*noeud_dep, donnees_dependance);

	termine_fonction();
	return false;
}

bool ContexteValidationCode::valide_enum(NoeudEnum *decl)
{
	CHRONO_TYPAGE(m_tacheronne.stats_typage.enumerations, "valide énum");
	auto type_enum = decl->type->comme_enum();
	auto &membres = type_enum->membres;

	// nous avons besoin du symbole le plus rapidement possible pour déterminer les types l'utilisant
	decl->bloc_parent->membres->pousse(decl);

	if (type_enum->est_erreur) {
		type_enum->type_donnees = espace->typeuse[TypeBase::Z32];
	}
	else if (decl->expression_type != nullptr) {
		if (valide_semantique_noeud(decl->expression_type)) {
			return true;
		}

		if (resoud_type_final(decl->expression_type, type_enum->type_donnees)) {
			return true;
		}

		/* les énum_drapeaux doivent être des types naturels pour éviter les problèmes d'arithmétiques binaire */
		if (type_enum->est_drapeau && !type_enum->type_donnees->est_entier_naturel()) {
			::rapporte_erreur(espace, decl->expression_type, "Les énum_drapeaux doivent être de type entier naturel (n8, n16, n32, ou n64).\n", erreur::Genre::TYPE_DIFFERENTS)
					.ajoute_message("Note : un entier naturel est requis car certaines manipulations de bits en complément à deux, par exemple les décalages à droite avec l'opérateur >>, préserve le signe de la valeur. "
									"Un décalage à droite sur l'octet de type relatif 10101010 produirait 10010101 et non 01010101 comme attendu. Ainsi, pour que je puisse garantir un programme bienformé, un type naturel doit être utilisé.\n");
		}
	}
	else if (type_enum->est_drapeau) {
		type_enum->type_donnees = espace->typeuse[TypeBase::N32];
	}
	else {
		type_enum->type_donnees = espace->typeuse[TypeBase::Z32];
	}

	auto &graphe = espace->graphe_dependance;
	graphe->connecte_type_type(type_enum, type_enum->type_donnees);

	type_enum->taille_octet = type_enum->type_donnees->taille_octet;
	type_enum->alignement = type_enum->type_donnees->alignement;

	espace->operateurs->ajoute_operateur_basique_enum(decl->type);

	auto noms_rencontres = dls::ensemblon<IdentifiantCode *, 32>();

	auto dernier_res = ResultatExpression();

	membres.reserve(decl->bloc->expressions->taille);
	decl->bloc->membres->reserve(decl->bloc->expressions->taille);

	POUR (*decl->bloc->expressions.verrou_ecriture()) {
		if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
			rapporte_erreur("Type d'expression inattendu dans l'énum", it);
			return true;
		}

		auto decl_expr = it->comme_decl_var();
		decl_expr->type = type_enum;

		decl->bloc->membres->pousse(decl_expr);

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
		/* est_errone est utilisé pour indiquer que dernier_resultat est la
		 * première valeur, si res.est_errone est également vrai, la valeur
		 * n'est jamais incrémentée (res étant assigné à dernier_resultat) */
		res.est_errone = false;

		// À FAIRE(erreur) : vérifie qu'aucune expression s'évalue à zéro
		if (expr != nullptr) {
			res = evalue_expression(espace, decl->bloc, expr);

			if (res.est_errone) {
				rapporte_erreur(res.message_erreur, res.noeud_erreur, erreur::Genre::VARIABLE_REDEFINIE);
				return true;
			}
		}
		else {
			if (dernier_res.est_errone) {
				/* première valeur, laisse à zéro si énum normal */
				dernier_res.est_errone = false;

				if (type_enum->est_drapeau || type_enum->est_erreur) {
					res.type = TypeExpression::ENTIER;
					res.entier = 1;
				}
			}
			else {
				if (dernier_res.type == TypeExpression::ENTIER) {
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

	membres.pousse({ espace->typeuse[TypeBase::Z32], "nombre_éléments", 0, static_cast<int>(membres.taille), nullptr, TypeCompose::Membre::EST_IMPLICITE });

	decl->drapeaux |= DECLARATION_FUT_VALIDEE;
	decl->type->drapeaux |= TYPE_FUT_VALIDE;
	return false;
}

bool ContexteValidationCode::valide_structure(NoeudStruct *decl)
{
	auto &graphe = espace->graphe_dependance;

	auto noeud_dependance = graphe->cree_noeud_type(decl->type);
	decl->noeud_dependance = noeud_dependance;

	if (decl->est_externe && decl->bloc == nullptr) {
		return false;
	}

	if (decl->est_polymorphe) {
		if (valide_arbre_aplatis(decl->arbre_aplatis_params)) {
			graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
			return true;
		}

		// nous validerons les membres lors de la monomorphisation
		decl->drapeaux |= DECLARATION_FUT_VALIDEE;
		decl->type->drapeaux |= TYPE_FUT_VALIDE;
		return false;
	}

	if (valide_arbre_aplatis(decl->arbre_aplatis)) {
		graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
		return true;
	}

	CHRONO_TYPAGE(m_tacheronne.stats_typage.structures, "valide structure");

	if (decl->bloc->membres->est_vide()) {
		rapporte_erreur("Bloc vide pour la déclaration de structure", decl);
		return true;
	}

	if (!decl->est_monomorphisation) {
		auto decl_precedente = trouve_dans_bloc(decl->bloc_parent, decl);

		// la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les structres (p.e. stat)
		// @vérifie si utile
		if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
			rapporte_erreur_redefinition_symbole(decl, decl_precedente);
			return true;
		}
	}

	auto type_compose = decl->type->comme_compose();
	// @réinitialise en cas d'erreurs passées
	type_compose->membres = kuri::tableau<TypeCompose::Membre>();
	type_compose->membres.reserve(decl->bloc->membres->taille);

	auto verifie_inclusion_valeur = [&decl, this](NoeudExpression *enf)
	{
		if (enf->type == decl->type) {
			rapporte_erreur("Ne peut inclure la structure dans elle-même par valeur", enf, erreur::Genre::TYPE_ARGUMENT);
			return true;
		}

		auto type_base = enf->type;

		if (type_base->genre == GenreType::TABLEAU_FIXE) {
			auto type_deref = type_dereference_pour(type_base);

			if (type_deref == decl->type) {
				rapporte_erreur("Ne peut inclure la structure dans elle-même par valeur", enf, erreur::Genre::TYPE_ARGUMENT);
				return true;
			}
		}

		return false;
	};

	auto ajoute_donnees_membre = [&, this](NoeudExpression *enfant, NoeudExpression *expr_valeur)
	{
		auto type_membre = enfant->type;
		auto align_type = type_membre->alignement;

		// À FAIRE: ceci devrait plutôt être déplacé dans la validation des déclarations, mais nous finissons sur une erreur de compilation à cause d'une attente
		if ((type_membre->drapeaux & TYPE_FUT_VALIDE) == 0) {
			VERIFIE_UNITE_TYPAGE(type_membre)
			unite->attend_sur_type(type_membre);
			return true;
		}

		if (align_type == 0) {
			rapporte_erreur("impossible de définir l'alignement du type", enfant);
			return true;
		}

		if (type_membre->taille_octet == 0) {
			rapporte_erreur("impossible de définir la taille du type", enfant);
			return true;
		}

		type_compose->membres.pousse({ enfant->type, enfant->ident->nom, 0, 0, expr_valeur });

		donnees_dependance.types_utilises.insere(type_membre);

		return false;
	};

	if (decl->est_union) {
		auto type_union = decl->type->comme_union();
		type_union->est_nonsure = decl->est_nonsure;

		POUR (*decl->bloc->membres.verrou_ecriture()) {
			auto decl_var = it->comme_decl_var();

			for (auto &donnees : decl_var->donnees_decl) {
				for (auto i = 0; i < donnees.variables.taille; ++i) {
					auto var = donnees.variables[i];

					if (var->type->est_rien()) {
						rapporte_erreur("Ne peut avoir un type « rien » dans une union", decl_var, erreur::Genre::TYPE_DIFFERENTS);
						return true;
					}

					if (var->type->est_structure() || var->type->est_union()) {
						if ((var->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
							VERIFIE_UNITE_TYPAGE(var->type)
							unite->attend_sur_type(var->type);
							return true;
						}
					}

					if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
						rapporte_erreur("Expression invalide dans la déclaration du membre de l'union", var);
						return true;
					}

					if (verifie_inclusion_valeur(var)) {
						return true;
					}

					/* l'arbre syntaxique des expressions par défaut doivent contenir
					 * la transformation puisque nous n'utilisons pas la déclaration
					 * pour générer la RI */
					auto expression = donnees.expression;
					transtype_si_necessaire(expression, donnees.transformations[i]);

					// À FAIRE : préserve l'emploi dans les données types
					if (ajoute_donnees_membre(var, expression)) {
						return true;
					}
				}
			}
		}

		calcule_taille_type_compose(type_union);

		if (!decl->est_nonsure) {
			type_union->cree_type_structure(espace->typeuse, type_union->decalage_index);
		}

		decl->type->drapeaux |= TYPE_FUT_VALIDE;

		POUR (type_compose->membres) {
			graphe->connecte_type_type(type_compose, it.type);
		}

		graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
		decl->bloc_parent->membres->pousse(decl);
		return false;
	}

	auto type_struct = type_compose->comme_structure();

	POUR (*decl->bloc->membres.verrou_lecture()) {
		if (dls::outils::est_element(it->genre, GenreNoeud::DECLARATION_STRUCTURE, GenreNoeud::DECLARATION_ENUM)) {
			// utilisation d'un type de données afin de pouvoir automatiquement déterminer un type
			auto type_de_donnees = espace->typeuse.type_type_de_donnees(it->type);
			type_compose->membres.pousse({ type_de_donnees, it->ident->nom, 0, 0, nullptr, TypeCompose::Membre::EST_CONSTANT });

			// l'utilisation d'un type de données brise le graphe de dépendance
			donnees_dependance.types_utilises.insere(it->type);
			continue;
		}

		if (it->possede_drapeau(EMPLOYE)) {
			type_struct->types_employes.pousse(it->type->comme_structure());
			continue;
		}

		if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
			rapporte_erreur("Déclaration inattendu dans le bloc de la structure", it);
			return true;
		}

		auto decl_var = it->comme_decl_var();

		if (decl_var->possede_drapeau(EST_CONSTANTE)) {
			type_compose->membres.pousse({ it->type, it->ident->nom, 0, 0, decl_var->expression, TypeCompose::Membre::EST_CONSTANT });
			continue;
		}

		if (decl_var->declaration_vient_d_un_emploi) {
			// À FAIRE : préserve l'emploi dans les données types
			if (ajoute_donnees_membre(decl_var, decl_var->expression)) {
				return true;
			}

			continue;
		}

		for (auto &donnees : decl_var->donnees_decl) {
			for (auto i = 0; i < donnees.variables.taille; ++i) {
				auto var = donnees.variables[i];

				if (var->type->est_rien()) {
					rapporte_erreur("Ne peut avoir un type « rien » dans une structure", decl_var, erreur::Genre::TYPE_DIFFERENTS);
					return true;
				}

				if (var->type->est_structure() || var->type->est_union()) {
					if ((var->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
						VERIFIE_UNITE_TYPAGE(var->type)
						unite->attend_sur_type(var->type);
						return true;
					}
				}

				if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					rapporte_erreur("Expression invalide dans la déclaration du membre de la structure", var);
					return true;
				}

				if (verifie_inclusion_valeur(var)) {
					return true;
				}

				/* l'arbre syntaxique des expressions par défaut doivent contenir
				 * la transformation puisque nous n'utilisons pas la déclaration
				 * pour générer la RI */
				auto expression = donnees.expression;
				transtype_si_necessaire(expression, donnees.transformations[i]);

				// À FAIRE : préserve l'emploi dans les données types
				if (ajoute_donnees_membre(var, expression)) {
					return true;
				}
			}
		}
	}

	POUR (*decl->bloc->expressions.verrou_ecriture()) {
		if (it->est_assignation()) {
			auto expr_assign = it->comme_assignation();
			auto variable = expr_assign->variable;

			for (auto &membre : type_compose->membres) {
				if (membre.nom == variable->ident->nom) {
					membre.expression_valeur_defaut = expr_assign->expression;
					break;
				}
			}
		}
	}

	calcule_taille_type_compose(type_compose);
	decl->type->drapeaux |= TYPE_FUT_VALIDE;
	decl->drapeaux |= DECLARATION_FUT_VALIDEE;

	POUR (type_compose->membres) {
		graphe->connecte_type_type(type_compose, it.type);
	}

	graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
	return false;
}

bool ContexteValidationCode::valide_declaration_variable(NoeudDeclarationVariable *decl)
{
	struct DeclarationEtReference {
		NoeudExpression *ref_decl = nullptr;
		NoeudDeclarationVariable *decl = nullptr;
	};

	auto feuilles_variables = dls::tablet<NoeudExpression *, 6>();
	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "rassemble variables");
		rassemble_expressions(decl->valeur, feuilles_variables);
	}

	/* Rassemble les variables, et crée des déclarations si nécessaire. */
	auto decls_et_refs = dls::tablet<DeclarationEtReference, 6>();
	decls_et_refs.redimensionne(feuilles_variables.taille());
	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "préparation");

		if (feuilles_variables.taille() == 1) {
			auto variable = feuilles_variables[0];

			if (!variable->est_ref_decl()) {
				rapporte_erreur("Expression inattendue à gauche de la déclaration", variable);
			}

			decls_et_refs[0].ref_decl = variable;
			decls_et_refs[0].decl = decl;
			variable->comme_ref_decl()->decl = decl;
		}
		else {
			for (auto i = 0; i < feuilles_variables.taille(); ++i) {
				auto variable = feuilles_variables[i];

				if (!variable->est_ref_decl()) {
					rapporte_erreur("Expression inattendue à gauche de la déclaration", variable);
				}

				// crée de nouvelles déclaration pour les différentes opérandes
				auto decl_extra = static_cast<NoeudDeclarationVariable *>(m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, variable->lexeme));
				decl_extra->ident = variable->ident;
				decl_extra->valeur = variable;
				decl_extra->bloc_parent = decl->bloc_parent;
				decl->bloc_parent->membres->pousse(decl_extra);

				decls_et_refs[i].ref_decl = variable;
				decls_et_refs[i].decl = decl_extra;
				variable->comme_ref_decl()->decl = decl_extra;
			}
		}
	}

	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "typage et redéfinition");

		POUR (decls_et_refs) {
			auto decl_prec = trouve_dans_bloc(it.decl->bloc_parent, it.decl);

			if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
				if (decl->lexeme->ligne > decl_prec->lexeme->ligne) {
					rapporte_erreur_redefinition_symbole(it.ref_decl, decl_prec);
					return true;
				}
			}

			if (resoud_type_final(it.ref_decl->expression_type, it.ref_decl->type)) {
				return true;
			}
		}
	}

	auto feuilles_expressions = dls::tablet<NoeudExpression *, 6>();
	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "rassemble expressions");
		rassemble_expressions(decl->expression, feuilles_expressions);
	}

	// pour chaque expression, associe les variables qui doivent recevoir leurs résultats
	// si une variable n'a pas de valeur, prend la valeur de la dernière expression

	auto variables = dls::file<NoeudExpression *>();
	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "enfile variables");

		POUR (feuilles_variables) {
			variables.enfile(it);
		}
	}

	dls::tablet<DonneesAssignations, 6> donnees_assignations{};

	auto ajoute_variable = [this, decl](DonneesAssignations &donnees, NoeudExpression *variable, NoeudExpression *expression, Type *type_de_l_expression) -> bool
	{
		if (variable->type == nullptr) {
			if (type_de_l_expression->genre == GenreType::ENTIER_CONSTANT) {
				variable->type = espace->typeuse[TypeBase::Z32];
				donnees.variables.pousse(variable);
				donnees.transformations.pousse({ TypeTransformation::CONVERTI_ENTIER_CONSTANT, variable->type });
			}
			else {
				variable->type = type_de_l_expression;
				donnees.variables.pousse(variable);
				donnees.transformations.pousse({ TypeTransformation::INUTILE });
			}
		}
		else {
			auto transformation = TransformationType();
			if (cherche_transformation(*espace, *this, type_de_l_expression, variable->type, transformation)) {
				return false;
			}

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				rapporte_erreur_assignation_type_differents(variable->type, type_de_l_expression, expression);
				return false;
			}

			donnees.variables.pousse(variable);
			donnees.transformations.pousse(transformation);
		}

		if (decl->drapeaux & EST_CONSTANTE && !type_de_l_expression->est_type_de_donnees()) {
			auto res_exec = evalue_expression(espace, decl->bloc_parent, expression);

			if (res_exec.est_errone) {
				rapporte_erreur("Impossible d'évaluer l'expression de la constante", expression);
				return false;
			}

			decl->valeur_expression = res_exec;
		}

		return true;
	};

	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "assignation expressions");

		POUR (feuilles_expressions) {
			auto donnees = DonneesAssignations();
			donnees.expression = it;

			// il est possible d'ignorer les variables
			if  (variables.est_vide()) {
				::rapporte_erreur(espace, decl, "Trop d'expressions ou de types pour l'assignation");
				return true;
			}

			if ((decl->drapeaux & EST_CONSTANTE) && it->est_non_initialisation()) {
				rapporte_erreur("Impossible de ne pas initialiser une constante", it);
				return true;
			}

			if (decl->drapeaux & EST_EXTERNE) {
				rapporte_erreur("Ne peut pas assigner une variable globale externe dans sa déclaration", decl);
				return true;
			}

			if (it->type == nullptr && !it->est_non_initialisation()) {
				rapporte_erreur("impossible de définir le type de l'expression", it);
				return true;
			}

			if (it->est_non_initialisation()) {
				donnees.variables.pousse(variables.defile());
				donnees.transformations.pousse({ TypeTransformation::INUTILE });
			}
			else if (it->est_appel() && it->comme_appel()->noeud_fonction_appelee && it->comme_appel()->noeud_fonction_appelee->type->est_fonction()) {
				auto type_fonction = it->comme_appel()->noeud_fonction_appelee->type->comme_fonction();

				donnees.multiple_retour = type_fonction->types_sorties.taille > 1;

				for (auto type : type_fonction->types_sorties) {
					if (variables.est_vide()) {
						break;
					}

					if (!ajoute_variable(donnees, variables.defile(), it, type)) {
						return true;
					}
				}
			}
			else if (it->type->est_rien()) {
				rapporte_erreur("impossible d'assigner une expression de type « rien » à une variable", it, erreur::Genre::ASSIGNATION_RIEN);
				return true;
			}
			else {
				if (!ajoute_variable(donnees, variables.defile(), it, it->type)) {
					return true;
				}
			}

			donnees_assignations.pousse(donnees);
		}

		if (donnees_assignations.est_vide()) {
			donnees_assignations.pousse({});
		}

		// a, b := c
		auto donnees = &donnees_assignations.back();
		while (!variables.est_vide()) {
			auto var = variables.defile();
			auto transformation = TransformationType(TypeTransformation::INUTILE);

			if (donnees->expression) {
				var->type = donnees->expression->type;

				if (var->type->est_entier_constant()) {
					var->type = espace->typeuse[TypeBase::Z32];
					transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, var->type };
				}
			}

			if (fonction_courante && fonction_courante->possede_drapeau(FORCE_NULCTX) && (var->type->est_structure() || var->type->est_union()) && !var->comme_ref_decl()->decl->possede_drapeau(EST_PARAMETRE)) {
				::rapporte_erreur(espace, var, "Impossible d'initialiser par défaut une structure dans un bloc n'ayant pas de contexte");
			}

			donnees->variables.pousse(var);
			donnees->transformations.pousse(transformation);
		}
	}

	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "validation finale");

		POUR (decls_et_refs) {
			auto decl_var = it.decl;
			auto variable = it.ref_decl;

			if (variable->type == nullptr) {
				rapporte_erreur("variable déclarée sans type", variable);
				return true;
			}

			decl_var->type = variable->type;

			if (decl_var->drapeaux & EST_GLOBALE) {
				auto graphe = espace->graphe_dependance.verrou_ecriture();
				graphe->cree_noeud_globale(decl_var);
			}

			auto bloc_parent = decl_var->bloc_parent;
			bloc_parent->membres->pousse(decl_var);

			decl_var->drapeaux |= DECLARATION_FUT_VALIDEE;
			donnees_dependance.types_utilises.insere(decl_var->type);
		}
	}

	{
		CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "copie données");

		decl->donnees_decl.reserve(donnees_assignations.taille());

		POUR (donnees_assignations) {
			decl->donnees_decl.pousse(std::move(it));
		}
	}

	return false;
}

bool ContexteValidationCode::valide_assignation(NoeudAssignation *inst)
{
	CHRONO_TYPAGE(m_tacheronne.stats_typage.assignations, "valide assignation");
	auto variable = inst->variable;

	dls::file<NoeudExpression *> variables;

	if (variable->est_virgule()) {
		auto virgule = variable->comme_virgule();
		POUR (virgule->expressions) {
			if (!est_valeur_gauche(it->genre_valeur)) {
				rapporte_erreur("Impossible d'assigner une expression à une valeur-droite !", inst, erreur::Genre::ASSIGNATION_INVALIDE);
				return true;
			}

			variables.enfile(it);
		}
	}
	else {
		if (!est_valeur_gauche(variable->genre_valeur)) {
			rapporte_erreur("Impossible d'assigner une expression à une valeur-droite !", inst, erreur::Genre::ASSIGNATION_INVALIDE);
			return true;
		}

		variables.enfile(variable);
	}

	dls::tablet<NoeudExpression *, 6> expressions;
	rassemble_expressions(inst->expression, expressions);

	auto ajoute_variable = [this](DonneesAssignations &donnees, NoeudExpression *var, NoeudExpression *expression, Type *type_de_l_expression) -> bool
	{
		auto transformation = TransformationType();
		if (cherche_transformation(*espace, *this, type_de_l_expression, var->type, transformation)) {
			return false;
		}

		if (transformation.type == TypeTransformation::IMPOSSIBLE) {
			rapporte_erreur_assignation_type_differents(var->type, type_de_l_expression, expression);
			return false;
		}

		donnees.variables.pousse(var);
		donnees.transformations.pousse(transformation);
		return true;
	};

	dls::tablet<DonneesAssignations, 6> donnees_assignations;

	POUR (expressions) {
		if (it->est_non_initialisation()) {
			rapporte_erreur("Impossible d'utiliser '---' dans une expression d'assignation", inst->expression);
			return true;
		}

		if (it->type == nullptr) {
			rapporte_erreur("Impossible de définir le type de la variable !", inst, erreur::Genre::TYPE_INCONNU);
			return true;
		}

		if (it->type->est_rien()) {
			rapporte_erreur("Impossible d'assigner une expression de type 'rien' à une variable !", inst, erreur::Genre::ASSIGNATION_RIEN);
			return true;
		}

		auto donnees = DonneesAssignations();
		donnees.expression = it;

		if (it->est_appel() && it->comme_appel()->noeud_fonction_appelee && it->comme_appel()->noeud_fonction_appelee->type->est_fonction()) {
			auto type_fonction = it->comme_appel()->noeud_fonction_appelee->type->comme_fonction();

			donnees.multiple_retour = type_fonction->types_sorties.taille > 1;

			for (auto type : type_fonction->types_sorties) {
				if (variables.est_vide()) {
					break;
				}

				if (!ajoute_variable(donnees, variables.defile(), it, type)) {
					return true;
				}
			}
		}
		else {
			if (!ajoute_variable(donnees, variables.defile(), it, it->type)) {
				return true;
			}
		}

		donnees_assignations.pousse(donnees);
	}

	// a, b = c
	auto donnees = &donnees_assignations.back();
	while (!variables.est_vide()) {
		if (!ajoute_variable(*donnees, variables.defile(), donnees->expression, donnees->expression->type)) {
			return true;
		}
	}

	inst->donnees_exprs.reserve(donnees_assignations.taille());
	POUR (donnees_assignations) {
		inst->donnees_exprs.pousse(it);
	}

	return false;
}

/* ************************************************************************** */

bool ContexteValidationCode::resoud_type_final(NoeudExpression *expression_type, Type *&type_final)
{
	Prof(resoud_type_final);

	if (expression_type == nullptr) {
		type_final = nullptr;
		return false;
	}

	auto type_var = expression_type->type;

	if (type_var->genre != GenreType::TYPE_DE_DONNEES) {
		rapporte_erreur("attendu un type de données", expression_type);
		return true;
	}

	auto type_de_donnees = type_var->comme_type_de_donnees();

	if (type_de_donnees->type_connu == nullptr) {
		rapporte_erreur("impossible de définir le type selon l'expression", expression_type);
		return true;
	}

	type_final = type_de_donnees->type_connu;
	return false;
}

void ContexteValidationCode::rapporte_erreur(const char *message, NoeudExpression *noeud)
{
	erreur::lance_erreur(message, *espace, noeud->lexeme);
}

void ContexteValidationCode::rapporte_erreur(const char *message, NoeudExpression *noeud, erreur::Genre genre)
{
	erreur::lance_erreur(message, *espace, noeud->lexeme, genre);
}

void ContexteValidationCode::rapporte_erreur_redefinition_symbole(NoeudExpression *decl, NoeudDeclaration *decl_prec)
{
	erreur::redefinition_symbole(*espace, decl->lexeme, decl_prec->lexeme);
}

void ContexteValidationCode::rapporte_erreur_redefinition_fonction(NoeudDeclarationEnteteFonction *decl, NoeudDeclaration *decl_prec)
{
	erreur::redefinition_fonction(*espace, decl_prec->lexeme, decl->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf)
{
	erreur::lance_erreur_type_arguments(type_arg->type, type_enf->type, *espace, type_enf->lexeme, type_arg->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_retour(const Type *type_arg, const Type *type_enf, NoeudExpression *racine)
{
	erreur::lance_erreur_type_retour(type_arg, type_enf, *espace, racine);
}

void ContexteValidationCode::rapporte_erreur_assignation_type_differents(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud)
{
	erreur::lance_erreur_assignation_type_differents(type_gauche, type_droite, *espace, noeud->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_operation(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud)
{
	erreur::lance_erreur_type_operation(type_gauche, type_droite, *espace, noeud->lexeme);
}

void ContexteValidationCode::rapporte_erreur_type_indexage(NoeudExpression *noeud)
{
	erreur::type_indexage(*espace, noeud);
}

void ContexteValidationCode::rapporte_erreur_type_operation(NoeudExpression *noeud)
{
	erreur::lance_erreur_type_operation(*espace, noeud);
}

void ContexteValidationCode::rapporte_erreur_type_operation_unaire(NoeudExpression *noeud)
{
	erreur::lance_erreur_type_operation_unaire(*espace, noeud);
}

void ContexteValidationCode::rapporte_erreur_acces_hors_limites(NoeudExpression *b, TypeTableauFixe *type_tableau, long index_acces)
{
	erreur::lance_erreur_acces_hors_limites(*espace, b, type_tableau->taille, type_tableau, index_acces);
}

void ContexteValidationCode::rapporte_erreur_membre_inconnu(NoeudExpression *acces, NoeudExpression *structure, NoeudExpression *membre, TypeCompose *type)
{
	erreur::membre_inconnu(*espace, acces, structure, membre, type);
}

void ContexteValidationCode::rapporte_erreur_membre_inactif(NoeudExpression *acces, NoeudExpression *structure, NoeudExpression *membre)
{
	erreur::membre_inactif(*espace, *this, acces, structure, membre);
}

void ContexteValidationCode::rapporte_erreur_valeur_manquante_discr(NoeudExpression *expression, dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes)
{
	erreur::valeur_manquante_discr(*espace, expression, valeurs_manquantes);
}

void ContexteValidationCode::rapporte_erreur_fonction_inconnue(NoeudExpression *b, const dls::tablet<DonneesCandidate, 10> &candidates)
{
	erreur::lance_erreur_fonction_inconnue(*espace, b, candidates);
}

void ContexteValidationCode::rapporte_erreur_fonction_nulctx(const NoeudExpression *appl_fonc, const NoeudExpression *decl_fonc, const NoeudExpression *decl_appel)
{
	erreur::lance_erreur_fonction_nulctx(*espace, appl_fonc, decl_fonc, decl_appel);
}

bool ContexteValidationCode::transtype_si_necessaire(NoeudExpression *&expression, Type *type_cible)
{
	auto transformation = TransformationType();
	if (cherche_transformation(*espace, *this, expression->type, type_cible, transformation)) {
		return false;
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		rapporte_erreur_assignation_type_differents(type_cible, expression->type, expression);
		return false;
	}

	return transtype_si_necessaire(expression, transformation);
}

bool ContexteValidationCode::transtype_si_necessaire(NoeudExpression *&expression, TransformationType const &transformation)
{
	if (transformation.type == TypeTransformation::INUTILE) {
		return true;
	}

	if (transformation.type == TypeTransformation::CONVERTI_ENTIER_CONSTANT) {
		expression->type = transformation.type_cible;
		return expression;
	}

	auto type_cible = transformation.type_cible;

	if (type_cible == nullptr) {
		if (transformation.type == TypeTransformation::CONSTRUIT_EINI) {
			type_cible = espace->typeuse[TypeBase::EINI];
		}
		else if (transformation.type == TypeTransformation::CONVERTI_VERS_PTR_RIEN) {
			type_cible = espace->typeuse[TypeBase::PTR_RIEN];
		}
		else if (transformation.type == TypeTransformation::PREND_REFERENCE) {
			type_cible = espace->typeuse.type_reference_pour(expression->type);
		}
		else if (transformation.type == TypeTransformation::DEREFERENCE) {
			type_cible = type_dereference_pour(expression->type);
		}
		else if (transformation.type == TypeTransformation::CONSTRUIT_TABL_OCTET) {
			type_cible = espace->typeuse[TypeBase::TABL_OCTET];
		}
		else if (transformation.type == TypeTransformation::CONVERTI_TABLEAU) {
			auto type_tableau_fixe = expression->type->comme_tableau_fixe();
			type_cible = espace->typeuse.type_tableau_dynamique(type_tableau_fixe->type_pointe);
		}
		else {
			assert_rappel(false, [&]() { std::cerr << "Type Transformation non géré : " << transformation.type << '\n'; });
		}
	}

	auto noeud_comme = m_tacheronne.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_COMME, expression->lexeme)->comme_comme();
	noeud_comme->type = type_cible;
	noeud_comme->expression = expression;
	noeud_comme->transformation = transformation;
	noeud_comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

	expression = noeud_comme;

	return true;
}
