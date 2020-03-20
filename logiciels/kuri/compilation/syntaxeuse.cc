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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "syntaxeuse.hh"

#undef DEBOGUE_EXPRESSION

#include <filesystem>

#include "biblinternes/langage/debogage.hh"
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "expression.h"
#include "outils_lexemes.hh"

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

/* ************************************************************************** */

// Pour les bibliothèques externes ou les inclusions, détermine le chemin absolu selon le fichier courant, au cas où la bibliothèque serait dans le même dossier que le fichier
static auto trouve_chemin_si_dans_dossier(DonneesModule *module, dls::chaine const &chaine)
{
	/* vérifie si le chemin est relatif ou absolu */
	auto chemin = std::filesystem::path(chaine.c_str());

	if (!std::filesystem::exists(chemin)) {
		/* le chemin n'est pas absolu, détermine s'il est dans le même dossier */
		auto chemin_abs = module->chemin + chaine;

		chemin = std::filesystem::path(chemin_abs.c_str());

		if (std::filesystem::exists(chemin)) {
			return chemin_abs;
		}
	}

	return chaine;
}

/* ************************************************************************** */

/**
 * Retourne vrai si le noeud passé en paramètre peut-être un noeud
 * valide pour précèder un opérateur unaire ('+', '-', etc.).
 */
static bool precede_unaire_valide(NoeudExpression *dernier_noeud)
{
	if (dernier_noeud == nullptr) {
		return true;
	}

	if (dernier_noeud->genre == GenreNoeud::DECLARATION_VARIABLE) {
		return true;
	}

	if (dernier_noeud->genre == GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
		return true;
	}

	if (dernier_noeud->genre == GenreNoeud::OPERATEUR_UNAIRE) {
		return true;
	}

	if (dernier_noeud->genre == GenreNoeud::OPERATEUR_BINAIRE) {
		return true;
	}

	return false;
}

#define CREE_NOEUD(Type, Genre, Lexeme) static_cast<Type *>(m_fichier->module->assembleuse->cree_noeud(Genre, Lexeme))

/* ************************************************************************** */

Syntaxeuse::Syntaxeuse(
		ContexteGenerationCode &contexte,
		Fichier *fichier,
		dls::chaine const &racine_kuri)
	: lng::analyseuse<DonneesLexeme>(fichier->lexemes)
	, m_contexte(contexte)
	, m_paires_vecteurs(PROFONDEUR_EXPRESSION_MAX)
	, m_racine_kuri(racine_kuri)
	, m_fichier(fichier)
{}

void Syntaxeuse::lance_analyse(std::ostream &os)
{
	m_position = 0;

	if (m_identifiants.taille() == 0) {
		return;
	}

	m_fichier->temps_analyse = 0.0;
	m_chrono_analyse.commence();
	analyse_expression_haut_niveau(os);
	m_fichier->temps_analyse += m_chrono_analyse.arrete();
}

void Syntaxeuse::analyse_expression_haut_niveau(std::ostream &os)
{
	while (!fini()) {
		auto id = this->identifiant_courant();

		switch (id) {
			case GenreLexeme::IMPORTE:
			{
				avance();

				if (!est_identifiant(GenreLexeme::CHAINE_LITTERALE) && !est_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
					lance_erreur("Attendu une chaine littérale après 'importe'");
				}

				avance();

				auto const nom_module = donnees().chaine;
				m_fichier->modules_importes.insere(nom_module);

				/* désactive le 'chronomètre' car sinon le temps d'analyse prendra
				 * également en compte le chargement, le découpage, et l'analyse du
				 * module importé */
				m_fichier->temps_analyse += m_chrono_analyse.arrete();
				importe_module(os, m_racine_kuri, dls::chaine(nom_module), m_contexte, donnees());
				m_chrono_analyse.reprend();
				break;
			}
			case GenreLexeme::CHARGE:
			{
				avance();

				if (!est_identifiant(GenreLexeme::CHAINE_LITTERALE) && !est_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
					lance_erreur("Attendu une chaine littérale après 'charge'");
				}

				avance();

				auto const nom_fichier = donnees().chaine;

				/* désactive le 'chronomètre' car sinon le temps d'analyse prendra
				 * également en compte le chargement, le découpage, et l'analyse du
				 * fichier chargé */
				m_fichier->temps_analyse += m_chrono_analyse.arrete();
				charge_fichier(os, m_fichier->module, m_racine_kuri, dls::chaine(nom_fichier), m_contexte, donnees());
				m_chrono_analyse.reprend();

				break;
			}
			default:
			{
				m_global = true;
				auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::INCONNU);

				if (noeud != nullptr) {
					if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
						auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
						decl_var->valeur = noeud;

						decl_var->bloc_parent->membres.pousse(decl_var);
						decl_var->bloc_parent->expressions.pousse(decl_var);
						decl_var->drapeaux |= EST_GLOBALE;
						m_contexte.file_typage.pousse(decl_var);
					}
					else if (est_declaration(noeud->genre)) {
						noeud->bloc_parent->membres.pousse(static_cast<NoeudDeclaration *>(noeud));
						noeud->bloc_parent->expressions.pousse(noeud);
						noeud->drapeaux |= EST_GLOBALE;
						m_contexte.file_typage.pousse(noeud);
					}
					else {
						noeud->bloc_parent->expressions.pousse(noeud);
						m_contexte.file_typage.pousse(noeud);
					}
				}

				m_global = false;
				break;
			}
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_declaration_fonction(GenreLexeme id, DonneesLexeme &lexeme)
{
	auto externe = false;

	if (est_identifiant(GenreLexeme::EXTERNE)) {
		avance();
		externe = true;

		if (id == GenreLexeme::COROUT && externe) {
			lance_erreur("Une coroutine ne peut pas être externe");
		}
	}

	m_fichier->module->fonctions_exportees.insere(lexeme.chaine);

	auto noeud = CREE_NOEUD(NoeudDeclarationFonction, GenreNoeud::DECLARATION_FONCTION, &lexeme);
	noeud->est_coroutine = id == GenreLexeme::COROUT;

	if (externe) {
		noeud->drapeaux |= EST_EXTERNE;
		noeud->est_externe = true;
	}

	if (m_etiquette_enligne) {
		noeud->drapeaux |= FORCE_ENLIGNE;
		m_etiquette_enligne = false;
	}
	else if (m_etiquette_horsligne) {
		noeud->drapeaux |= FORCE_HORSLIGNE;
		m_etiquette_horsligne = false;
	}

	if (m_etiquette_nulctx) {
		noeud->drapeaux |= FORCE_NULCTX;
		m_etiquette_nulctx = false;
	}

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	/* analyse les paramètres de la fonction */
	while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression(GenreLexeme::VIRGULE, type_id::FONC);

		if (param->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
			decl_var->valeur = param;

			noeud->params.pousse(decl_var);
		}
		else {
			noeud->params.pousse(static_cast<NoeudDeclaration *>(param));
		}
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction, À FAIRE : inférence */

	consomme(GenreLexeme::RETOUR_TYPE, "Attendu un retour de type");

	while (true) {
		noeud->noms_retours.pousse("__ret" + dls::vers_chaine(noeud->noms_retours.taille()));

		auto type_declare = analyse_declaration_type(false);
		noeud->type_declare.types_sorties.pousse(type_declare);

		if (est_identifiant(type_id::ACCOLADE_OUVRANTE) || est_identifiant(type_id::POINT_VIRGULE)) {
			break;
		}

		if (est_identifiant(type_id::VIRGULE)) {
			avance();
		}
	}

	if (externe) {
		consomme(GenreLexeme::POINT_VIRGULE, "Attendu un point-virgule ';' après la déclaration de la fonction externe");

		if (noeud->type_declare.types_sorties.taille() > 1) {
			lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
		}
	}
	else {
		/* ignore les points-virgules implicites */
		if (est_identifiant(GenreLexeme::POINT_VIRGULE)) {
			avance();
		}

		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après la liste des paramètres de la fonction");

		noeud->bloc = analyse_bloc();
		aplatis_arbre(noeud->bloc, noeud->arbre_aplatis);

//		std::cerr << "Abre aplatis pour fonction " << noeud->ident->nom << " :\n";

//		POUR (noeud->arbre_aplatis) {
//			std::cerr << "-- " << chaine_genre_noeud(it->genre) << '\n';
//		}
	}

	return noeud;
}

void Syntaxeuse::analyse_controle_si(GenreNoeud tn)
{
	auto noeud = CREE_NOEUD(NoeudSi, tn, &donnees());
	noeud->bloc_parent->expressions.pousse(noeud);

	noeud->condition = analyse_expression(GenreLexeme::ACCOLADE_OUVRANTE, GenreLexeme::SI);

	noeud->bloc_si_vrai = analyse_bloc();

	if (est_identifiant(GenreLexeme::SINON)) {
		avance();

		/* Peu importe que le 'sinon' contient un 'si' ou non, nous ajoutons un
		 * bloc pour créer un niveau d'indirection. Car dans le cas où nous
		 * avons un contrôle du type si/sinon si dans une boucle, la génération
		 * de blocs LLVM dans l'arbre syntactic devient plus compliquée sans
		 * cette indirection : certaines instructions de branchage ne sont pas
		 * ajoutées alors qu'elles devraient l'être et la logique pour
		 * correctement traiter ce cas sans l'indirection semble être complexe.
		 * LLVM devrait pouvoir effacer cette indirection en enlevant les
		 * branchements redondants. */
		noeud->bloc_si_faux = m_fichier->module->assembleuse->empile_bloc();

		if (est_identifiant(GenreLexeme::SI)) {
			avance();
			analyse_controle_si(GenreNoeud::INSTRUCTION_SI);
		}
		else if (est_identifiant(GenreLexeme::SAUFSI)) {
			avance();
			analyse_controle_si(GenreNoeud::INSTRUCTION_SAUFSI);
		}
		else {
			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après 'sinon'");

			analyse_corps_fonction();

			consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante à la fin du contrôle 'sinon'");
		}

		m_fichier->module->assembleuse->depile_bloc();
	}
}

void Syntaxeuse::analyse_controle_pour()
{
	auto noeud = CREE_NOEUD(NoeudPour, GenreNoeud::INSTRUCTION_POUR, &donnees());
	noeud->bloc_parent->expressions.pousse(noeud);

	/* enfant 1 : déclaration variable */
	noeud->variable = analyse_expression(GenreLexeme::DANS, GenreLexeme::POUR);

	/* enfant 2 : expr */
	noeud->expression = analyse_expression(GenreLexeme::ACCOLADE_OUVRANTE, GenreLexeme::DANS);

	recule();

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'pour'");

	/* enfant 3 : bloc */
	noeud->bloc = analyse_bloc();

	/* enfant 4 : bloc sansarrêt (optionel) */
	if (est_identifiant(GenreLexeme::SANSARRET)) {
		avance();

		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'sansarrêt'");

		noeud->bloc_sansarret = analyse_bloc();
	}

	/* enfant 4 ou 5 : bloc sinon (optionel) */
	if (est_identifiant(GenreLexeme::SINON)) {
		avance();

		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");

		noeud->bloc_sinon = analyse_bloc();
	}
}

void Syntaxeuse::analyse_corps_fonction()
{
	/* Il est possible qu'une fonction soit vide, donc vérifie d'abord que
	 * l'on n'ait pas terminé. */
	while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
		auto const pos = position();

		if (est_identifiant(GenreLexeme::RETOURNE)) {
			avance();
			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_RETOUR, &donnees());
			noeud->bloc_parent->expressions.pousse(noeud);

			/* Considération du cas où l'on ne retourne rien 'retourne;'. */
			if (!est_identifiant(GenreLexeme::POINT_VIRGULE)) {
				noeud->expr = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::RETOURNE);
			}
			else {
				avance();
			}

		}
		else if (est_identifiant(GenreLexeme::RETIENS)) {
			avance();
			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_RETIENS, &donnees());
			noeud->bloc_parent->expressions.pousse(noeud);
			noeud->expr = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::RETOURNE);
		}
		else if (est_identifiant(GenreLexeme::POUR)) {
			avance();
			analyse_controle_pour();
		}
		else if (est_identifiant(GenreLexeme::BOUCLE)) {
			avance();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'boucle'");

			auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_BOUCLE, &donnees());
			noeud->bloc_parent->expressions.pousse(noeud);
			noeud->bloc = analyse_bloc();
		}
		else if (est_identifiant(GenreLexeme::REPETE)) {
			avance();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'répète'");

			auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_REPETE, &donnees());
			noeud->bloc_parent->expressions.pousse(noeud);

			noeud->bloc = analyse_bloc();

			consomme(GenreLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

			noeud->condition = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::TANTQUE);
		}
		else if (est_identifiant(GenreLexeme::TANTQUE)) {
			avance();

			auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_TANTQUE, &donnees());
			noeud->bloc_parent->expressions.pousse(noeud);

			noeud->condition = analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::TANTQUE);

			/* recule pour être de nouveau synchronisé */
			recule();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de 'tanque'");

			noeud->bloc = analyse_bloc();
		}
		else if (est_identifiant(GenreLexeme::ARRETE) || est_identifiant(GenreLexeme::CONTINUE)) {
			avance();

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, &donnees());
			noeud->bloc_parent->expressions.pousse(noeud);

			if (est_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
				avance();
				noeud->expr = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, &donnees());
			}

			consomme(GenreLexeme::POINT_VIRGULE, "Attendu un point virgule ';'");
		}
		else if (est_identifiant(GenreLexeme::DIFFERE)) {
			avance();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'diffère'");

			auto bloc = analyse_bloc();
			bloc->bloc_parent->expressions.pousse(bloc);
			bloc->est_differe = true;
		}
		else if (est_identifiant(GenreLexeme::NONSUR)) {
			avance();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'nonsûr'");

			auto bloc = analyse_bloc();
			bloc->bloc_parent->expressions.pousse(bloc);
			bloc->est_nonsur = true;
		}
		else if (est_identifiant(GenreLexeme::DISCR)) {
			avance();

			auto noeud_discr = CREE_NOEUD(NoeudDiscr, GenreNoeud::INSTRUCTION_DISCR, &donnees());
			noeud_discr->bloc_parent->expressions.pousse(noeud_discr);

			noeud_discr->expr = analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::DISCR);

			/* recule pour être de nouveau synchronisé */
			recule();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

			auto sinon_rencontre = false;

			while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {			
				if (est_identifiant(GenreLexeme::SINON)) {
					avance();

					if (sinon_rencontre) {
						lance_erreur("Redéfinition d'un bloc sinon");
					}

					sinon_rencontre = true;

					consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

					noeud_discr->bloc_sinon = analyse_bloc();
				}
				else {
					auto expr = analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::DISCR);

					/* recule pour être de nouveau synchronisé */
					recule();

					consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

					auto bloc = analyse_bloc();

					noeud_discr->paires_discr.pousse({ expr, bloc });
				}
			}

			consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc de « discr »");
		}
		else if (est_identifiant(type_id::ACCOLADE_OUVRANTE)) {
			avance();
			auto bloc = analyse_bloc();
			bloc->bloc_parent->expressions.pousse(bloc);
		}
		else if (est_identifiant(GenreLexeme::POUSSE_CONTEXTE)) {
			avance();
			auto noeud = CREE_NOEUD(NoeudPousseContexte, GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE, &donnees());

			noeud->expr = analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::POUSSE_CONTEXTE);
			noeud->bloc = analyse_bloc();

			noeud->bloc_parent->expressions.pousse(noeud);
		}
		else {
			auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::EGAL);

			if (noeud != nullptr) {
				if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
					decl_var->valeur = noeud;

					decl_var->bloc_parent->membres.pousse(decl_var);
					decl_var->bloc_parent->expressions.pousse(decl_var);
				}
				else if (est_declaration(noeud->genre)) {
					noeud->bloc_parent->membres.pousse(static_cast<NoeudDeclaration *>(noeud));
					noeud->bloc_parent->expressions.pousse(noeud);
				}
				else {
					noeud->bloc_parent->expressions.pousse(noeud);
				}
			}
		}

		/* Dans les fuzz-tests, c'est possible d'être bloqué dans une boucle
		 * infinie :
		 * - nous arrivons au dernier cas, analyse_expression_droite
		 * - dans l'analyse, le premier identifiant est une parenthèse fermante
		 * - puisque parenthèse fermante, on recule et on sors de la boucle
		 * - puisqu'on sors de la boucle, on avance et on retourne
		 * - donc recule + avance = on bouge pas.
		 *
		 * Pas sûr pour l'instant de la manière dont on pourrait résoudre ce
		 * problème.
		 */
		if (pos == position()) {
			lance_erreur("Boucle infini dans l'analyse du corps de la fonction");
		}
	}
}

NoeudBloc *Syntaxeuse::analyse_bloc()
{
	auto bloc = m_fichier->module->assembleuse->empile_bloc();
	analyse_corps_fonction();
	m_fichier->module->assembleuse->depile_bloc();

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc");

	return bloc;
}

NoeudExpression *Syntaxeuse::analyse_expression(
		GenreLexeme identifiant_final,
		GenreLexeme racine_expr)
{
	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */
	auto profondeur = m_profondeur++;

	if (profondeur >= m_paires_vecteurs.taille()) {
		lance_erreur("Excès de la pile d'expression autorisée");
	}

	auto &expressions = m_paires_vecteurs[profondeur].first;
	expressions.efface();

	auto &operateurs = m_paires_vecteurs[profondeur].second;
	operateurs.efface();

	auto transfere_operateurs_mineures_dans_expression = [&](GenreLexeme id_operateur)
	{
		while (!operateurs.est_vide()
			   && (precedence_faible(id_operateur, operateurs.back()->lexeme->genre)))
		{
			expressions.pousse(operateurs.back());
			operateurs.pop_back();
		}
	};

	auto dernier_noeud = static_cast<NoeudExpression *>(nullptr);

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	auto assignation = false;

	auto drapeaux = drapeaux_noeud::AUCUN;

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

	while (!requiers_identifiant(identifiant_final)) {
		auto &lexeme = donnees();

		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(lexeme.genre) << FIN_LOG_EXPRESSION;

		switch (lexeme.genre) {
			case GenreLexeme::EMPL:
			{
				drapeaux |= EMPLOYE;
				break;
			}
			case GenreLexeme::EXTERNE:
			{
				drapeaux |= (EST_EXTERNE | DECLARATION);
				break;
			}
			case GenreLexeme::CHAINE_CARACTERE:
			{
				auto variable_consommee = false;

				/* appel fonction : chaine + ( */
				if (est_identifiant(GenreLexeme::PARENTHESE_OUVRANTE)) {
					avance();

					dernier_noeud = analyse_appel_fonction(lexeme);
					expressions.pousse(dernier_noeud);

					variable_consommee = true;
				}
				/* construction structure : chaine + { */
				else if (dls::outils::est_element(racine_expr, GenreLexeme::EGAL, GenreLexeme::RETOURNE, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INFO_DE, GenreLexeme::FONC)
						 && est_identifiant(GenreLexeme::ACCOLADE_OUVRANTE))
				{
					avance();

					dernier_noeud = analyse_construction_structure(lexeme);
					expressions.pousse(dernier_noeud);

					// À FAIRE, XXX - désynchronisation dans l'analyse de la construction de structure ?
					if (racine_expr != GenreLexeme::CROCHET_OUVRANT && racine_expr != GenreLexeme::FONC) {
						termine_boucle = true;
					}

					variable_consommee = true;
				}
				else if (est_identifiant(GenreLexeme::DECLARATION_CONSTANTE)) {
					avance();

					switch (this->identifiant_courant()) {
						case GenreLexeme::FONC:
						case GenreLexeme::COROUT:
						{
							auto id_fonc = this->identifiant_courant();
							avance();
							dernier_noeud = analyse_declaration_fonction(id_fonc, lexeme);

							expressions.pousse(dernier_noeud);

							termine_boucle = true;
							variable_consommee = true;

							break;
						}
						case GenreLexeme::STRUCT:
						case GenreLexeme::UNION:
						{
							auto id_struct = this->identifiant_courant();
							avance();
							dernier_noeud = analyse_declaration_structure(id_struct, lexeme);

							expressions.pousse(dernier_noeud);

							termine_boucle = true;
							variable_consommee = true;

							break;
						}
						case GenreLexeme::ENUM:
						case GenreLexeme::ENUM_DRAPEAU:
						case GenreLexeme::ERREUR:
						{
							auto id = this->identifiant_courant();
							avance();
							dernier_noeud = analyse_declaration_enum(id, lexeme);

							expressions.pousse(dernier_noeud);

							termine_boucle = true;
							variable_consommee = true;

							break;
						}
						default:
						{
							recule();
						}
					}
				}

				/* variable : chaine */
				if (!variable_consommee) {
					dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, &lexeme);
					expressions.pousse(dernier_noeud);

					dernier_noeud->drapeaux |= drapeaux;
					drapeaux = drapeaux_noeud::AUCUN;

					/* nous avons la déclaration d'un type dans la structure */
					if ((racine_expr != type_id::TRANSTYPE && racine_expr != type_id::LOGE && racine_expr != type_id::RELOGE) && est_identifiant(GenreLexeme::DOUBLE_POINTS)) {
						dernier_noeud->type_declare = analyse_declaration_type();
						drapeaux |= DECLARATION;
					}
				}

				break;
			}
			case GenreLexeme::OPERATEUR:
			{
				dernier_noeud = analyse_declaration_operateur();
				dernier_noeud->drapeaux |= IGNORE_OPERATEUR;
				expressions.pousse(dernier_noeud);
				termine_boucle = true;

				break;
			}
			case GenreLexeme::NOMBRE_REEL:
			{
				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL, &lexeme);
				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::NOMBRE_BINAIRE:
			case GenreLexeme::NOMBRE_ENTIER:
			case GenreLexeme::NOMBRE_HEXADECIMAL:
			case GenreLexeme::NOMBRE_OCTAL:
			{
				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER, &lexeme);
				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::CHAINE_LITTERALE:
			{
				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_CHAINE, &lexeme);
				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::CARACTERE:
			{
				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_CARACTERE, &lexeme);
				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::NUL:
			{
				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_NUL, &lexeme);
				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::TAILLE_DE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_TAILLE_DE, &lexeme);
				dernier_noeud->valeur_calculee = analyse_declaration_type(false);

				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::INFO_DE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

				auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_INFO_DE, &lexeme);

				noeud->expr = analyse_expression(GenreLexeme::INCONNU, GenreLexeme::INFO_DE);

				/* vérifie mais n'avance pas */
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'taille_de'");

				expressions.pousse(noeud);
				dernier_noeud = noeud;
				break;
			}
			case GenreLexeme::VRAI:
			case GenreLexeme::FAUX:
			{
				lexeme.genre = GenreLexeme::BOOL;
				dernier_noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN, &lexeme);
				expressions.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::TRANSTYPE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'transtype'");

				auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_TRANSTYPE, &lexeme);

				noeud->expr = analyse_expression(GenreLexeme::DOUBLE_POINTS, GenreLexeme::TRANSTYPE);

				noeud->type_declare = analyse_declaration_type(false);

				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après la déclaration du type");

				expressions.pousse(noeud);
				dernier_noeud = noeud;
				break;
			}
			case GenreLexeme::MEMOIRE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

				auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_MEMOIRE, &lexeme);
				noeud->expr = analyse_expression(GenreLexeme::INCONNU, GenreLexeme::MEMOIRE);

				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

				expressions.pousse(noeud);
				dernier_noeud = noeud;
				break;
			}
			case GenreLexeme::PARENTHESE_OUVRANTE:
			{
				auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_PARENTHESE, &lexeme);
				noeud->expr = analyse_expression(GenreLexeme::PARENTHESE_FERMANTE, GenreLexeme::PARENTHESE_OUVRANTE);

				expressions.pousse(noeud);
				dernier_noeud = noeud;

				break;
			}
			case GenreLexeme::PARENTHESE_FERMANTE:
			{
				/* recule pour être synchronisé avec les différentes sorties */
				recule();
				termine_boucle = true;
				break;
			}
			/* opérations binaire */
			case GenreLexeme::PLUS:
			case GenreLexeme::MOINS:
			{
				auto id_operateur = lexeme.genre;

				if (precede_unaire_valide(dernier_noeud)) {
					if (id_operateur == GenreLexeme::PLUS) {
						id_operateur = GenreLexeme::PLUS_UNAIRE;
					}
					else if (id_operateur == GenreLexeme::MOINS) {
						id_operateur = GenreLexeme::MOINS_UNAIRE;
					}

					lexeme.genre = id_operateur;
					dernier_noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::OPERATEUR_UNAIRE, &lexeme);
				}
				else {
					dernier_noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, &lexeme);
				}

				transfere_operateurs_mineures_dans_expression(id_operateur);

				operateurs.pousse(dernier_noeud);

				break;
			}
			case GenreLexeme::FOIS:
			case GenreLexeme::DIVISE:
			case GenreLexeme::ESPERLUETTE:
			case GenreLexeme::POURCENT:
			case GenreLexeme::INFERIEUR:
			case GenreLexeme::INFERIEUR_EGAL:
			case GenreLexeme::SUPERIEUR:
			case GenreLexeme::SUPERIEUR_EGAL:
			case GenreLexeme::DECALAGE_DROITE:
			case GenreLexeme::DECALAGE_GAUCHE:
			case GenreLexeme::DIFFERENCE:
			case GenreLexeme::ESP_ESP:
			case GenreLexeme::EGALITE:
			case GenreLexeme::BARRE_BARRE:
			case GenreLexeme::BARRE:
			case GenreLexeme::CHAPEAU:
			case GenreLexeme::PLUS_EGAL:
			case GenreLexeme::MOINS_EGAL:
			case GenreLexeme::DIVISE_EGAL:
			case GenreLexeme::MULTIPLIE_EGAL:
			case GenreLexeme::MODULO_EGAL:
			case GenreLexeme::ET_EGAL:
			case GenreLexeme::OU_EGAL:
			case GenreLexeme::OUX_EGAL:
			case GenreLexeme::DEC_DROITE_EGAL:
			case GenreLexeme::DEC_GAUCHE_EGAL:
			case GenreLexeme::VIRGULE:
			{
				/* Correction de crash d'aléatest, improbable dans la vrai vie. */
				if (expressions.est_vide() && est_operateur_binaire(lexeme.genre)) {
					lance_erreur("Opérateur binaire utilisé en début d'expression");
				}

				transfere_operateurs_mineures_dans_expression(lexeme.genre);
				dernier_noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, &lexeme);
				operateurs.pousse(dernier_noeud);

				break;
			}
			case GenreLexeme::TROIS_POINTS:
			{
				if (precede_unaire_valide(dernier_noeud)) {
					lexeme.genre = GenreLexeme::EXPANSION_VARIADIQUE;
					auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPANSION_VARIADIQUE, &lexeme);

					avance();

					if (donnees().genre != GenreLexeme::CHAINE_CARACTERE) {
						lance_erreur("Attendu une variable après '...'");
					}

					noeud->expr = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, &donnees());

					expressions.pousse(noeud);
					dernier_noeud = noeud;
				}
				else {
					transfere_operateurs_mineures_dans_expression(lexeme.genre);
					dernier_noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_PLAGE, &lexeme);
					operateurs.pousse(dernier_noeud);
				}

				break;
			}
			case GenreLexeme::POINT:
			{
				transfere_operateurs_mineures_dans_expression(lexeme.genre);
				dernier_noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_REFERENCE_MEMBRE, &lexeme);
				operateurs.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::EGAL:
			{
				if (assignation && racine_expr != GenreLexeme::FONC) {
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				transfere_operateurs_mineures_dans_expression(lexeme.genre);

				GenreNoeud tn;

				if ((drapeaux & DECLARATION) != 0) {
					tn = GenreNoeud::DECLARATION_VARIABLE;
					dernier_noeud = CREE_NOEUD(NoeudDeclarationVariable, tn, &lexeme);
					// À FAIRE : noeud->drapeaux_declaration |= DECLARATION_EST_CONSTANTE
				}
				else {
					tn = GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE;
					dernier_noeud = CREE_NOEUD(NoeudExpressionBinaire, tn, &lexeme);
				}

				operateurs.pousse(dernier_noeud);

				break;
			}
			case GenreLexeme::DECLARATION_VARIABLE:
			{
				if (assignation && racine_expr != GenreLexeme::FONC) {
					lance_erreur("Ne peut faire de déclaration dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				transfere_operateurs_mineures_dans_expression(lexeme.genre);

				dernier_noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, &lexeme);
				// À FAIRE : noeud->drapeaux_declaration |= DECLARATION_EST_DYNAMIQUE
				operateurs.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::DECLARATION_CONSTANTE:
			{
				if (assignation) {
					lance_erreur("Ne peut faire de déclaration dans une expression droite (const)", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				transfere_operateurs_mineures_dans_expression(lexeme.genre);

				dernier_noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, &lexeme);
				// À FAIRE : noeud->drapeaux_declaration |= DECLARATION_EST_CONSTANTE
				operateurs.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::CROCHET_OUVRANT:
			{
				/* l'accès à un élément d'un tableau est chaine[index] */
				if (!precede_unaire_valide(dernier_noeud)) {
					transfere_operateurs_mineures_dans_expression(lexeme.genre);

					auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, &lexeme);
					operateurs.pousse(noeud);

					auto noeud_expr = analyse_expression(GenreLexeme::CROCHET_FERMANT, GenreLexeme::CROCHET_OUVRANT);

					/* Si la racine de l'expression est un opérateur, il faut
					 * l'empêcher d'être prise en compte pour l'expression
					 * courante. */
					noeud_expr->drapeaux |= IGNORE_OPERATEUR;

					expressions.pousse(noeud_expr);
					dernier_noeud = noeud_expr;
				}
				else {
					/* change l'identifiant pour ne pas le confondre avec l'opérateur binaire [] */
					lexeme.genre = GenreLexeme::TABLEAU;
					auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU, &lexeme);

					noeud->expr = analyse_expression(GenreLexeme::CROCHET_FERMANT, GenreLexeme::CROCHET_OUVRANT);

					/* il est possible que le crochet n'ait pas été consommé,
					 * par exemple dans le cas où nous avons un point-virgule
					 * implicite dans la construction */
					if (est_identifiant(GenreLexeme::CROCHET_FERMANT)) {
						avance();
					}

					expressions.pousse(noeud);
					dernier_noeud = noeud;
				}

				break;
			}
			/* opérations unaire */
			case GenreLexeme::AROBASE:
			case GenreLexeme::EXCLAMATION:
			case GenreLexeme::TILDE:
			case GenreLexeme::PLUS_UNAIRE:
			case GenreLexeme::MOINS_UNAIRE:
			case GenreLexeme::EXPANSION_VARIADIQUE:
			{
				transfere_operateurs_mineures_dans_expression(lexeme.genre);
				dernier_noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::OPERATEUR_UNAIRE, &lexeme);
				operateurs.pousse(dernier_noeud);
				break;
			}
			case GenreLexeme::ACCOLADE_FERMANTE:
			{
				/* une accolade fermante marque généralement la fin de la
				 * construction d'une structure */
				termine_boucle = true;
				/* recule pour être synchroniser avec la sortie dans
				 * analyse_construction_structure() */
				recule();
				break;
			}
			case GenreLexeme::LOGE:
			{
				auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_LOGE, &lexeme);

				if (est_identifiant(GenreLexeme::CHAINE)) {
					noeud->type_declare = analyse_declaration_type(false);

					avance();

					noeud->expr_chaine = analyse_expression(type_id::SINON, type_id::LOGE);

					if (!requiers_identifiant(type_id::PARENTHESE_FERMANTE)) {
						lance_erreur("Attendu une paranthèse fermante ')");
					}
				}
				else {
					noeud->type_declare = analyse_declaration_type(false);
				}

				if (est_identifiant(type_id::SINON)) {
					avance();

					if (!requiers_identifiant(type_id::ACCOLADE_OUVRANTE)) {
						lance_erreur("Attendu une accolade ouvrante '{'");
					}

					noeud->bloc = analyse_bloc();

					termine_boucle = true;
				}

				expressions.pousse(noeud);
				dernier_noeud = noeud;
				break;
			}
			case GenreLexeme::RELOGE:
			{
				/* reloge nom : type; */
				auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_RELOGE, &lexeme);

				noeud->expr = analyse_expression(type_id::DOUBLE_POINTS, type_id::RELOGE);

				if (est_identifiant(type_id::CHAINE)) {
					noeud->type_declare = analyse_declaration_type(false);

					avance();

					noeud->expr_chaine = analyse_expression(type_id::SINON, type_id::RELOGE);

					if (!requiers_identifiant(type_id::PARENTHESE_FERMANTE)) {
						lance_erreur("Attendu une paranthèse fermante ')");
					}
				}
				else {
					noeud->type_declare = analyse_declaration_type(false);
				}

				if (est_identifiant(type_id::SINON)) {
					avance();

					if (!requiers_identifiant(type_id::ACCOLADE_OUVRANTE)) {
						lance_erreur("Attendu une accolade ouvrante '{'");
					}

					noeud->bloc = analyse_bloc();

					termine_boucle = true;
				}

				expressions.pousse(noeud);
				dernier_noeud = noeud;
				break;
			}
			case GenreLexeme::DELOGE:
			{
				auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_DELOGE, &lexeme);

				noeud->expr = analyse_expression(type_id::POINT_VIRGULE, type_id::DELOGE);

				/* besoin de reculer car l'analyse va jusqu'au point-virgule, ce
				 * qui nous fait absorber le code de l'expression suivante */
				recule();

				expressions.pousse(noeud);
				dernier_noeud = noeud;

				break;
			}
			case GenreLexeme::DIRECTIVE:
			{
				if (est_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
					avance();

					auto directive = donnees().chaine;

					if (directive == "inclus") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, donnees().chaine);
						m_contexte.assembleuse->ajoute_inclusion(chaine);
					}
					else if (directive == "bibliothèque_dynamique") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, donnees().chaine);
						m_contexte.assembleuse->bibliotheques_dynamiques.pousse(chaine);
					}
					else if (directive == "bibliothèque_statique") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, donnees().chaine);
						m_contexte.assembleuse->bibliotheques_statiques.pousse(chaine);
					}
					else if (directive == "def") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_contexte.assembleuse->definitions.pousse(chaine);
					}
					else if (directive == "exécute") {
						auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::DIRECTIVE_EXECUTION, &lexeme);

						noeud->expr = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::INCONNU);

						expressions.pousse(noeud);
						dernier_noeud = noeud;
						m_contexte.noeuds_a_executer.pousse(noeud);
					}
					else if (directive == "finsi") {
						// dépile la dernière directive si, erreur si aucune
					}
					else if (directive == "enligne") {
						m_etiquette_enligne = true;
					}
					else if (directive == "horsligne") {
						m_etiquette_horsligne = true;
					}
					else if (directive == "chemin") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_contexte.assembleuse->chemins.pousse(chaine);
					}
					else if (directive == "nulctx") {
						/* marque la  déclaration  d'une fonction comme ne
						 * requierant pas le contexte implicite */
						m_etiquette_nulctx = true;
					}
					else {
						lance_erreur("Directive inconnue");
					}
				}
				else if (est_identifiant(GenreLexeme::SI)) {
					analyse_directive_si();
				}
				else if (est_identifiant(GenreLexeme::SINON)) {
					avance();

					if (est_identifiant(GenreLexeme::SI)) {
						analyse_directive_si();
					}

					// ignore le code si la directive si parente a été consommée
				}
				else {
					lance_erreur("Directive inconnue");
				}

				termine_boucle = true;
				break;
			}
			case GenreLexeme::SI:
			{
				analyse_controle_si(GenreNoeud::INSTRUCTION_SI);
				termine_boucle = true;
				break;
			}
			case GenreLexeme::SAUFSI:
			{
				analyse_controle_si(GenreNoeud::INSTRUCTION_SAUFSI);
				termine_boucle = true;
				break;
			}
			case GenreLexeme::POINT_VIRGULE:
			{
				termine_boucle = true;
				break;
			}
			case GenreLexeme::TENTE:
			{
				auto inst = CREE_NOEUD(NoeudTente, GenreNoeud::INSTRUCTION_TENTE, &lexeme);

				inst->expr_appel = analyse_expression(GenreLexeme::PIEGE, GenreLexeme::TENTE);
				recule();

				if (est_identifiant(GenreLexeme::PIEGE)) {
					avance();

					if (est_identifiant(GenreLexeme::NONATTEIGNABLE)) {
						avance();
					}
					else {
						inst->expr_piege = analyse_expression(GenreLexeme::ACCOLADE_OUVRANTE, GenreLexeme::TENTE);
						recule();

						consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après l'expression de « piège »");

						inst->bloc = analyse_bloc();
					}
				}
				else {
					avance();
				}

				dernier_noeud = inst;
				expressions.pousse(dernier_noeud);

				termine_boucle = true;
				break;
			}
			case GenreLexeme::PIEGE:
			{
				recule();
				termine_boucle = true;
				break;
			}
			default:
			{
				lance_erreur("Identifiant inattendu dans l'expression");
			}
		}

		if (termine_boucle) {
			break;
		}
	}

	/* Retourne s'il n'y a rien dans l'expression, ceci est principalement pour
	 * éviter de crasher lors des fuzz-tests. */
	if (expressions.est_vide()) {
		--m_profondeur;
		return nullptr;
	}

	while (!operateurs.est_vide()) {
		expressions.pousse(operateurs.back());
		operateurs.pop_back();
	}

	operateurs.reserve(expressions.taille());

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Expression :" << FIN_LOG_EXPRESSION;

	for (auto noeud : expressions) {
		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(noeud->lexeme->genre) << FIN_LOG_EXPRESSION;

		if (noeud->genre == GenreNoeud::DECLARATION_VARIABLE) {
			if (operateurs.taille() < 2) {
				erreur::lance_erreur(
							"Expression malformée pour la déclaration",
							m_contexte,
							noeud->lexeme,
							erreur::type_erreur::NORMAL);
			}

			auto n2 = operateurs.back();
			operateurs.pop_back();

			auto n1 = operateurs.back();
			operateurs.pop_back();

			auto expr_bin = static_cast<NoeudDeclarationVariable *>(noeud);
			expr_bin->valeur = n1;
			expr_bin->expression = n2;

			// À FAIRE : cas où nous avons plusieurs variables
			expr_bin->ident = n1->ident;
		}
		else if (!dls::outils::possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_binaire(noeud->lexeme->genre)) {
			if (operateurs.taille() < 2) {
				erreur::lance_erreur(
							"Expression malformée pour opérateur binaire",
							m_contexte,
							noeud->lexeme,
							erreur::type_erreur::NORMAL);
			}

			auto n2 = operateurs.back();
			operateurs.pop_back();

			auto n1 = operateurs.back();
			operateurs.pop_back();

			auto expr_bin = static_cast<NoeudExpressionBinaire *>(noeud);
			expr_bin->expr1 = n1;
			expr_bin->expr2 = n2;
		}
		else if (!dls::outils::possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_unaire(noeud->lexeme->genre)) {
			auto n1 = operateurs.back();
			operateurs.pop_back();

			auto expr_un = static_cast<NoeudExpressionUnaire *>(noeud);
			expr_un->expr = n1;
		}

		operateurs.pousse(noeud);
	}

	auto noeud_expr = operateurs.back();

	operateurs.pop_back();

	if (operateurs.taille() != 0) {
		auto premier_noeud = operateurs.back();
		dernier_noeud = premier_noeud;
		operateurs.pop_back();

		auto pos_premier = premier_noeud->lexeme->chaine.pointeur();
		auto pos_dernier = pos_premier;

		while (!operateurs.est_vide()) {
			auto n = operateurs.back();
			operateurs.pop_back();

			auto pos_n = n->lexeme->chaine.pointeur();

			if (pos_n < pos_premier) {
				premier_noeud = n;
			}
			if (pos_n > pos_dernier) {
				dernier_noeud = n;
			}
		}

		erreur::lance_erreur_plage(
					"Expression malformée, il est possible qu'il manque un opérateur",
					m_contexte,
					premier_noeud->lexeme,
					dernier_noeud->lexeme);
	}

	--m_profondeur;

	return noeud_expr;
}

NoeudExpression *Syntaxeuse::analyse_appel_fonction(DonneesLexeme &lexeme)
{
	auto noeud = CREE_NOEUD(NoeudExpressionAppel, GenreNoeud::EXPRESSION_APPEL_FONCTION, &lexeme);

	/* ici nous devons être au niveau du premier paramètre */
	while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
		/* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
		 * si identifiant final == ')', alors l'algorithme s'arrête quand une
		 * paranthèse fermante est trouvé et que la pile est vide */
		auto expr = analyse_expression(GenreLexeme::VIRGULE, GenreLexeme::EGAL);
		noeud->params.pousse(expr);
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attenu ')' à la fin des argument de l'appel");

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_structure(GenreLexeme id, DonneesLexeme &lexeme)
{
	auto noeud_decl = CREE_NOEUD(NoeudStruct, GenreNoeud::DECLARATION_STRUCTURE, &lexeme);
	noeud_decl->est_union = (id == GenreLexeme::UNION);

	if (noeud_decl->est_union) {
		noeud_decl->type = m_contexte.typeuse.reserve_type_union(noeud_decl);
	}
	else {
		noeud_decl->type = m_contexte.typeuse.reserve_type_structure(noeud_decl);
	}

	if (lexeme.chaine == "ContexteProgramme") {
		m_contexte.type_contexte = noeud_decl->type;
	}

	if (est_identifiant(type_id::EXTERNE)) {
		noeud_decl->est_externe = true;
		avance();
	}

	if (est_identifiant(type_id::NONSUR)) {
		noeud_decl->est_nonsure = true;
		avance();
	}

	auto analyse_membres = true;

	if (noeud_decl->est_externe) {
		if (est_identifiant(type_id::POINT_VIRGULE)) {
			avance();
			analyse_membres = false;
		}
	}

	if (analyse_membres) {
		auto bloc = m_fichier->module->assembleuse->empile_bloc();
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

		while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
			auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, type_id::STRUCT);

			if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
				decl_var->valeur = noeud;

				decl_var->bloc_parent->membres.pousse(decl_var);
				decl_var->bloc_parent->expressions.pousse(decl_var);
			}
			else {
				bloc->membres.pousse(static_cast<NoeudDeclaration *>(noeud));
				bloc->expressions.pousse(noeud);
			}
		}

		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de la structure");

		m_fichier->module->assembleuse->depile_bloc();
		noeud_decl->bloc = bloc;
	}

	return noeud_decl;
}

NoeudExpression *Syntaxeuse::analyse_declaration_enum(GenreLexeme genre, DonneesLexeme &lexeme)
{
	auto noeud_decl = CREE_NOEUD(NoeudEnum, GenreNoeud::DECLARATION_ENUM, &lexeme);

	if (genre != GenreLexeme::ERREUR) {
		noeud_decl->desc.est_drapeau = genre == GenreLexeme::ENUM_DRAPEAU;

		if (!est_identifiant(GenreLexeme::ACCOLADE_OUVRANTE)) {
			noeud_decl->type_declare = analyse_declaration_type(false);
		}

		noeud_decl->type = m_contexte.typeuse.reserve_type_enum(noeud_decl);
	}
	else {
		noeud_decl->desc.est_erreur = true;
		noeud_decl->type = m_contexte.typeuse.reserve_type_erreur(noeud_decl);
	}

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

	auto bloc = m_fichier->module->assembleuse->empile_bloc();

	while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
		auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::EGAL);

		if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
			decl_var->valeur = noeud;

			decl_var->bloc_parent->membres.pousse(decl_var);
			decl_var->bloc_parent->expressions.pousse(decl_var);
		}
		else {
			bloc->membres.pousse(static_cast<NoeudDeclaration *>(noeud));
			bloc->expressions.pousse(noeud);
		}
	}

	m_fichier->module->assembleuse->depile_bloc();
	noeud_decl->bloc = bloc;

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

	return noeud_decl;
}

DonneesTypeDeclare Syntaxeuse::analyse_declaration_type(bool double_point)
{
	if (double_point && !requiers_identifiant(GenreLexeme::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':'");
	}

	auto nulctx = false;

	if (est_identifiant(GenreLexeme::DIRECTIVE)) {
		avance();
		avance();

		auto nom_directive = donnees().chaine;

		if (nom_directive != "nulctx") {
			lance_erreur("Directive invalide pour le type");
		}

		nulctx = true;
	}

	/* Vérifie si l'on a un pointeur vers une fonction. */
	if (est_identifiant(GenreLexeme::FONC) || est_identifiant(GenreLexeme::COROUT)) {
		avance();

		auto dt = DonneesTypeDeclare(donnees().genre);

		consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu un '(' après 'fonction'");

		if (!nulctx) {
			auto dt_ctx = DonneesTypeDeclare(GenreLexeme::CHAINE_CARACTERE);
			dt_ctx.nom_struct = "ContexteProgramme";
			dt.types_entrees.pousse(dt_ctx);
		}

		while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
			auto dtd = analyse_declaration_type(false);
			dt.types_entrees.pousse(dtd);

			if (!est_identifiant(GenreLexeme::VIRGULE)) {
				break;
			}

			avance();
		}

		avance();

		bool eu_paren_ouvrante = false;

		if (est_identifiant(GenreLexeme::PARENTHESE_OUVRANTE)) {
			avance();
			eu_paren_ouvrante = true;
		}

		while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
			auto dtd = analyse_declaration_type(false);
			dt.types_sorties.pousse(dtd);

			auto est_virgule = est_identifiant(GenreLexeme::VIRGULE);

			if ((est_virgule && !eu_paren_ouvrante) || !est_virgule) {
				break;
			}

			avance();
		}

		if (eu_paren_ouvrante && est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
			avance();
		}

		return dt;
	}

	return analyse_declaration_type_ex();
}

DonneesTypeDeclare Syntaxeuse::analyse_declaration_type_ex()
{
	auto dernier_id = GenreLexeme{};
	auto donnees_type = DonneesTypeDeclare{};

	while (est_specifiant_type(identifiant_courant())) {
		auto id = this->identifiant_courant();
		avance();

		switch (id) {
			case type_id::CROCHET_OUVRANT:
			{
				auto expr = static_cast<NoeudBase *>(nullptr);

				if (this->identifiant_courant() != GenreLexeme::CROCHET_FERMANT) {
					expr = analyse_expression(GenreLexeme::CROCHET_FERMANT, GenreLexeme::CROCHET_OUVRANT);
				}
				else {
					avance();
				}

				donnees_type.pousse(GenreLexeme::TABLEAU);
				donnees_type.expressions.pousse(static_cast<NoeudExpression *>(expr));

				break;
			}
			case type_id::TROIS_POINTS:
			{
				donnees_type.pousse(id);
				break;
			}
			case type_id::FOIS:
			{
				donnees_type.pousse(GenreLexeme::POINTEUR);
				break;
			}
			case type_id::ESPERLUETTE:
			{
				donnees_type.pousse(GenreLexeme::REFERENCE);
				break;
			}
			case type_id::TYPE_DE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu un '(' après 'type_de'");

				auto expr = analyse_expression(
							GenreLexeme::PARENTHESE_FERMANTE,
							GenreLexeme::TYPE_DE);

				donnees_type.pousse(GenreLexeme::TYPE_DE);
				donnees_type.expressions.pousse(expr);

				break;
			}
			case type_id::DOLLAR:
			{
				donnees_type.pousse(id);

				if (!requiers_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
					lance_erreur("Attendu une chaine de caractère après '$'");
				}

				donnees_type.nom_gabarit = donnees().chaine;
				donnees_type.est_gabarit = true;
				break;
			}
			default:
			{
				break;
			}
		}

		dernier_id = id;
	}

	auto type_attendu = true;

	if (dernier_id == GenreLexeme::TYPE_DE || dernier_id == GenreLexeme::DOLLAR) {
		type_attendu = false;
	}

	/* Soutiens pour les types des fonctions variadiques externes. */
	if (dernier_id == GenreLexeme::TROIS_POINTS && est_identifiant(type_id::PARENTHESE_FERMANTE)) {
		type_attendu = false;
	}

	if (type_attendu) {
		consomme_type("Attendu la déclaration d'un type");

		auto identifiant = donnees().genre;

		if (identifiant == GenreLexeme::CHAINE_CARACTERE) {
			donnees_type.nom_struct = donnees().chaine;
		}

		donnees_type.pousse(identifiant);
	}

	return donnees_type;
}

NoeudExpression *Syntaxeuse::analyse_construction_structure(DonneesLexeme &lexeme)
{
	auto noeud = CREE_NOEUD(NoeudExpressionAppel, GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE, &lexeme);

	/* ici nous devons être au niveau du premier paramètre */
	while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
		/* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
		 * si identifiant final == ')', alors l'algorithme s'arrête quand une
		 * paranthèse fermante est trouvé et que la pile est vide */
		auto expr = analyse_expression(GenreLexeme::VIRGULE, GenreLexeme::EGAL);
		noeud->params.pousse(expr);
	}

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la construction de la structure");

	return noeud;
}

void Syntaxeuse::analyse_directive_si()
{
	avance();
	avance();
	lance_erreur("Directive inconnue");
}

static bool est_operateur_surchargeable(GenreLexeme genre)
{
	switch (genre) {
		default:
		{
			return false;
		}
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::POURCENT:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::BARRE:
		case GenreLexeme::TILDE:
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::CHAPEAU:
		{
			return true;
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_declaration_operateur()
{
	// nous sommes au niveau du lexème de l'opérateur
	auto id = this->identifiant_courant();

	if (!est_operateur_surchargeable(id)) {
		lance_erreur("L'opérateur n'est pas surchargeable");
	}

	avance();

	auto &lexeme = this->donnees();

	// :: fonc
	consomme(GenreLexeme::DECLARATION_CONSTANTE, "Attendu :: après la déclaration de l'opérateur");
	consomme(GenreLexeme::FONC, "Attendu fonc après ::");

	auto noeud = CREE_NOEUD(NoeudDeclarationFonction, GenreNoeud::DECLARATION_OPERATEUR, &lexeme);

	if (m_etiquette_enligne) {
		noeud->drapeaux |= FORCE_ENLIGNE;
		m_etiquette_enligne = false;
	}
	else if (m_etiquette_horsligne) {
		noeud->drapeaux |= FORCE_HORSLIGNE;
		m_etiquette_horsligne = false;
	}

	if (m_etiquette_nulctx) {
		noeud->drapeaux |= FORCE_NULCTX;
		m_etiquette_nulctx = false;
	}

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	/* analyse les paramètres de la fonction */
	while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression(GenreLexeme::VIRGULE, type_id::FONC);

		if (param->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
			decl_var->valeur = param;

			noeud->params.pousse(decl_var);
		}
		else {
			noeud->params.pousse(static_cast<NoeudDeclaration *>(param));
		}
	}

	if (noeud->params.taille > 2) {
		erreur::lance_erreur(
					"La surcharge d'opérateur ne peut prendre au plus 2 paramètres",
					m_contexte,
					&lexeme);
	}
	else if (noeud->params.taille == 1) {
		if (id == GenreLexeme::PLUS) {
			lexeme.genre = GenreLexeme::PLUS_UNAIRE;
		}
		else if (id == GenreLexeme::MOINS) {
			lexeme.genre = GenreLexeme::MOINS_UNAIRE;
		}
		else if (id != GenreLexeme::TILDE && id != GenreLexeme::EXCLAMATION) {
			erreur::lance_erreur(
						"La surcharge d'opérateur unaire n'est possible que pour '+', '-', '~', ou '!'",
						m_contexte,
						&lexeme);
		}
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction, À FAIRE : inférence */

	consomme(GenreLexeme::RETOUR_TYPE, "Attendu un retour de type");

	while (true) {
		noeud->noms_retours.pousse("__ret" + dls::vers_chaine(noeud->noms_retours.taille()));

		auto type_declare = analyse_declaration_type(false);
		noeud->type_declare.types_sorties.pousse(type_declare);

		if (est_identifiant(type_id::ACCOLADE_OUVRANTE) || est_identifiant(type_id::POINT_VIRGULE)) {
			break;
		}

		if (est_identifiant(type_id::VIRGULE)) {
			avance();
		}
	}

	if (noeud->type_declare.taille() > 1) {
		lance_erreur("Il est impossible d'avoir plusieurs de sortie pour un opérateur");
	}

	/* ignore les points-virgules implicites */
	if (est_identifiant(GenreLexeme::POINT_VIRGULE)) {
		avance();
	}

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après la liste des paramètres de la fonction");

	noeud->bloc = analyse_bloc();
	aplatis_arbre(noeud->bloc, noeud->arbre_aplatis);

	return noeud;
}

void Syntaxeuse::consomme(GenreLexeme id, const char *message)
{
	if (!requiers_identifiant(id)) {
		lance_erreur(message);
	}
}

void Syntaxeuse::consomme_type(const char *message)
{
	auto const ok = est_identifiant_type(this->identifiant_courant());
	avance();

	if (!ok) {
		lance_erreur(message);
	}
}

void Syntaxeuse::lance_erreur(const dls::chaine &quoi, erreur::type_erreur type)
{
	erreur::lance_erreur(quoi, m_contexte, &donnees(), type);
}
