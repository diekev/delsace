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

#include "biblinternes/langage/debogage.hh"
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "expression.h"
#include "outils_lexemes.hh"

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

/**
 * Retourne vrai si l'identifiant passé en paramètre peut-être un identifiant
 * valide pour précèder un opérateur unaire '+' ou '-'.
 */
static bool precede_unaire_valide(GenreLexeme dernier_identifiant)
{
	if (dernier_identifiant == GenreLexeme::PARENTHESE_FERMANTE) {
		return false;
	}

	if (dernier_identifiant == GenreLexeme::CROCHET_FERMANT) {
		return false;
	}

	if (dernier_identifiant == GenreLexeme::CHAINE_CARACTERE) {
		return false;
	}

	if (est_nombre(dernier_identifiant)) {
		return false;
	}

	/* À FAIRE : ceci mélange a[-i] et a[i] - b[i] */
	if (dernier_identifiant == GenreLexeme::CROCHET_OUVRANT) {
		return false;
	}

	if (dernier_identifiant == GenreLexeme::CARACTERE) {
		return false;
	}

	if (dernier_identifiant == GenreLexeme::TRANSTYPE) {
		return false;
	}

	if (dernier_identifiant == GenreLexeme::MEMOIRE) {
		return false;
	}

	return true;
}

/* ************************************************************************** */

Syntaxeuse::Syntaxeuse(
		ContexteGenerationCode &contexte,
		Fichier *fichier,
		dls::chaine const &racine_kuri)
	: lng::analyseuse<DonneesLexeme>(fichier->lexemes)
	, m_contexte(contexte)
	, m_assembleuse(contexte.assembleuse)
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
	analyse_corps(os);
	m_fichier->temps_analyse += m_chrono_analyse.arrete();
}

void Syntaxeuse::analyse_corps(std::ostream &os)
{
	while (!fini()) {
		auto id = this->identifiant_courant();

		switch (id) {
			case GenreLexeme::CHAINE_CARACTERE:
			{
				avance();

				auto &lexeme = donnees();

				if (this->identifiant_courant() == GenreLexeme::DECLARATION_CONSTANTE) {
					avance();

					id = this->identifiant_courant();

					switch (id) {
						case GenreLexeme::FONC:
						case GenreLexeme::COROUT:
						{
							avance();
							analyse_declaration_fonction(id, lexeme);
							break;
						}
						case GenreLexeme::STRUCT:
						case GenreLexeme::UNION:
						{
							avance();
							analyse_declaration_structure(id, lexeme);
							break;
						}
						case GenreLexeme::ENUM:
						{
							avance();
							analyse_declaration_enum(false, lexeme);
							break;
						}
						case GenreLexeme::ENUM_DRAPEAU:
						{
							avance();
							analyse_declaration_enum(true, lexeme);
							break;
						}
						default:
						{
							recule();
							recule();

							m_global = true;
							auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::INCONNU);

							if (noeud && noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
								noeud->genre = GenreNoeud::DECLARATION_VARIABLE;
							}
							m_global = false;

							break;
						}
					}
				}
				else {
					recule();
					m_global = true;
					auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::INCONNU);

					if (noeud && noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
						noeud->genre = GenreNoeud::DECLARATION_VARIABLE;
					}
					m_global = false;
				}

				break;
			}
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
				// dans le cas des expressions commençant par « dyn »
				m_global = true;
				auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::INCONNU);

				if (noeud && noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					noeud->genre = GenreNoeud::DECLARATION_VARIABLE;
				}
				m_global = false;
				break;
			}
		}
	}
}

void Syntaxeuse::analyse_declaration_fonction(GenreLexeme id, DonneesLexeme &lexeme)
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

	auto noeud = m_assembleuse->empile_noeud(GenreNoeud::DECLARATION_FONCTION, lexeme);

	if (externe) {
		noeud->drapeaux |= EST_EXTERNE;
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

	auto donnees_fonctions = DonneesFonction{};
	donnees_fonctions.est_coroutine = (id == GenreLexeme::COROUT);
	donnees_fonctions.est_externe = externe;
	donnees_fonctions.noeud_decl = noeud;

	/* analyse les paramètres de la fonction */
	m_assembleuse->empile_noeud(GenreNoeud::DECLARATION_PARAMETRES_FONCTION, donnees());

	analyse_expression(GenreLexeme::PARENTHESE_FERMANTE, GenreLexeme::PARENTHESE_OUVRANTE);

	m_assembleuse->depile_noeud(GenreNoeud::DECLARATION_PARAMETRES_FONCTION);

	/* analyse les types de retour de la fonction, À FAIRE : inférence */

	avance();

	auto idx_ret = 0;

	while (true) {
		auto type_declare = analyse_declaration_type(false);
		donnees_fonctions.types_retours_decl.pousse(type_declare);
		donnees_fonctions.noms_retours.pousse("__ret" + dls::vers_chaine(idx_ret++));

		if (est_identifiant(type_id::ACCOLADE_OUVRANTE) || est_identifiant(type_id::POINT_VIRGULE)) {
			break;
		}

		if (est_identifiant(type_id::VIRGULE)) {
			avance();
		}
	}

	noeud->type_declare = donnees_fonctions.types_retours_decl[0];

	m_fichier->module->ajoute_donnees_fonctions(lexeme.chaine, donnees_fonctions);

	if (externe) {
		consomme(GenreLexeme::POINT_VIRGULE, "Attendu un point-virgule ';' après la déclaration de la fonction externe");

		if (donnees_fonctions.idx_types_retours.taille() > 1) {
			lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
		}
	}
	else {
		/* ignore les points-virgules implicites */
		if (est_identifiant(GenreLexeme::POINT_VIRGULE)) {
			avance();
		}

		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après la liste des paramètres de la fonction");

		analyse_bloc();
	}

	m_assembleuse->depile_noeud(GenreNoeud::DECLARATION_FONCTION);
}

void Syntaxeuse::analyse_controle_si(GenreNoeud tn)
{
	m_assembleuse->empile_noeud(tn, donnees());

	analyse_expression(GenreLexeme::ACCOLADE_OUVRANTE, GenreLexeme::SI);

	analyse_bloc();

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
		m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, donnees());

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

		m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_COMPOSEE);
	}

	m_assembleuse->depile_noeud(tn);
}

/* Arbre :
 * NoeudPour
 * - enfant 1 : déclaration variable
 * - enfant 2 : expr
 * - enfant 3 : bloc
 * - enfant 4 : bloc sansarrêt ou sinon
 * - enfant 5 : bloc sinon
 */
void Syntaxeuse::analyse_controle_pour()
{
	m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_POUR, donnees());

	/* enfant 1 : déclaration variable */

	analyse_expression(GenreLexeme::DANS, GenreLexeme::POUR);

	/* enfant 2 : expr */

	analyse_expression(GenreLexeme::ACCOLADE_OUVRANTE, GenreLexeme::DANS);

	recule();

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'pour'");

	/* enfant 3 : bloc */
	analyse_bloc();

	/* enfant 4 : bloc sansarrêt (optionel) */
	if (est_identifiant(GenreLexeme::SANSARRET)) {
		avance();

		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'sansarrêt'");

		analyse_bloc();
	}

	/* enfant 4 ou 5 : bloc sinon (optionel) */
	if (est_identifiant(GenreLexeme::SINON)) {
		avance();

		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");

		analyse_bloc();
	}

	m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_POUR);
}

void Syntaxeuse::analyse_corps_fonction()
{
	/* Il est possible qu'une fonction soit vide, donc vérifie d'abord que
	 * l'on n'ait pas terminé. */
	while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
		auto const pos = position();

		if (est_identifiant(GenreLexeme::RETOURNE)) {
			avance();
			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_RETOUR, donnees());

			/* Considération du cas où l'on ne retourne rien 'retourne;'. */
			if (!est_identifiant(GenreLexeme::POINT_VIRGULE)) {
				analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::RETOURNE);
			}
			else {
				avance();
			}

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_RETOUR);
		}
		else if (est_identifiant(GenreLexeme::RETIENS)) {
			avance();
			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_RETIENS, donnees());

			analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::RETOURNE);

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_RETIENS);
		}
		else if (est_identifiant(GenreLexeme::POUR)) {
			avance();
			analyse_controle_pour();
		}
		else if (est_identifiant(GenreLexeme::BOUCLE)) {
			avance();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'boucle'");

			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_BOUCLE, donnees());

			analyse_bloc();

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_BOUCLE);
		}
		else if (est_identifiant(GenreLexeme::REPETE)) {
			avance();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'répète'");

			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_REPETE, donnees());

			analyse_bloc();

			consomme(GenreLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

			analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::TANTQUE);

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_REPETE);
		}
		else if (est_identifiant(GenreLexeme::TANTQUE)) {
			avance();
			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_TANTQUE, donnees());

			analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::TANTQUE);

			/* recule pour être de nouveau synchronisé */
			recule();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de 'tanque'");

			analyse_bloc();

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_TANTQUE);
		}
		else if (est_identifiant(GenreLexeme::ARRETE) || est_identifiant(GenreLexeme::CONTINUE)) {
			avance();

			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, donnees());

			if (est_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
				avance();
				m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, donnees());
				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION);
			}

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE);

			consomme(GenreLexeme::POINT_VIRGULE, "Attendu un point virgule ';'");
		}
		else if (est_identifiant(GenreLexeme::DIFFERE)) {
			avance();

			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_DIFFERE, donnees());

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'diffère'");

			analyse_bloc();

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_DIFFERE);
		}
		else if (est_identifiant(GenreLexeme::NONSUR)) {
			avance();

			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_NONSUR, donnees());

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'nonsûr'");

			analyse_bloc();

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_NONSUR);
		}
		else if (est_identifiant(GenreLexeme::DISCR)) {
			avance();

			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_DISCR, donnees());

			analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::DISCR);

			/* recule pour être de nouveau synchronisé */
			recule();

			consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

			auto sinon_rencontre = false;

			while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
				m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_PAIRE_DISCR, donnees());

				if (est_identifiant(GenreLexeme::SINON)) {
					avance();

					if (sinon_rencontre) {
						lance_erreur("Redéfinition d'un bloc sinon");
					}

					auto noeud = m_assembleuse->cree_noeud(GenreNoeud::INSTRUCTION_SINON, donnees());
					m_assembleuse->ajoute_noeud(noeud);

					sinon_rencontre = true;
				}
				else {
					analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::DISCR);

					/* recule pour être de nouveau synchronisé */
					recule();
				}

				consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

				analyse_bloc();

				m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_PAIRE_DISCR);
			}

			consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_DISCR);
		}
		else if (est_identifiant(type_id::ACCOLADE_OUVRANTE)) {
			avance();
			analyse_bloc();
		}
		else if (est_identifiant(GenreLexeme::POUSSE_CONTEXTE)) {
			avance();
			m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE, donnees());

			analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::POUSSE_CONTEXTE);
			analyse_bloc();

			m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE);
		}
		else {
			auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::EGAL);

			if (noeud && noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				noeud->genre = GenreNoeud::DECLARATION_VARIABLE;
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

void Syntaxeuse::analyse_bloc()
{
	m_assembleuse->empile_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, donnees());
	analyse_corps_fonction();
	m_assembleuse->depile_noeud(GenreNoeud::INSTRUCTION_COMPOSEE);

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc");
}

noeud::base *Syntaxeuse::analyse_expression(
		GenreLexeme identifiant_final,
		GenreLexeme racine_expr,
		bool ajoute_noeud)
{
	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */
	auto profondeur = m_profondeur++;

	if (profondeur >= m_paires_vecteurs.taille()) {
		lance_erreur("Excès de la pile d'expression autorisée");
	}

	auto &expression = m_paires_vecteurs[profondeur].first;
	expression.efface();

	auto &pile = m_paires_vecteurs[profondeur].second;
	pile.efface();

	auto vide_pile_operateur = [&](GenreLexeme id_operateur)
	{
		while (!pile.est_vide()
			   && (precedence_faible(id_operateur, pile.back()->identifiant())))
		{
			expression.pousse(pile.back());
			pile.pop_back();
		}
	};

	auto genre_dernier_lexeme = (m_position == 0) ? GenreLexeme::INCONNU : donnees().genre;

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	auto assignation = false;

	auto drapeaux = drapeaux_noeud::AUCUN;

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

	while (!requiers_identifiant(identifiant_final)) {
		auto &lexeme = donnees();

		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(lexeme.genre) << FIN_LOG_EXPRESSION;

		auto genre_courant = lexeme.genre;

		switch (genre_courant) {
			case GenreLexeme::DYN:
			{
				drapeaux |= (DYNAMIC | DECLARATION);
				break;
			}
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
				/* appel fonction : chaine + ( */
				if (est_identifiant(GenreLexeme::PARENTHESE_OUVRANTE)) {
					avance();

					auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_APPEL_FONCTION, lexeme, false);

					analyse_appel_fonction(noeud);

					m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_APPEL_FONCTION);

					expression.pousse(noeud);
				}
				/* construction structure : chaine + { */
				else if (dls::outils::est_element(racine_expr, GenreLexeme::EGAL, GenreLexeme::RETOURNE, GenreLexeme::CROCHET_OUVRANT)
						 && est_identifiant(GenreLexeme::ACCOLADE_OUVRANTE))
				{
					auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE, lexeme, false);

					avance();

					analyse_construction_structure(noeud);

					m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE);

					expression.pousse(noeud);

					// À FAIRE, XXX - désynchronisation dans l'analyse de la construction de structure ?
					if (racine_expr != GenreLexeme::CROCHET_OUVRANT) {
						termine_boucle = true;
					}
				}
				/* variable : chaine */
				else {
					auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme);
					expression.pousse(noeud);

					noeud->drapeaux |= drapeaux;
					drapeaux = drapeaux_noeud::AUCUN;

					/* nous avons la déclaration d'un type dans la structure */
					if ((racine_expr != type_id::TRANSTYPE && racine_expr != type_id::LOGE && racine_expr != type_id::RELOGE) && est_identifiant(GenreLexeme::DOUBLE_POINTS)) {
						noeud->type_declare = analyse_declaration_type();
						drapeaux |= DECLARATION;
					}
				}

				break;
			}
			case GenreLexeme::NOMBRE_REEL:
			{
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL, lexeme);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::NOMBRE_BINAIRE:
			case GenreLexeme::NOMBRE_ENTIER:
			case GenreLexeme::NOMBRE_HEXADECIMAL:
			case GenreLexeme::NOMBRE_OCTAL:
			{
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER, lexeme);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::CHAINE_LITTERALE:
			{
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_CHAINE, lexeme);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::CARACTERE:
			{
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_CARACTERE, lexeme);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::NUL:
			{
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_NUL, lexeme);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::TAILLE_DE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_TAILLE_DE, lexeme);
				noeud->valeur_calculee = analyse_declaration_type(false);

				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::INFO_DE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

				auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_INFO_DE, lexeme, false);

				analyse_expression(GenreLexeme::INCONNU, GenreLexeme::INFO_DE);

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_INFO_DE);

				/* vérifie mais n'avance pas */
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'taille_de'");

				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::VRAI:
			case GenreLexeme::FAUX:
			{
				/* remplace l'identifiant par id_lexeme::BOOL */
				lexeme.genre = GenreLexeme::BOOL;
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN, lexeme);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::TRANSTYPE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'transtype'");

				auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_TRANSTYPE, lexeme, false);

				analyse_expression(GenreLexeme::DOUBLE_POINTS, GenreLexeme::TRANSTYPE);

				noeud->type_declare = analyse_declaration_type(false);

				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après la déclaration du type");

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_TRANSTYPE);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::MEMOIRE:
			{
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

				auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_MEMOIRE, lexeme, false);

				analyse_expression(GenreLexeme::INCONNU, GenreLexeme::MEMOIRE);

				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_MEMOIRE);
				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::PARENTHESE_OUVRANTE:
			{
				auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_PARENTHESE, lexeme, false);

				analyse_expression(GenreLexeme::PARENTHESE_FERMANTE, GenreLexeme::PARENTHESE_OUVRANTE);

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_PARENTHESE);

				expression.pousse(noeud);

				/* ajourne id_courant avec une parenthèse fermante, car étant
				 * une parenthèse ouvrante, il ferait échouer le test de
				 * détermination d'un opérateur unaire */
				genre_courant = GenreLexeme::PARENTHESE_FERMANTE;

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
				auto noeud = static_cast<noeud::base *>(nullptr);

				if (precede_unaire_valide(genre_dernier_lexeme)) {
					if (id_operateur == GenreLexeme::PLUS) {
						id_operateur = GenreLexeme::PLUS_UNAIRE;
					}
					else if (id_operateur == GenreLexeme::MOINS) {
						id_operateur = GenreLexeme::MOINS_UNAIRE;
					}

					lexeme.genre = id_operateur;
					noeud = m_assembleuse->cree_noeud(GenreNoeud::OPERATEUR_UNAIRE, lexeme);
				}
				else {
					noeud = m_assembleuse->cree_noeud(GenreNoeud::OPERATEUR_BINAIRE, lexeme);
				}

				vide_pile_operateur(id_operateur);

				pile.pousse(noeud);

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
				if (expression.est_vide() && est_operateur_binaire(lexeme.genre)) {
					lance_erreur("Opérateur binaire utilisé en début d'expression");
				}

				vide_pile_operateur(lexeme.genre);
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::OPERATEUR_BINAIRE, lexeme);
				pile.pousse(noeud);

				break;
			}
			case GenreLexeme::TROIS_POINTS:
			{
				if (precede_unaire_valide(genre_dernier_lexeme)) {
					lexeme.genre = GenreLexeme::EXPANSION_VARIADIQUE;
					auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPANSION_VARIADIQUE, lexeme);

					avance();

					if (donnees().genre != GenreLexeme::CHAINE_CARACTERE) {
						lance_erreur("Attendu une variable après '...'");
					}

					auto noeud_var = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, donnees());
					noeud->enfants.pousse(noeud_var);

					expression.pousse(noeud);
				}
				else {
					vide_pile_operateur(lexeme.genre);
					auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_PLAGE, lexeme);
					pile.pousse(noeud);
				}

				break;
			}
			case GenreLexeme::POINT:
			{
				vide_pile_operateur(lexeme.genre);
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE, lexeme);
				pile.pousse(noeud);
				break;
			}
			case GenreLexeme::EGAL:
			{
				if (assignation) {
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				vide_pile_operateur(lexeme.genre);

				GenreNoeud tn;

				if ((drapeaux & DECLARATION) != 0) {
					tn = GenreNoeud::DECLARATION_VARIABLE;
				}
				else {
					tn = GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE;
				}

				auto noeud = m_assembleuse->cree_noeud(tn, lexeme);
				pile.pousse(noeud);
				break;
			}
			case GenreLexeme::DECLARATION_VARIABLE:
			{
				if (assignation) {
					lance_erreur("Ne peut faire de déclaration dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				vide_pile_operateur(lexeme.genre);

				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, lexeme);
				pile.pousse(noeud);
				break;
			}
			case GenreLexeme::CROCHET_OUVRANT:
			{
				/* l'accès à un élément d'un tableau est chaine[index] */
				if (genre_dernier_lexeme == GenreLexeme::CHAINE_CARACTERE
						|| genre_dernier_lexeme == GenreLexeme::CHAINE_LITTERALE
						 || genre_dernier_lexeme == GenreLexeme::CROCHET_OUVRANT) {
					vide_pile_operateur(lexeme.genre);

					auto noeud = m_assembleuse->empile_noeud(GenreNoeud::OPERATEUR_BINAIRE, lexeme, false);
					pile.pousse(noeud);

					analyse_expression(GenreLexeme::CROCHET_FERMANT, GenreLexeme::CROCHET_OUVRANT);

					/* Extrait le noeud enfant, il sera de nouveau ajouté dans
					 * la compilation de l'expression à la fin de la fonction. */
					auto noeud_expr = noeud->enfants.front();
					noeud->enfants.efface();

					/* Si la racine de l'expression est un opérateur, il faut
					 * l'empêcher d'être prise en compte pour l'expression
					 * courante. */
					noeud_expr->drapeaux |= IGNORE_OPERATEUR;

					expression.pousse(noeud_expr);

					m_assembleuse->depile_noeud(GenreNoeud::OPERATEUR_BINAIRE);
				}
				else {
					/* change l'identifiant pour ne pas le confondre avec l'opérateur binaire [] */
					lexeme.genre = GenreLexeme::TABLEAU;
					auto noeud = m_assembleuse->empile_noeud(GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU, lexeme, false);

					analyse_expression(GenreLexeme::CROCHET_FERMANT, GenreLexeme::CROCHET_OUVRANT);

					/* il est possible que le crochet n'ait pas été consommé,
					 * par exemple dans le cas où nous avons un point-virgule
					 * implicite dans la construction */
					if (est_identifiant(GenreLexeme::CROCHET_FERMANT)) {
						avance();
					}

					m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU);

					expression.pousse(noeud);
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
				vide_pile_operateur(lexeme.genre);
				auto noeud = m_assembleuse->cree_noeud(GenreNoeud::OPERATEUR_UNAIRE, lexeme);
				pile.pousse(noeud);
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
				auto noeud = m_assembleuse->empile_noeud(
							GenreNoeud::EXPRESSION_LOGE,
							lexeme,
							false);

				if (est_identifiant(GenreLexeme::CHAINE)) {
					noeud->type_declare = analyse_declaration_type(false);

					avance();

					analyse_expression(type_id::SINON, type_id::LOGE);

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

					analyse_bloc();

					termine_boucle = true;
				}

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_LOGE);

				expression.pousse(noeud);
				break;
			}
			case GenreLexeme::RELOGE:
			{
				/* reloge nom : type; */
				auto noeud_reloge = m_assembleuse->empile_noeud(
							GenreNoeud::EXPRESSION_RELOGE,
							lexeme,
							false);

				analyse_expression(type_id::DOUBLE_POINTS, type_id::RELOGE);

				if (est_identifiant(type_id::CHAINE)) {
					noeud_reloge->type_declare = analyse_declaration_type(false);

					avance();

					analyse_expression(type_id::SINON, type_id::RELOGE);

					if (!requiers_identifiant(type_id::PARENTHESE_FERMANTE)) {
						lance_erreur("Attendu une paranthèse fermante ')");
					}
				}
				else {
					noeud_reloge->type_declare = analyse_declaration_type(false);
				}

				if (est_identifiant(type_id::SINON)) {
					avance();

					if (!requiers_identifiant(type_id::ACCOLADE_OUVRANTE)) {
						lance_erreur("Attendu une accolade ouvrante '{'");
					}

					analyse_bloc();

					termine_boucle = true;
				}

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_RELOGE);

				expression.pousse(noeud_reloge);
				break;
			}
			case GenreLexeme::DELOGE:
			{
				auto noeud = m_assembleuse->empile_noeud(
							GenreNoeud::EXPRESSION_DELOGE,
							lexeme,
							false);

				analyse_expression(type_id::POINT_VIRGULE, type_id::DELOGE);

				/* besoin de reculer car l'analyse va jusqu'au point-virgule, ce
				 * qui nous fait absorber le code de l'expression suivante */
				recule();

				m_assembleuse->depile_noeud(GenreNoeud::EXPRESSION_DELOGE);

				expression.pousse(noeud);

				break;
			}
			case GenreLexeme::DIRECTIVE:
			{
				if (est_identifiant(GenreLexeme::CHAINE_CARACTERE)) {
					avance();

					auto directive = donnees().chaine;

					if (directive == "inclus") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->ajoute_inclusion(chaine);
					}
					else if (directive == "bib") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->bibliotheques.pousse(chaine);
					}
					else if (directive == "def") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->definitions.pousse(chaine);
					}
					else if (directive == "exécute") {
						m_assembleuse->empile_noeud(GenreNoeud::DIRECTIVE_EXECUTION, lexeme);

						auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::INCONNU);

						m_assembleuse->depile_noeud(GenreNoeud::DIRECTIVE_EXECUTION);

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
					else if (directive == "commande") {
						// ajoute une commande à exécuter après la compilation
						// ignore si pas dans le fichier racine ?
					}
					else if (directive == "sortie") {
						// renseigne le nom et le type (obj, exe) du fichier de sortie
					}
					else if (directive == "chemin") {
						consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->chemins.pousse(chaine);
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
			default:
			{
				lance_erreur("Identifiant inattendu dans l'expression");
			}
		}

		if (termine_boucle) {
			break;
		}

		genre_dernier_lexeme = genre_courant;
	}

	/* Retourne s'il n'y a rien dans l'expression, ceci est principalement pour
	 * éviter de crasher lors des fuzz-tests. */
	if (expression.est_vide()) {
		--m_profondeur;
		return nullptr;
	}

	while (!pile.est_vide()) {
		expression.pousse(pile.back());
		pile.pop_back();
	}

	pile.reserve(expression.taille());

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Expression :" << FIN_LOG_EXPRESSION;

	for (auto noeud : expression) {
		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(noeud->identifiant()) << FIN_LOG_EXPRESSION;

		if (!dls::outils::possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_binaire(noeud->identifiant())) {
			if (pile.taille() < 2) {
				erreur::lance_erreur(
							"Expression malformée pour opérateur binaire",
							m_contexte,
							noeud->donnees_lexeme(),
							erreur::type_erreur::NORMAL);
			}

			auto n2 = pile.back();
			pile.pop_back();

			auto n1 = pile.back();
			pile.pop_back();

			noeud->ajoute_noeud(n1);
			noeud->ajoute_noeud(n2);
		}
		else if (!dls::outils::possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_unaire(noeud->identifiant())) {
			auto n1 = pile.back();
			pile.pop_back();

			noeud->ajoute_noeud(n1);
		}

		pile.pousse(noeud);
	}

	auto noeud_expr = pile.back();

	if (ajoute_noeud) {
		m_assembleuse->ajoute_noeud(noeud_expr);
	}

	pile.pop_back();

	if (pile.taille() != 0) {
		auto premier_noeud = pile.back();
		auto dernier_noeud = premier_noeud;
		pile.pop_back();

		auto pos_premier = premier_noeud->donnees_lexeme().chaine.pointeur();
		auto pos_dernier = pos_premier;

		while (!pile.est_vide()) {
			auto n = pile.back();
			pile.pop_back();

			auto pos_n = n->donnees_lexeme().chaine.pointeur();

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
					premier_noeud->donnees_lexeme(),
					dernier_noeud->donnees_lexeme());
	}

	--m_profondeur;

	return noeud_expr;
}

void Syntaxeuse::analyse_appel_fonction(noeud::base *noeud)
{
	/* ici nous devons être au niveau du premier paramètre */
	while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
		if (sont_2_identifiants(GenreLexeme::CHAINE_CARACTERE, GenreLexeme::EGAL)) {
			avance();

			auto nom_argument = donnees().chaine;
			ajoute_nom_argument(noeud, nom_argument);

			avance();
		}
		else {
			ajoute_nom_argument(noeud, "");
		}

		/* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
		 * si identifiant final == ')', alors l'algorithme s'arrête quand une
		 * paranthèse fermante est trouvé et que la pile est vide */
		analyse_expression(GenreLexeme::VIRGULE, GenreLexeme::EGAL);
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attenu ')' à la fin des argument de l'appel");
}

void Syntaxeuse::analyse_declaration_structure(GenreLexeme id, DonneesLexeme &lexeme)
{
	auto est_externe = false;
	auto est_nonsur = false;

	if (est_identifiant(type_id::EXTERNE)) {
		est_externe = true;
		avance();
	}

	auto noeud_decl = m_assembleuse->empile_noeud(GenreNoeud::DECLARATION_STRUCTURE, lexeme);

	if (est_identifiant(type_id::NONSUR)) {
		est_nonsur = true;
		avance();
	}

	if (m_contexte.structure_existe(lexeme.chaine)) {
		lance_erreur("Redéfinition de la structure", erreur::type_erreur::STRUCTURE_REDEFINIE);
	}

	auto donnees_structure = DonneesStructure{};
	donnees_structure.noeud_decl = noeud_decl;
	donnees_structure.est_enum = false;
	donnees_structure.est_externe = est_externe;
	donnees_structure.est_union = (id == GenreLexeme::UNION);
	donnees_structure.est_nonsur = est_nonsur;

	m_contexte.ajoute_donnees_structure(lexeme.chaine, donnees_structure);

	if (lexeme.chaine == "ContexteProgramme") {
		m_contexte.index_type_contexte = donnees_structure.index_type;
	}

	auto analyse_membres = true;

	if (est_externe) {
		if (est_identifiant(type_id::POINT_VIRGULE)) {
			avance();
			analyse_membres = false;
		}
	}

	if (analyse_membres) {
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

		while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
			auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, type_id::STRUCT);

			if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				noeud->genre = GenreNoeud::DECLARATION_VARIABLE;
			}
		}

		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de la structure");
	}

	m_assembleuse->depile_noeud(GenreNoeud::DECLARATION_STRUCTURE);
}

void Syntaxeuse::analyse_declaration_enum(bool est_drapeau, DonneesLexeme &lexeme)
{
	auto noeud_decl = m_assembleuse->empile_noeud(GenreNoeud::DECLARATION_ENUM, lexeme);

	auto donnees_structure = DonneesStructure{};
	donnees_structure.est_enum = true;
	donnees_structure.est_drapeau = est_drapeau;
	donnees_structure.noeud_decl = noeud_decl;

	m_contexte.ajoute_donnees_structure(lexeme.chaine, donnees_structure);

	noeud_decl->type_declare = analyse_declaration_type(false);

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

	while (!est_identifiant(GenreLexeme::ACCOLADE_FERMANTE)) {
		auto noeud = analyse_expression(GenreLexeme::POINT_VIRGULE, GenreLexeme::EGAL);

		if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			noeud->genre = GenreNoeud::DECLARATION_VARIABLE;
		}
	}

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

	m_assembleuse->depile_noeud(GenreNoeud::DECLARATION_ENUM);
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

		auto dt = DonneesTypeDeclare{};
		dt.pousse(donnees().genre);

		consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu un '(' après 'fonction'");

		dt.pousse(GenreLexeme::PARENTHESE_OUVRANTE);

		if (!nulctx) {
			ajoute_contexte_programme(m_contexte, dt);

			if (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
				dt.pousse(GenreLexeme::VIRGULE);
			}
		}

		while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
			auto dtd = analyse_declaration_type(false);
			dt.pousse(dtd);

			if (!est_identifiant(GenreLexeme::VIRGULE)) {
				break;
			}

			avance();
			dt.pousse(GenreLexeme::VIRGULE);
		}

		avance();
		dt.pousse(GenreLexeme::PARENTHESE_FERMANTE);

		bool eu_paren_ouvrante = false;

		if (est_identifiant(GenreLexeme::PARENTHESE_OUVRANTE)) {
			avance();
			eu_paren_ouvrante = true;
		}

		dt.pousse(GenreLexeme::PARENTHESE_OUVRANTE);

		while (!est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
			auto dtd = analyse_declaration_type(false);
			dt.pousse(dtd);

			auto est_virgule = est_identifiant(GenreLexeme::VIRGULE);

			if ((est_virgule && !eu_paren_ouvrante) || !est_virgule) {
				break;
			}

			avance();
			dt.pousse(GenreLexeme::VIRGULE);
		}

		if (eu_paren_ouvrante && est_identifiant(GenreLexeme::PARENTHESE_FERMANTE)) {
			avance();
		}

		dt.pousse(GenreLexeme::PARENTHESE_FERMANTE);

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
				auto expr = static_cast<noeud::base *>(nullptr);

				if (this->identifiant_courant() != GenreLexeme::CROCHET_FERMANT) {
					expr = analyse_expression(GenreLexeme::CROCHET_FERMANT, GenreLexeme::CROCHET_OUVRANT, false);
				}
				else {
					avance();
				}

				donnees_type.pousse(GenreLexeme::TABLEAU);
				donnees_type.expressions.pousse(expr);

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
							GenreLexeme::TYPE_DE,
							false);

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
			auto const nom_type = donnees().chaine;

			if (!m_contexte.structure_existe(nom_type)) {
				lance_erreur("Structure inconnue", erreur::type_erreur::STRUCTURE_INCONNUE);
			}

			auto const &donnees_structure = m_contexte.donnees_structure(nom_type);
			identifiant = (identifiant | (static_cast<int>(donnees_structure.id) << 8));
		}

		donnees_type.pousse(identifiant);
	}

	return donnees_type;
}

void Syntaxeuse::analyse_construction_structure(noeud::base *noeud)
{
	auto liste_param = dls::tableau<dls::vue_chaine_compacte>{};

	/* ici nous devons être au niveau du premier paramètre */
	while (!est_identifiant(type_id::ACCOLADE_FERMANTE)) {
		if (!sont_2_identifiants(type_id::CHAINE_CARACTERE, type_id::EGAL)) {
			lance_erreur(
						"Le nom des membres est requis pour la construction de la structure",
						erreur::type_erreur::MEMBRE_INCONNU);
		}

		avance();

		auto nom = donnees().chaine;
		liste_param.pousse(nom);

		avance();

		analyse_expression(GenreLexeme::VIRGULE, GenreLexeme::EGAL);
	}

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la construction de la structure");

	noeud->drapeaux |= EST_CALCULE;
	noeud->valeur_calculee = liste_param;
}

void Syntaxeuse::analyse_directive_si()
{
	avance();
	avance();

	auto directive = donnees().chaine;

	if (directive == "linux") {

	}
	else if (directive == "windows") {

	}
	else if (directive == "macos") {

	}
	else if (directive == "vrai") {

	}
	else if (directive == "faux") {

	}
	else if (directive == "arch8") {

	}
	else if (directive == "arch32") {

	}
	else if (directive == "arch64") {

	}
	else {
		lance_erreur("Directive inconnue");
	}
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
	erreur::lance_erreur(quoi, m_contexte, donnees(), type);
}
