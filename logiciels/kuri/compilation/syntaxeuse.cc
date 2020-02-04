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

using denombreuse = lng::decoupeuse_nombre<TypeLexeme>;

/**
 * Retourne vrai si l'identifiant passé en paramètre peut-être un identifiant
 * valide pour précèder un opérateur unaire '+' ou '-'.
 */
static bool precede_unaire_valide(TypeLexeme dernier_identifiant)
{
	if (dernier_identifiant == TypeLexeme::PARENTHESE_FERMANTE) {
		return false;
	}

	if (dernier_identifiant == TypeLexeme::CROCHET_FERMANT) {
		return false;
	}

	if (dernier_identifiant == TypeLexeme::CHAINE_CARACTERE) {
		return false;
	}

	if (est_nombre(dernier_identifiant)) {
		return false;
	}

	/* À FAIRE : ceci mélange a[-i] et a[i] - b[i] */
	if (dernier_identifiant == TypeLexeme::CROCHET_OUVRANT) {
		return false;
	}

	if (dernier_identifiant == TypeLexeme::CARACTERE) {
		return false;
	}

	if (dernier_identifiant == TypeLexeme::TRANSTYPE) {
		return false;
	}

	if (dernier_identifiant == TypeLexeme::MEMOIRE) {
		return false;
	}

	return true;
}

/* ************************************************************************** */

Syntaxeuse::Syntaxeuse(
		ContexteGenerationCode &contexte,
		Fichier *fichier,
		dls::chaine const &racine_kuri)
	: lng::analyseuse<DonneesLexeme>(fichier->morceaux)
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
			case TypeLexeme::FONC:
			case TypeLexeme::COROUT:
			{
				avance();
				analyse_declaration_fonction(id);
				break;
			}
			case TypeLexeme::STRUCT:
			case TypeLexeme::UNION:
			{
				avance();
				analyse_declaration_structure(id);
				break;
			}
			case TypeLexeme::ENUM:
			{
				avance();
				analyse_declaration_enum(false);
				break;
			}
			case TypeLexeme::ENUM_DRAPEAU:
			{
				avance();
				analyse_declaration_enum(true);
				break;
			}
			case TypeLexeme::IMPORTE:
			{
				avance();

				if (!est_identifiant(TypeLexeme::CHAINE_LITTERALE) && !est_identifiant(TypeLexeme::CHAINE_CARACTERE)) {
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
			case TypeLexeme::CHARGE:
			{
				avance();

				if (!est_identifiant(TypeLexeme::CHAINE_LITTERALE) && !est_identifiant(TypeLexeme::CHAINE_CARACTERE)) {
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
				auto noeud = analyse_expression(TypeLexeme::POINT_VIRGULE, TypeLexeme::INCONNU);

				if (noeud && noeud->type == type_noeud::VARIABLE) {
					noeud->type = type_noeud::DECLARATION_VARIABLE;
				}
				m_global = false;
				break;
			}
		}
	}
}

void Syntaxeuse::analyse_declaration_fonction(TypeLexeme id)
{
	auto externe = false;

	if (est_identifiant(TypeLexeme::EXTERNE)) {
		avance();
		externe = true;

		if (id == TypeLexeme::COROUT && externe) {
			lance_erreur("Une coroutine ne peut pas être externe");
		}
	}

	consomme(TypeLexeme::CHAINE_CARACTERE, "Attendu la déclaration du nom de la fonction");

	auto const nom_fonction = donnees().chaine;
	m_fichier->module->fonctions_exportees.insere(nom_fonction);

	auto noeud = m_assembleuse->empile_noeud(type_noeud::DECLARATION_FONCTION, m_contexte, donnees());

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

	consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	auto donnees_fonctions = DonneesFonction{};
	donnees_fonctions.est_coroutine = (id == TypeLexeme::COROUT);
	donnees_fonctions.est_externe = externe;
	donnees_fonctions.noeud_decl = noeud;

	/* analyse les paramètres de la fonction */
	m_assembleuse->empile_noeud(type_noeud::LISTE_PARAMETRES_FONCTION, m_contexte, donnees());

	analyse_expression(TypeLexeme::PARENTHESE_FERMANTE, TypeLexeme::PARENTHESE_OUVRANTE);

	m_assembleuse->depile_noeud(type_noeud::LISTE_PARAMETRES_FONCTION);

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

	m_fichier->module->ajoute_donnees_fonctions(nom_fonction, donnees_fonctions);

	if (externe) {
		consomme(TypeLexeme::POINT_VIRGULE, "Attendu un point-virgule ';' après la déclaration de la fonction externe");

		if (donnees_fonctions.idx_types_retours.taille() > 1) {
			lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
		}
	}
	else {
		/* ignore les points-virgules implicites */
		if (est_identifiant(TypeLexeme::POINT_VIRGULE)) {
			avance();
		}

		consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après la liste des paramètres de la fonction");

		analyse_bloc();
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_FONCTION);
}

void Syntaxeuse::analyse_controle_si(type_noeud tn)
{
	m_assembleuse->empile_noeud(tn, m_contexte, donnees());

	analyse_expression(TypeLexeme::ACCOLADE_OUVRANTE, TypeLexeme::SI);

	analyse_bloc();

	if (est_identifiant(TypeLexeme::SINON)) {
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
		m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());

		if (est_identifiant(TypeLexeme::SI)) {
			avance();
			analyse_controle_si(type_noeud::SI);
		}
		else if (est_identifiant(TypeLexeme::SAUFSI)) {
			avance();
			analyse_controle_si(type_noeud::SAUFSI);
		}
		else {
			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après 'sinon'");

			analyse_corps_fonction();

			consomme(TypeLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante à la fin du contrôle 'sinon'");
		}

		m_assembleuse->depile_noeud(type_noeud::BLOC);
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
	m_assembleuse->empile_noeud(type_noeud::POUR, m_contexte, donnees());

	/* enfant 1 : déclaration variable */

	analyse_expression(TypeLexeme::DANS, TypeLexeme::POUR);

	/* enfant 2 : expr */

	analyse_expression(TypeLexeme::ACCOLADE_OUVRANTE, TypeLexeme::DANS);

	recule();

	consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'pour'");

	/* enfant 3 : bloc */
	analyse_bloc();

	/* enfant 4 : bloc sansarrêt (optionel) */
	if (est_identifiant(TypeLexeme::SANSARRET)) {
		avance();

		consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'sansarrêt'");

		analyse_bloc();
	}

	/* enfant 4 ou 5 : bloc sinon (optionel) */
	if (est_identifiant(TypeLexeme::SINON)) {
		avance();

		consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");

		analyse_bloc();
	}

	m_assembleuse->depile_noeud(type_noeud::POUR);
}

void Syntaxeuse::analyse_corps_fonction()
{
	/* Il est possible qu'une fonction soit vide, donc vérifie d'abord que
	 * l'on n'ait pas terminé. */
	while (!est_identifiant(TypeLexeme::ACCOLADE_FERMANTE)) {
		auto const pos = position();

		if (est_identifiant(TypeLexeme::RETOURNE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETOUR, m_contexte, donnees());

			/* Considération du cas où l'on ne retourne rien 'retourne;'. */
			if (!est_identifiant(TypeLexeme::POINT_VIRGULE)) {
				analyse_expression(TypeLexeme::POINT_VIRGULE, TypeLexeme::RETOURNE);
			}
			else {
				avance();
			}

			m_assembleuse->depile_noeud(type_noeud::RETOUR);
		}
		else if (est_identifiant(TypeLexeme::RETIENS)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETIENS, m_contexte, donnees());

			analyse_expression(TypeLexeme::POINT_VIRGULE, TypeLexeme::RETOURNE);

			m_assembleuse->depile_noeud(type_noeud::RETIENS);
		}
		else if (est_identifiant(TypeLexeme::POUR)) {
			avance();
			analyse_controle_pour();
		}
		else if (est_identifiant(TypeLexeme::BOUCLE)) {
			avance();

			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'boucle'");

			m_assembleuse->empile_noeud(type_noeud::BOUCLE, m_contexte, donnees());

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::BOUCLE);
		}
		else if (est_identifiant(TypeLexeme::REPETE)) {
			avance();

			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'répète'");

			m_assembleuse->empile_noeud(type_noeud::REPETE, m_contexte, donnees());

			analyse_bloc();

			consomme(TypeLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

			analyse_expression(TypeLexeme::POINT_VIRGULE, TypeLexeme::TANTQUE);

			m_assembleuse->depile_noeud(type_noeud::REPETE);
		}
		else if (est_identifiant(TypeLexeme::TANTQUE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::TANTQUE, m_contexte, donnees());

			analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::TANTQUE);

			/* recule pour être de nouveau synchronisé */
			recule();

			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de 'tanque'");

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::TANTQUE);
		}
		else if (est_identifiant(TypeLexeme::ARRETE) || est_identifiant(TypeLexeme::CONTINUE)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::CONTINUE_ARRETE, m_contexte, donnees());

			if (est_identifiant(TypeLexeme::CHAINE_CARACTERE)) {
				avance();
				m_assembleuse->empile_noeud(type_noeud::VARIABLE, m_contexte, donnees());
				m_assembleuse->depile_noeud(type_noeud::VARIABLE);
			}

			m_assembleuse->depile_noeud(type_noeud::CONTINUE_ARRETE);

			consomme(TypeLexeme::POINT_VIRGULE, "Attendu un point virgule ';'");
		}
		else if (est_identifiant(TypeLexeme::DIFFERE)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::DIFFERE, m_contexte, donnees());

			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'diffère'");

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::DIFFERE);
		}
		else if (est_identifiant(TypeLexeme::NONSUR)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::NONSUR, m_contexte, donnees());

			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après 'nonsûr'");

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::NONSUR);
		}
		else if (est_identifiant(TypeLexeme::DISCR)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::DISCR, m_contexte, donnees());

			analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::DISCR);

			/* recule pour être de nouveau synchronisé */
			recule();

			consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

			auto sinon_rencontre = false;

			while (!est_identifiant(TypeLexeme::ACCOLADE_FERMANTE)) {
				m_assembleuse->empile_noeud(type_noeud::PAIRE_DISCR, m_contexte, donnees());

				if (est_identifiant(TypeLexeme::SINON)) {
					avance();

					if (sinon_rencontre) {
						lance_erreur("Redéfinition d'un bloc sinon");
					}

					auto noeud = m_assembleuse->cree_noeud(type_noeud::SINON, m_contexte, donnees());
					m_assembleuse->ajoute_noeud(noeud);

					sinon_rencontre = true;
				}
				else {
					analyse_expression(type_id::ACCOLADE_OUVRANTE, type_id::DISCR);

					/* recule pour être de nouveau synchronisé */
					recule();
				}

				consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

				analyse_bloc();

				m_assembleuse->depile_noeud(type_noeud::PAIRE_DISCR);
			}

			consomme(TypeLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

			m_assembleuse->depile_noeud(type_noeud::DISCR);
		}
		else if (est_identifiant(type_id::ACCOLADE_OUVRANTE)) {
			avance();
			analyse_bloc();
		}
		else {
			auto noeud = analyse_expression(TypeLexeme::POINT_VIRGULE, TypeLexeme::EGAL);

			if (noeud && noeud->type == type_noeud::VARIABLE) {
				noeud->type = type_noeud::DECLARATION_VARIABLE;
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
	m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
	analyse_corps_fonction();
	m_assembleuse->depile_noeud(type_noeud::BLOC);

	consomme(TypeLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc");
}

noeud::base *Syntaxeuse::analyse_expression(
		TypeLexeme identifiant_final,
		TypeLexeme racine_expr,
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

	auto vide_pile_operateur = [&](TypeLexeme id_operateur)
	{
		while (!pile.est_vide()
			   && (precedence_faible(id_operateur, pile.back()->identifiant())))
		{
			expression.pousse(pile.back());
			pile.pop_back();
		}
	};

	auto dernier_identifiant = (m_position == 0) ? TypeLexeme::INCONNU : donnees().identifiant;

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	auto assignation = false;

	auto drapeaux = drapeaux_noeud::AUCUN;

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

	while (!requiers_identifiant(identifiant_final)) {
		auto &morceau = donnees();

		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(morceau.identifiant) << FIN_LOG_EXPRESSION;

		auto id_courant = morceau.identifiant;

		switch (id_courant) {
			case TypeLexeme::DYN:
			{
				drapeaux |= (DYNAMIC | DECLARATION);
				break;
			}
			case TypeLexeme::EMPL:
			{
				drapeaux |= EMPLOYE;
				break;
			}
			case TypeLexeme::EXTERNE:
			{
				drapeaux |= (EST_EXTERNE | DECLARATION);
				break;
			}
			case TypeLexeme::CHAINE_CARACTERE:
			{
				/* appel fonction : chaine + ( */
				if (est_identifiant(TypeLexeme::PARENTHESE_OUVRANTE)) {
					avance();

					auto noeud = m_assembleuse->empile_noeud(type_noeud::APPEL_FONCTION, m_contexte, morceau, false);

					analyse_appel_fonction(noeud);

					m_assembleuse->depile_noeud(type_noeud::APPEL_FONCTION);

					expression.pousse(noeud);
				}
				/* construction structure : chaine + { */
				else if ((racine_expr == TypeLexeme::EGAL || racine_expr == type_id::RETOURNE) && est_identifiant(TypeLexeme::ACCOLADE_OUVRANTE)) {
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_STRUCTURE, m_contexte, morceau, false);

					avance();

					analyse_construction_structure(noeud);

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_STRUCTURE);

					expression.pousse(noeud);

					termine_boucle = true;
				}
				/* variable : chaine */
				else {
					auto noeud = m_assembleuse->cree_noeud(type_noeud::VARIABLE, m_contexte, morceau);
					expression.pousse(noeud);

					noeud->drapeaux |= drapeaux;
					drapeaux = drapeaux_noeud::AUCUN;

					/* nous avons la déclaration d'un type dans la structure */
					if ((racine_expr != type_id::TRANSTYPE && racine_expr != type_id::LOGE && racine_expr != type_id::RELOGE) && est_identifiant(TypeLexeme::DOUBLE_POINTS)) {
						noeud->type_declare = analyse_declaration_type();
						drapeaux |= DECLARATION;
					}
				}

				break;
			}
			case TypeLexeme::NOMBRE_REEL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_REEL, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::NOMBRE_BINAIRE:
			case TypeLexeme::NOMBRE_ENTIER:
			case TypeLexeme::NOMBRE_HEXADECIMAL:
			case TypeLexeme::NOMBRE_OCTAL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_ENTIER, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::CHAINE_LITTERALE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CHAINE_LITTERALE, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::CARACTERE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CARACTERE, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::NUL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NUL, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::TAILLE_DE:
			{
				consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

				auto noeud = m_assembleuse->cree_noeud(type_noeud::TAILLE_DE, m_contexte, morceau);
				noeud->valeur_calculee = analyse_declaration_type(false);

				consomme(TypeLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::INFO_DE:
			{
				consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

				auto noeud = m_assembleuse->empile_noeud(type_noeud::INFO_DE, m_contexte, morceau, false);

				analyse_expression(TypeLexeme::INCONNU, TypeLexeme::INFO_DE);

				m_assembleuse->depile_noeud(type_noeud::INFO_DE);

				/* vérifie mais n'avance pas */
				consomme(TypeLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'taille_de'");

				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::VRAI:
			case TypeLexeme::FAUX:
			{
				/* remplace l'identifiant par id_morceau::BOOL */
				morceau.identifiant = TypeLexeme::BOOL;
				auto noeud = m_assembleuse->cree_noeud(type_noeud::BOOLEEN, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::TRANSTYPE:
			{
				consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'transtype'");

				auto noeud = m_assembleuse->empile_noeud(type_noeud::TRANSTYPE, m_contexte, morceau, false);

				analyse_expression(TypeLexeme::DOUBLE_POINTS, TypeLexeme::TRANSTYPE);

				noeud->type_declare = analyse_declaration_type(false);

				consomme(TypeLexeme::PARENTHESE_FERMANTE, "Attendu ')' après la déclaration du type");

				m_assembleuse->depile_noeud(type_noeud::TRANSTYPE);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::MEMOIRE:
			{
				consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

				auto noeud = m_assembleuse->empile_noeud(type_noeud::MEMOIRE, m_contexte, morceau, false);

				analyse_expression(TypeLexeme::INCONNU, TypeLexeme::MEMOIRE);

				consomme(TypeLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

				m_assembleuse->depile_noeud(type_noeud::MEMOIRE);
				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::PARENTHESE_OUVRANTE:
			{
				auto noeud = m_assembleuse->empile_noeud(type_noeud::EXPRESSION_PARENTHESE, m_contexte, morceau, false);

				analyse_expression(TypeLexeme::PARENTHESE_FERMANTE, TypeLexeme::PARENTHESE_OUVRANTE);

				m_assembleuse->depile_noeud(type_noeud::EXPRESSION_PARENTHESE);

				expression.pousse(noeud);

				/* ajourne id_courant avec une parenthèse fermante, car étant
				 * une parenthèse ouvrante, il ferait échouer le test de
				 * détermination d'un opérateur unaire */
				id_courant = TypeLexeme::PARENTHESE_FERMANTE;

				break;
			}
			case TypeLexeme::PARENTHESE_FERMANTE:
			{
				/* recule pour être synchronisé avec les différentes sorties */
				recule();
				termine_boucle = true;
				break;
			}
			/* opérations binaire */
			case TypeLexeme::PLUS:
			case TypeLexeme::MOINS:
			{
				auto id_operateur = morceau.identifiant;
				auto noeud = static_cast<noeud::base *>(nullptr);

				if (precede_unaire_valide(dernier_identifiant)) {
					if (id_operateur == TypeLexeme::PLUS) {
						id_operateur = TypeLexeme::PLUS_UNAIRE;
					}
					else if (id_operateur == TypeLexeme::MOINS) {
						id_operateur = TypeLexeme::MOINS_UNAIRE;
					}

					morceau.identifiant = id_operateur;
					noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
				}
				else {
					noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
				}

				vide_pile_operateur(id_operateur);

				pile.pousse(noeud);

				break;
			}
			case TypeLexeme::FOIS:
			case TypeLexeme::DIVISE:
			case TypeLexeme::ESPERLUETTE:
			case TypeLexeme::POURCENT:
			case TypeLexeme::INFERIEUR:
			case TypeLexeme::INFERIEUR_EGAL:
			case TypeLexeme::SUPERIEUR:
			case TypeLexeme::SUPERIEUR_EGAL:
			case TypeLexeme::DECALAGE_DROITE:
			case TypeLexeme::DECALAGE_GAUCHE:
			case TypeLexeme::DIFFERENCE:
			case TypeLexeme::ESP_ESP:
			case TypeLexeme::EGALITE:
			case TypeLexeme::BARRE_BARRE:
			case TypeLexeme::BARRE:
			case TypeLexeme::CHAPEAU:
			case TypeLexeme::PLUS_EGAL:
			case TypeLexeme::MOINS_EGAL:
			case TypeLexeme::DIVISE_EGAL:
			case TypeLexeme::MULTIPLIE_EGAL:
			case TypeLexeme::MODULO_EGAL:
			case TypeLexeme::ET_EGAL:
			case TypeLexeme::OU_EGAL:
			case TypeLexeme::OUX_EGAL:
			case TypeLexeme::DEC_DROITE_EGAL:
			case TypeLexeme::DEC_GAUCHE_EGAL:
			case TypeLexeme::VIRGULE:
			{
				/* Correction de crash d'aléatest, improbable dans la vrai vie. */
				if (expression.est_vide() && est_operateur_binaire(morceau.identifiant)) {
					lance_erreur("Opérateur binaire utilisé en début d'expression");
				}

				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
				pile.pousse(noeud);

				break;
			}
			case TypeLexeme::TROIS_POINTS:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::PLAGE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case TypeLexeme::POINT:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE_POINT, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case TypeLexeme::EGAL:
			{
				if (assignation) {
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				vide_pile_operateur(morceau.identifiant);

				type_noeud tn;

				if ((drapeaux & DECLARATION) != 0) {
					tn = type_noeud::DECLARATION_VARIABLE;
				}
				else {
					tn = type_noeud::ASSIGNATION_VARIABLE;
				}

				auto noeud = m_assembleuse->cree_noeud(tn, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case TypeLexeme::DECLARATION_VARIABLE:
			{
				if (assignation) {
					lance_erreur("Ne peut faire de déclaration dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				vide_pile_operateur(morceau.identifiant);

				auto noeud = m_assembleuse->cree_noeud(type_noeud::DECLARATION_VARIABLE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case TypeLexeme::CROCHET_OUVRANT:
			{
				/* l'accès à un élément d'un tableau est chaine[index] */
				if (dernier_identifiant == TypeLexeme::CHAINE_CARACTERE
						|| dernier_identifiant == TypeLexeme::CHAINE_LITTERALE
						 || dernier_identifiant == TypeLexeme::CROCHET_OUVRANT) {
					vide_pile_operateur(morceau.identifiant);

					auto noeud = m_assembleuse->empile_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau, false);
					pile.pousse(noeud);

					analyse_expression(TypeLexeme::CROCHET_FERMANT, TypeLexeme::CROCHET_OUVRANT);

					/* Extrait le noeud enfant, il sera de nouveau ajouté dans
					 * la compilation de l'expression à la fin de la fonction. */
					auto noeud_expr = noeud->enfants.front();
					noeud->enfants.efface();

					/* Si la racine de l'expression est un opérateur, il faut
					 * l'empêcher d'être prise en compte pour l'expression
					 * courante. */
					noeud_expr->drapeaux |= IGNORE_OPERATEUR;

					expression.pousse(noeud_expr);

					m_assembleuse->depile_noeud(type_noeud::OPERATION_BINAIRE);
				}
				else {
					/* change l'identifiant pour ne pas le confondre avec l'opérateur binaire [] */
					morceau.identifiant = TypeLexeme::TABLEAU;
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_TABLEAU, m_contexte, morceau, false);

					analyse_expression(TypeLexeme::CROCHET_FERMANT, TypeLexeme::CROCHET_OUVRANT);

					/* il est possible que le crochet n'ait pas été consommé,
					 * par exemple dans le cas où nous avons un point-virgule
					 * implicite dans la construction */
					if (est_identifiant(TypeLexeme::CROCHET_FERMANT)) {
						avance();
					}

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_TABLEAU);

					expression.pousse(noeud);
				}

				break;
			}
			/* opérations unaire */
			case TypeLexeme::AROBASE:
			case TypeLexeme::EXCLAMATION:
			case TypeLexeme::TILDE:
			case TypeLexeme::PLUS_UNAIRE:
			case TypeLexeme::MOINS_UNAIRE:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case TypeLexeme::ACCOLADE_FERMANTE:
			{
				/* une accolade fermante marque généralement la fin de la
				 * construction d'une structure */
				termine_boucle = true;
				/* recule pour être synchroniser avec la sortie dans
				 * analyse_construction_structure() */
				recule();
				break;
			}
			case TypeLexeme::LOGE:
			{
				auto noeud = m_assembleuse->empile_noeud(
							type_noeud::LOGE,
							m_contexte,
							morceau,
							false);

				if (est_identifiant(TypeLexeme::CHAINE)) {
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

				m_assembleuse->depile_noeud(type_noeud::LOGE);

				expression.pousse(noeud);
				break;
			}
			case TypeLexeme::RELOGE:
			{
				/* reloge nom : type; */
				auto noeud_reloge = m_assembleuse->empile_noeud(
							type_noeud::RELOGE,
							m_contexte,
							morceau,
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

				m_assembleuse->depile_noeud(type_noeud::RELOGE);

				expression.pousse(noeud_reloge);
				break;
			}
			case TypeLexeme::DELOGE:
			{
				auto noeud = m_assembleuse->empile_noeud(
							type_noeud::DELOGE,
							m_contexte,
							morceau,
							false);

				analyse_expression(type_id::POINT_VIRGULE, type_id::DELOGE);

				/* besoin de reculer car l'analyse va jusqu'au point-virgule, ce
				 * qui nous fait absorber le code de l'expression suivante */
				recule();

				m_assembleuse->depile_noeud(type_noeud::DELOGE);

				expression.pousse(noeud);

				break;
			}
			case TypeLexeme::DIRECTIVE:
			{
				if (est_identifiant(TypeLexeme::CHAINE_CARACTERE)) {
					avance();

					auto directive = donnees().chaine;

					if (directive == "inclus") {
						consomme(TypeLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->ajoute_inclusion(chaine);
					}
					else if (directive == "bib") {
						consomme(TypeLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->bibliotheques.pousse(chaine);
					}
					else if (directive == "def") {
						consomme(TypeLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

						auto chaine = donnees().chaine;
						m_assembleuse->definitions.pousse(chaine);
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
						// ajoute le chemin à la liste des chemins où chercher les modules
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
				else if (est_identifiant(TypeLexeme::SI)) {
					analyse_directive_si();
				}
				else if (est_identifiant(TypeLexeme::SINON)) {
					avance();

					if (est_identifiant(TypeLexeme::SI)) {
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
			case TypeLexeme::SI:
			{
				analyse_controle_si(type_noeud::SI);
				termine_boucle = true;
				break;
			}
			case TypeLexeme::SAUFSI:
			{
				analyse_controle_si(type_noeud::SAUFSI);
				termine_boucle = true;
				break;
			}
			case TypeLexeme::POINT_VIRGULE:
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

		dernier_identifiant = id_courant;
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
							noeud->donnees_morceau(),
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

		auto pos_premier = premier_noeud->donnees_morceau().chaine.pointeur();
		auto pos_dernier = pos_premier;

		while (!pile.est_vide()) {
			auto n = pile.back();
			pile.pop_back();

			auto pos_n = n->donnees_morceau().chaine.pointeur();

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
					premier_noeud->donnees_morceau(),
					dernier_noeud->donnees_morceau());
	}

	--m_profondeur;

	return noeud_expr;
}

void Syntaxeuse::analyse_appel_fonction(noeud::base *noeud)
{
	/* ici nous devons être au niveau du premier paramètre */
	while (!est_identifiant(TypeLexeme::PARENTHESE_FERMANTE)) {
		if (sont_2_identifiants(TypeLexeme::CHAINE_CARACTERE, TypeLexeme::EGAL)) {
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
		analyse_expression(TypeLexeme::VIRGULE, TypeLexeme::EGAL);
	}

	consomme(TypeLexeme::PARENTHESE_FERMANTE, "Attenu ')' à la fin des argument de l'appel");
}

void Syntaxeuse::analyse_declaration_structure(TypeLexeme id)
{
	auto est_externe = false;
	auto est_nonsur = false;

	if (est_identifiant(type_id::EXTERNE)) {
		est_externe = true;
		avance();
	}

	consomme(TypeLexeme::CHAINE_CARACTERE, "Attendu une chaine de caractères après 'struct'");

	auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_STRUCTURE, m_contexte, donnees());
	auto nom_structure = donnees().chaine;

	if (est_identifiant(type_id::NONSUR)) {
		est_nonsur = true;
		avance();
	}

	if (m_contexte.structure_existe(nom_structure)) {
		lance_erreur("Redéfinition de la structure", erreur::type_erreur::STRUCTURE_REDEFINIE);
	}

	auto donnees_structure = DonneesStructure{};
	donnees_structure.noeud_decl = noeud_decl;
	donnees_structure.est_enum = false;
	donnees_structure.est_externe = est_externe;
	donnees_structure.est_union = (id == TypeLexeme::UNION);
	donnees_structure.est_nonsur = est_nonsur;

	m_contexte.ajoute_donnees_structure(nom_structure, donnees_structure);

	if (nom_structure == "ContexteProgramme") {
		auto dt = DonneesTypeFinal();
		dt.pousse(TypeLexeme::POINTEUR);
		dt.pousse(m_contexte.typeuse[donnees_structure.index_type]);
		m_contexte.index_type_contexte = m_contexte.typeuse.ajoute_type(dt);
	}

	auto analyse_membres = true;

	if (est_externe) {
		if (est_identifiant(type_id::POINT_VIRGULE)) {
			avance();
			analyse_membres = false;
		}
	}

	if (analyse_membres) {
		consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

		while (!est_identifiant(TypeLexeme::ACCOLADE_FERMANTE)) {
			auto noeud = analyse_expression(TypeLexeme::POINT_VIRGULE, type_id::STRUCT);

			if (noeud->type == type_noeud::VARIABLE) {
				noeud->type = type_noeud::DECLARATION_VARIABLE;
			}
		}

		consomme(TypeLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de la structure");
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_STRUCTURE);
}

void Syntaxeuse::analyse_declaration_enum(bool est_drapeau)
{
	consomme(TypeLexeme::CHAINE_CARACTERE, "Attendu un nom après 'énum'");

	auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_ENUM, m_contexte, donnees());
	auto nom = noeud_decl->morceau.chaine;

	auto donnees_structure = DonneesStructure{};
	donnees_structure.est_enum = true;
	donnees_structure.est_drapeau = est_drapeau;
	donnees_structure.noeud_decl = noeud_decl;

	m_contexte.ajoute_donnees_structure(nom, donnees_structure);

	noeud_decl->type_declare = analyse_declaration_type();

	consomme(TypeLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

	while (!est_identifiant(TypeLexeme::ACCOLADE_FERMANTE)) {
		auto noeud = analyse_expression(TypeLexeme::POINT_VIRGULE, TypeLexeme::EGAL);

		if (noeud->type == type_noeud::VARIABLE) {
			noeud->type = type_noeud::DECLARATION_VARIABLE;
		}
	}

	consomme(TypeLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_ENUM);
}

DonneesTypeDeclare Syntaxeuse::analyse_declaration_type(bool double_point)
{
	if (double_point && !requiers_identifiant(TypeLexeme::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':'");
	}

	auto nulctx = false;

	if (est_identifiant(TypeLexeme::DIRECTIVE)) {
		avance();
		avance();

		auto nom_directive = donnees().chaine;

		if (nom_directive != "nulctx") {
			lance_erreur("Directive invalide pour le type");
		}

		nulctx = true;
	}

	/* Vérifie si l'on a un pointeur vers une fonction. */
	if (est_identifiant(TypeLexeme::FONC) || est_identifiant(TypeLexeme::COROUT)) {
		avance();

		auto dt = DonneesTypeDeclare{};
		dt.pousse(donnees().identifiant);

		consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu un '(' après 'fonction'");

		dt.pousse(TypeLexeme::PARENTHESE_OUVRANTE);

		if (!nulctx) {
			ajoute_contexte_programme(m_contexte, dt);

			if (!est_identifiant(TypeLexeme::PARENTHESE_FERMANTE)) {
				dt.pousse(TypeLexeme::VIRGULE);
			}
		}

		while (!est_identifiant(TypeLexeme::PARENTHESE_FERMANTE)) {
			auto dtd = analyse_declaration_type(false);
			dt.pousse(dtd);

			if (!est_identifiant(TypeLexeme::VIRGULE)) {
				break;
			}

			avance();
			dt.pousse(TypeLexeme::VIRGULE);
		}

		avance();
		dt.pousse(TypeLexeme::PARENTHESE_FERMANTE);

		bool eu_paren_ouvrante = false;

		if (est_identifiant(TypeLexeme::PARENTHESE_OUVRANTE)) {
			avance();
			eu_paren_ouvrante = true;
		}

		dt.pousse(TypeLexeme::PARENTHESE_OUVRANTE);

		while (!est_identifiant(TypeLexeme::PARENTHESE_FERMANTE)) {
			auto dtd = analyse_declaration_type(false);
			dt.pousse(dtd);

			auto est_virgule = est_identifiant(TypeLexeme::VIRGULE);

			if ((est_virgule && !eu_paren_ouvrante) || !est_virgule) {
				break;
			}

			avance();
			dt.pousse(TypeLexeme::VIRGULE);
		}

		if (eu_paren_ouvrante && est_identifiant(TypeLexeme::PARENTHESE_FERMANTE)) {
			avance();
		}

		dt.pousse(TypeLexeme::PARENTHESE_FERMANTE);

		return dt;
	}

	return analyse_declaration_type_ex();
}

DonneesTypeDeclare Syntaxeuse::analyse_declaration_type_ex()
{
	auto dernier_id = TypeLexeme{};
	auto donnees_type = DonneesTypeDeclare{};

	while (est_specifiant_type(identifiant_courant())) {
		auto id = this->identifiant_courant();
		avance();

		switch (id) {
			case type_id::CROCHET_OUVRANT:
			{
				auto expr = static_cast<noeud::base *>(nullptr);

				if (this->identifiant_courant() != TypeLexeme::CROCHET_FERMANT) {
					expr = analyse_expression(TypeLexeme::CROCHET_FERMANT, TypeLexeme::CROCHET_OUVRANT, false);
				}
				else {
					avance();
				}

				donnees_type.pousse(TypeLexeme::TABLEAU);
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
				donnees_type.pousse(TypeLexeme::POINTEUR);
				break;
			}
			case type_id::ESPERLUETTE:
			{
				donnees_type.pousse(TypeLexeme::REFERENCE);
				break;
			}
			case type_id::TYPE_DE:
			{
				consomme(TypeLexeme::PARENTHESE_OUVRANTE, "Attendu un '(' après 'type_de'");

				auto expr = analyse_expression(
							TypeLexeme::PARENTHESE_FERMANTE,
							TypeLexeme::TYPE_DE,
							false);

				donnees_type.pousse(TypeLexeme::TYPE_DE);
				donnees_type.expressions.pousse(expr);

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

	if (dernier_id == TypeLexeme::TYPE_DE) {
		type_attendu = false;
	}

	/* Soutiens pour les types des fonctions variadiques externes. */
	if (dernier_id == TypeLexeme::TROIS_POINTS && est_identifiant(type_id::PARENTHESE_FERMANTE)) {
		type_attendu = false;
	}

	if (type_attendu) {
		consomme_type("Attendu la déclaration d'un type");

		auto identifiant = donnees().identifiant;

		if (identifiant == TypeLexeme::CHAINE_CARACTERE) {
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

		analyse_expression(TypeLexeme::VIRGULE, TypeLexeme::EGAL);
	}

	consomme(TypeLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la construction de la structure");

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

void Syntaxeuse::consomme(TypeLexeme id, const char *message)
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
