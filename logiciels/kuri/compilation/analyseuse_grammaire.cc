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

#include "analyseuse_grammaire.h"

#undef DEBOGUE_EXPRESSION

#include "biblinternes/langage/debogage.hh"
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "expression.h"

using denombreuse = lng::decoupeuse_nombre<id_morceau>;

/**
 * Pointeur spécial utilisé pour représenter un noeud de type paranthèse
 * ouvrante dans l'arbre syntactic. Ce noeud n'est pas insérer dans l'arbre,
 * mais simplement utilisé pour compiler les arbres syntactics des expressions.
 */
static auto NOEUD_PARENTHESE = reinterpret_cast<noeud::base *>(id_morceau::PARENTHESE_OUVRANTE);

static bool est_specifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::FOIS:
		case id_morceau::ESPERLUETTE:
		case id_morceau::CROCHET_OUVRANT:
		case id_morceau::TROIS_POINTS:
		case id_morceau::TYPE_DE:
			return true;
		default:
			return false;
	}
}

static bool est_identifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::N128:
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::R128:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::Z128:
		case id_morceau::BOOL:
		case id_morceau::RIEN:
		case id_morceau::EINI:
		case id_morceau::CHAINE:
		case id_morceau::CHAINE_CARACTERE:
		case id_morceau::OCTET:
			return true;
		default:
			return false;
	}
}

static bool est_nombre_entier(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::NOMBRE_BINAIRE:
		case id_morceau::NOMBRE_ENTIER:
		case id_morceau::NOMBRE_HEXADECIMAL:
		case id_morceau::NOMBRE_OCTAL:
			return true;
		default:
			return false;
	}
}

static bool est_nombre(id_morceau identifiant)
{
	return est_nombre_entier(identifiant) || (identifiant == id_morceau::NOMBRE_REEL);
}

static bool est_operateur_unaire(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::AROBASE:
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::CROCHET_OUVRANT:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}

static bool est_operateur_binaire(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MOINS_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		case id_morceau::DEC_DROITE_EGAL:
		case id_morceau::DEC_GAUCHE_EGAL:
		case id_morceau::ESPERLUETTE:
		case id_morceau::POURCENT:
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DIFFERENCE:
		case id_morceau::ESP_ESP:
		case id_morceau::EGALITE:
		case id_morceau::BARRE_BARRE:
		case id_morceau::BARRE:
		case id_morceau::CHAPEAU:
		case id_morceau::DE:
		case id_morceau::POINT:
		case id_morceau::EGAL:
		case id_morceau::TROIS_POINTS:
		case id_morceau::VIRGULE:
		case id_morceau::CROCHET_OUVRANT:
			return true;
		default:
			return false;
	}
}

/**
 * Retourne vrai se l'identifiant passé en paramètre peut-être un identifiant
 * valide pour précèder un opérateur unaire '+' ou '-'.
 */
static bool precede_unaire_valide(id_morceau dernier_identifiant)
{
	if (dernier_identifiant == id_morceau::PARENTHESE_FERMANTE) {
		return false;
	}

	if (dernier_identifiant == id_morceau::CROCHET_FERMANT) {
		return false;
	}

	if (dernier_identifiant == id_morceau::CHAINE_CARACTERE) {
		return false;
	}

	if (est_nombre(dernier_identifiant)) {
		return false;
	}

	/* À FAIRE : ceci mélange a[-i] et a[i] - b[i] */
	if (dernier_identifiant == id_morceau::CROCHET_OUVRANT) {
		return false;
	}

	if (dernier_identifiant == id_morceau::CARACTERE) {
		return false;
	}

	if (dernier_identifiant == id_morceau::TRANSTYPE) {
		return false;
	}

	if (dernier_identifiant == id_morceau::MEMOIRE) {
		return false;
	}

	return true;
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(
		ContexteGenerationCode &contexte,
		Fichier *fichier,
		dls::chaine const &racine_kuri)
	: lng::analyseuse<DonneesMorceau>(fichier->morceaux)
	, m_contexte(contexte)
	, m_assembleuse(contexte.assembleuse)
	, m_paires_vecteurs(PROFONDEUR_EXPRESSION_MAX)
	, m_racine_kuri(racine_kuri)
	, m_fichier(fichier)
{}

void analyseuse_grammaire::lance_analyse(std::ostream &os)
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

void analyseuse_grammaire::analyse_corps(std::ostream &os)
{
	while (!fini()) {
		auto id = this->identifiant_courant();

		switch (id) {
			case id_morceau::FONC:
			case id_morceau::COROUT:
			{
				avance();
				analyse_declaration_fonction(id);
				break;
			}
			case id_morceau::STRUCT:
			case id_morceau::UNION:
			{
				avance();
				analyse_declaration_structure(id);
				break;
			}
			case id_morceau::ENUM:
			{
				avance();
				analyse_declaration_enum();
				break;
			}
			case id_morceau::IMPORTE:
			{
				avance();

				if (!est_identifiant(id_morceau::CHAINE_LITTERALE) && !est_identifiant(id_morceau::CHAINE_CARACTERE)) {
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
			case id_morceau::CHARGE:
			{
				avance();

				if (!est_identifiant(id_morceau::CHAINE_LITTERALE) && !est_identifiant(id_morceau::CHAINE_CARACTERE)) {
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
				analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::INCONNU);
				break;
			}
		}
	}
}

void analyseuse_grammaire::analyse_declaration_fonction(id_morceau id)
{
	auto externe = false;

	if (est_identifiant(id_morceau::EXTERNE)) {
		avance();
		externe = true;

		if (id == id_morceau::COROUT && externe) {
			lance_erreur("Une coroutine ne peut pas être externe");
		}
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu la déclaration du nom de la fonction");
	}

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

	if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
	}

	auto donnees_fonctions = DonneesFonction{};
	donnees_fonctions.est_coroutine = (id == id_morceau::COROUT);
	donnees_fonctions.est_externe = externe;
	donnees_fonctions.noeud_decl = noeud;

	/* analyse les paramètres de la fonction */
	m_assembleuse->empile_noeud(type_noeud::LISTE_PARAMETRES_FONCTION, m_contexte, donnees());

	analyse_expression_droite(id_morceau::PARENTHESE_FERMANTE, id_morceau::PARENTHESE_OUVRANTE);

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
		if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
			lance_erreur("Attendu un point-virgule ';' après la déclaration de la fonction externe");
		}

		if (donnees_fonctions.idx_types_retours.taille() > 1) {
			lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
		}
	}
	else {
		/* ignore les points-virgules implicites */
		if (est_identifiant(id_morceau::POINT_VIRGULE)) {
			avance();
		}

		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante après la liste des paramètres de la fonction");
		}

		analyse_bloc();
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_FONCTION);
}

void analyseuse_grammaire::analyse_controle_si(type_noeud tn)
{
	m_assembleuse->empile_noeud(tn, m_contexte, donnees());

	analyse_expression_droite(id_morceau::ACCOLADE_OUVRANTE, id_morceau::SI);

	analyse_bloc();

	if (est_identifiant(id_morceau::SINON)) {
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

		if (est_identifiant(id_morceau::SI)) {
			avance();
			analyse_controle_si(type_noeud::SI);
		}
		else if (est_identifiant(id_morceau::SAUFSI)) {
			avance();
			analyse_controle_si(type_noeud::SAUFSI);
		}
		else {
			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante après 'sinon'");
			}

			analyse_corps_fonction();

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante à la fin du contrôle 'sinon'");
			}
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
void analyseuse_grammaire::analyse_controle_pour()
{
	m_assembleuse->empile_noeud(type_noeud::POUR, m_contexte, donnees());

	/* enfant 1 : déclaration variable */

	analyse_expression_droite(id_morceau::DANS, id_morceau::POUR);

	/* enfant 2 : expr */

	analyse_expression_droite(id_morceau::ACCOLADE_OUVRANTE, id_morceau::DANS);

	recule();

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'pour'");
	}

	/* enfant 3 : bloc */
	analyse_bloc();

	/* enfant 4 : bloc sansarrêt (optionel) */
	if (est_identifiant(id_morceau::SANSARRET)) {
		avance();

		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'sansarrêt'");
		}

		analyse_bloc();
	}

	/* enfant 4 ou 5 : bloc sinon (optionel) */
	if (est_identifiant(id_morceau::SINON)) {
		avance();

		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");
		}

		analyse_bloc();
	}

	m_assembleuse->depile_noeud(type_noeud::POUR);
}

void analyseuse_grammaire::analyse_corps_fonction()
{
	/* Il est possible qu'une fonction soit vide, donc vérifie d'abord que
	 * l'on n'ait pas terminé. */
	while (!est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		auto const pos = position();

		if (est_identifiant(id_morceau::RETOURNE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETOUR, m_contexte, donnees());

			/* Considération du cas où l'on ne retourne rien 'retourne;'. */
			if (!est_identifiant(id_morceau::POINT_VIRGULE)) {
				analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::RETOURNE);
			}
			else {
				avance();
			}

			m_assembleuse->depile_noeud(type_noeud::RETOUR);
		}
		else if (est_identifiant(id_morceau::RETIENS)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETIENS, m_contexte, donnees());

			analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::RETOURNE);

			m_assembleuse->depile_noeud(type_noeud::RETIENS);
		}
		else if (est_identifiant(id_morceau::POUR)) {
			avance();
			analyse_controle_pour();
		}
		else if (est_identifiant(id_morceau::BOUCLE)) {
			avance();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'boucle'");
			}

			m_assembleuse->empile_noeud(type_noeud::BOUCLE, m_contexte, donnees());

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::BOUCLE);
		}
		else if (est_identifiant(id_morceau::REPETE)) {
			avance();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'répète'");
			}

			m_assembleuse->empile_noeud(type_noeud::REPETE, m_contexte, donnees());

			analyse_bloc();

			if (!requiers_identifiant(id_morceau::TANTQUE)) {
				lance_erreur("Attendu une 'tantque' après le bloc de 'répète'");
			}

			analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::TANTQUE);

			m_assembleuse->depile_noeud(type_noeud::REPETE);
		}
		else if (est_identifiant(id_morceau::TANTQUE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::TANTQUE, m_contexte, donnees());

			analyse_expression_droite(type_id::ACCOLADE_OUVRANTE, type_id::TANTQUE);

			/* recule pour être de nouveau synchronisé */
			recule();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après l'expression de 'tanque'");
			}

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::TANTQUE);
		}
		else if (est_identifiant(id_morceau::ARRETE) || est_identifiant(id_morceau::CONTINUE)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::CONTINUE_ARRETE, m_contexte, donnees());

			if (est_identifiant(id_morceau::CHAINE_CARACTERE)) {
				avance();
				m_assembleuse->empile_noeud(type_noeud::VARIABLE, m_contexte, donnees());
				m_assembleuse->depile_noeud(type_noeud::VARIABLE);
			}

			m_assembleuse->depile_noeud(type_noeud::CONTINUE_ARRETE);

			if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
				lance_erreur("Attendu un point virgule ';'");
			}
		}
		else if (est_identifiant(id_morceau::DIFFERE)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::DIFFERE, m_contexte, donnees());

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'diffère'");
			}

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::DIFFERE);
		}
		else if (est_identifiant(id_morceau::NONSUR)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::NONSUR, m_contexte, donnees());

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'nonsûr'");
			}

			analyse_bloc();

			m_assembleuse->depile_noeud(type_noeud::NONSUR);
		}
		else if (est_identifiant(id_morceau::ASSOCIE)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::ASSOCIE, m_contexte, donnees());

			analyse_expression_droite(type_id::ACCOLADE_OUVRANTE, type_id::ASSOCIE);

			/* recule pour être de nouveau synchronisé */
			recule();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après l'expression de 'associe'");
			}

			while (true) {
				if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
					/* nous avons terminé */
					break;
				}

				m_assembleuse->empile_noeud(type_noeud::PAIRE_ASSOCIATION, m_contexte, donnees());

				analyse_expression_droite(type_id::ACCOLADE_OUVRANTE, type_id::ASSOCIE);

				/* recule pour être de nouveau synchronisé */
				recule();

				if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
					lance_erreur("Attendu une accolade ouvrante '{' après l'expression de 'associe'");
				}

				analyse_bloc();

				m_assembleuse->depile_noeud(type_noeud::PAIRE_ASSOCIATION);
			}

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'associe'");
			}

			m_assembleuse->depile_noeud(type_noeud::ASSOCIE);
		}
		else if (est_identifiant(type_id::ACCOLADE_OUVRANTE)) {
			avance();
			analyse_bloc();
		}
		else {
			analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::EGAL);
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

void analyseuse_grammaire::analyse_bloc()
{
	m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
	analyse_corps_fonction();
	m_assembleuse->depile_noeud(type_noeud::BLOC);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante '}' à la fin du bloc");
	}
}

noeud::base *analyseuse_grammaire::analyse_expression_droite(
		id_morceau identifiant_final,
		id_morceau racine_expr,
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

	auto vide_pile_operateur = [&](id_morceau id_operateur)
	{
		while (!pile.est_vide()
			   && pile.back() != NOEUD_PARENTHESE
			   && (precedence_faible(id_operateur, pile.back()->identifiant())))
		{
			expression.pousse(pile.back());
			pile.pop_back();
		}
	};

	/* Nous tenons compte du nombre de paranthèse pour pouvoir nous arrêter en
	 * cas d'analyse d'une expression en dernier paramètre d'un appel de
	 * fontion. */
	auto paren = 0;
	auto dernier_identifiant = (m_position == 0) ? id_morceau::INCONNU : donnees().identifiant;

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	auto assignation = false;

	auto drapeaux = drapeaux_noeud::AUCUN;

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

	while (!requiers_identifiant(identifiant_final)) {
		auto &morceau = donnees();

		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(morceau.identifiant) << FIN_LOG_EXPRESSION;

		switch (morceau.identifiant) {
			case id_morceau::SOIT:
			{
				drapeaux |= DECLARATION;
				break;
			}
			case id_morceau::DYN:
			{
				drapeaux |= (DYNAMIC | DECLARATION);
				break;
			}
			case id_morceau::EMPL:
			{
				drapeaux |= EMPLOYE;
				break;
			}
			case id_morceau::CHAINE_CARACTERE:
			{
				/* appel fonction : chaine + ( */
				if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					avance();

					auto noeud = m_assembleuse->empile_noeud(type_noeud::APPEL_FONCTION, m_contexte, morceau, false);

					analyse_appel_fonction(noeud);

					m_assembleuse->depile_noeud(type_noeud::APPEL_FONCTION);

					expression.pousse(noeud);
				}
				/* construction structure : chaine + { */
				else if (racine_expr == id_morceau::EGAL && est_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
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
					if ((racine_expr != type_id::TRANSTYPE && racine_expr != type_id::LOGE && racine_expr != type_id::RELOGE) && est_identifiant(id_morceau::DOUBLE_POINTS)) {
						noeud->type_declare = analyse_declaration_type();
					}
				}

				break;
			}
			case id_morceau::NOMBRE_REEL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_REEL, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::NOMBRE_BINAIRE:
			case id_morceau::NOMBRE_ENTIER:
			case id_morceau::NOMBRE_HEXADECIMAL:
			case id_morceau::NOMBRE_OCTAL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_ENTIER, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::CHAINE_LITTERALE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CHAINE_LITTERALE, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::CARACTERE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CARACTERE, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::NUL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NUL, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::TAILLE_DE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'taille_de'");
				}

				auto noeud = m_assembleuse->cree_noeud(type_noeud::TAILLE_DE, m_contexte, morceau);
				noeud->valeur_calculee = analyse_declaration_type(false);

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après le type de 'taille_de'");
				}

				expression.pousse(noeud);
				break;
			}
			case id_morceau::INFO_DE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'info_de'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::INFO_DE, m_contexte, morceau, false);

				analyse_expression_droite(id_morceau::INCONNU, id_morceau::INFO_DE);

				m_assembleuse->depile_noeud(type_noeud::INFO_DE);

				/* vérifie mais n'avance pas */
				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après l'expression de 'taille_de'");
				}

				expression.pousse(noeud);
				break;
			}
			case id_morceau::VRAI:
			case id_morceau::FAUX:
			{
				/* remplace l'identifiant par id_morceau::BOOL */
				morceau.identifiant = id_morceau::BOOL;
				auto noeud = m_assembleuse->cree_noeud(type_noeud::BOOLEEN, m_contexte, morceau);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::TRANSTYPE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'transtype'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::TRANSTYPE, m_contexte, morceau, false);

				analyse_expression_droite(id_morceau::DOUBLE_POINTS, id_morceau::TRANSTYPE);

				noeud->type_declare = analyse_declaration_type(false);

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après la déclaration du type");
				}

				m_assembleuse->depile_noeud(type_noeud::TRANSTYPE);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::MEMOIRE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'mémoire'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::MEMOIRE, m_contexte, morceau, false);

				analyse_expression_droite(id_morceau::INCONNU, id_morceau::MEMOIRE);

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après l'expression");
				}

				m_assembleuse->depile_noeud(type_noeud::MEMOIRE);
				expression.pousse(noeud);
				break;
			}
			case id_morceau::PARENTHESE_OUVRANTE:
			{
				++paren;
				pile.pousse(NOEUD_PARENTHESE);
				break;
			}
			case id_morceau::PARENTHESE_FERMANTE:
			{
				/* S'il n'y a pas de parenthèse ouvrante, c'est que nous avons
				 * atteint la fin d'une déclaration d'appel de fonction. */
				if (paren == 0) {
					/* recule pour être synchroniser avec la sortie dans
					 * analyse_appel_fonction() */
					recule();

					termine_boucle = true;
					break;
				}

				if (pile.est_vide()) {
					lance_erreur("Il manque une paranthèse dans l'expression !");
				}

				while (pile.back() != NOEUD_PARENTHESE) {
					expression.pousse(pile.back());
					pile.pop_back();
				}

				/* Enlève la parenthèse restante de la pile. */
				pile.pop_back();

				--paren;
				break;
			}
			/* opérations binaire */
			case id_morceau::PLUS:
			case id_morceau::MOINS:
			{
				auto id_operateur = morceau.identifiant;
				auto noeud = static_cast<noeud::base *>(nullptr);

				if (precede_unaire_valide(dernier_identifiant)) {
					if (id_operateur == id_morceau::PLUS) {
						id_operateur = id_morceau::PLUS_UNAIRE;
					}
					else if (id_operateur == id_morceau::MOINS) {
						id_operateur = id_morceau::MOINS_UNAIRE;
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
			case id_morceau::FOIS:
			case id_morceau::DIVISE:
			case id_morceau::ESPERLUETTE:
			case id_morceau::POURCENT:
			case id_morceau::INFERIEUR:
			case id_morceau::INFERIEUR_EGAL:
			case id_morceau::SUPERIEUR:
			case id_morceau::SUPERIEUR_EGAL:
			case id_morceau::DECALAGE_DROITE:
			case id_morceau::DECALAGE_GAUCHE:
			case id_morceau::DIFFERENCE:
			case id_morceau::ESP_ESP:
			case id_morceau::EGALITE:
			case id_morceau::BARRE_BARRE:
			case id_morceau::BARRE:
			case id_morceau::CHAPEAU:
			case id_morceau::PLUS_EGAL:
			case id_morceau::MOINS_EGAL:
			case id_morceau::DIVISE_EGAL:
			case id_morceau::MULTIPLIE_EGAL:
			case id_morceau::MODULO_EGAL:
			case id_morceau::ET_EGAL:
			case id_morceau::OU_EGAL:
			case id_morceau::OUX_EGAL:
			case id_morceau::DEC_DROITE_EGAL:
			case id_morceau::DEC_GAUCHE_EGAL:
			case id_morceau::VIRGULE:
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
			case id_morceau::TROIS_POINTS:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::PLAGE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case id_morceau::DE:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE_DE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case id_morceau::POINT:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE_POINT, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case id_morceau::EGAL:
			{
				if (assignation) {
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				vide_pile_operateur(morceau.identifiant);

				auto noeud = m_assembleuse->cree_noeud(type_noeud::ASSIGNATION_VARIABLE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case id_morceau::CROCHET_OUVRANT:
			{
				/* l'accès à un élément d'un tableau est chaine[index] */
				if (dernier_identifiant == id_morceau::CHAINE_CARACTERE
						|| dernier_identifiant == id_morceau::CHAINE_LITTERALE
						 || dernier_identifiant == id_morceau::CROCHET_OUVRANT) {
					vide_pile_operateur(morceau.identifiant);

					auto noeud = m_assembleuse->empile_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau, false);
					pile.pousse(noeud);

					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT);

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
					morceau.identifiant = id_morceau::TABLEAU;
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_TABLEAU, m_contexte, morceau, false);

					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT);

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_TABLEAU);

					expression.pousse(noeud);
				}

				break;
			}
			/* opérations unaire */
			case id_morceau::AROBASE:
			case id_morceau::EXCLAMATION:
			case id_morceau::TILDE:
			case id_morceau::PLUS_UNAIRE:
			case id_morceau::MOINS_UNAIRE:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
				pile.pousse(noeud);
				break;
			}
			case id_morceau::ACCOLADE_FERMANTE:
			{
				/* une accolade fermante marque généralement la fin de la
				 * construction d'une structure */
				termine_boucle = true;
				/* recule pour être synchroniser avec la sortie dans
				 * analyse_construction_structure() */
				recule();
				break;
			}
			case id_morceau::LOGE:
			{
				auto noeud = m_assembleuse->empile_noeud(
							type_noeud::LOGE,
							m_contexte,
							morceau,
							false);

				if (est_identifiant(id_morceau::CHAINE)) {
					noeud->type_declare = analyse_declaration_type(false);

					avance();

					analyse_expression_droite(type_id::SINON, type_id::LOGE);

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
			case id_morceau::RELOGE:
			{
				/* reloge nom : type; */
				auto noeud_reloge = m_assembleuse->empile_noeud(
							type_noeud::RELOGE,
							m_contexte,
							morceau,
							false);

				analyse_expression_droite(type_id::DOUBLE_POINTS, type_id::RELOGE);

				if (est_identifiant(type_id::CHAINE)) {
					noeud_reloge->type_declare = analyse_declaration_type(false);

					avance();

					analyse_expression_droite(type_id::SINON, type_id::RELOGE);

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
			case id_morceau::DELOGE:
			{
				auto noeud = m_assembleuse->empile_noeud(
							type_noeud::DELOGE,
							m_contexte,
							morceau,
							false);

				analyse_expression_droite(type_id::POINT_VIRGULE, type_id::DELOGE);

				/* besoin de reculer car l'analyse va jusqu'au point-virgule, ce
				 * qui nous fait absorber le code de l'expression suivante */
				recule();

				m_assembleuse->depile_noeud(type_noeud::DELOGE);

				expression.pousse(noeud);

				break;
			}
			case id_morceau::DIRECTIVE:
			{
				if (est_identifiant(id_morceau::CHAINE_CARACTERE)) {
					avance();

					auto directive = donnees().chaine;

					if (directive == "inclus") {
						if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
							lance_erreur("Attendu une chaine littérale après la directive");
						}

						auto chaine = donnees().chaine;
						m_assembleuse->inclusions.pousse(chaine);
					}
					else if (directive == "bib") {
						if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
							lance_erreur("Attendu une chaine littérale après la directive");
						}

						auto chaine = donnees().chaine;
						m_assembleuse->bibliotheques.pousse(chaine);
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
				else if (est_identifiant(id_morceau::SI)) {
					analyse_directive_si();
				}
				else if (est_identifiant(id_morceau::SINON)) {
					avance();

					if (est_identifiant(id_morceau::SI)) {
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
			case id_morceau::SI:
			{
				analyse_controle_si(type_noeud::SI);
				termine_boucle = true;
				break;
			}
			case id_morceau::SAUFSI:
			{
				analyse_controle_si(type_noeud::SAUFSI);
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

		dernier_identifiant = morceau.identifiant;
	}

	/* Retourne s'il n'y a rien dans l'expression, ceci est principalement pour
	 * éviter de crasher lors des fuzz-tests. */
	if (expression.est_vide()) {
		--m_profondeur;
		return nullptr;
	}

	while (!pile.est_vide()) {
		if (pile.back() == NOEUD_PARENTHESE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

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

void analyseuse_grammaire::analyse_appel_fonction(noeud::base *noeud)
{
	/* ici nous devons être au niveau du premier paramètre */

	while (true) {
		/* aucun paramètre, ou la liste de paramètre est vide */
		if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
			avance();
			return;
		}

		if (sont_2_identifiants(id_morceau::CHAINE_CARACTERE, id_morceau::EGAL)) {
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
		analyse_expression_droite(id_morceau::VIRGULE, id_morceau::EGAL);
	}
}

void analyseuse_grammaire::analyse_declaration_structure(id_morceau id)
{
	auto est_externe = false;
	auto est_nonsur = false;

	if (est_identifiant(type_id::EXTERNE)) {
		est_externe = true;
		avance();
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaine de caractères après 'struct'");
	}

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
	donnees_structure.est_union = (id == id_morceau::UNION);
	donnees_structure.est_nonsur = est_nonsur;

	m_contexte.ajoute_donnees_structure(nom_structure, donnees_structure);

	auto analyse_membres = true;

	if (est_externe) {
		if (est_identifiant(type_id::POINT_VIRGULE)) {
			avance();
			analyse_membres = false;
		}
	}

	if (analyse_membres) {
		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu '{' après le nom de la structure");
		}

		while (true) {
			if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				/* nous avons terminé */
				break;
			}

			analyse_expression_droite(id_morceau::POINT_VIRGULE, type_id::STRUCT);
		}

		if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu '}' à la fin de la déclaration de la structure");
		}
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_STRUCTURE);
}

void analyseuse_grammaire::analyse_declaration_enum()
{
	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu un nom après 'énum'");
	}

	auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_ENUM, m_contexte, donnees());
	auto nom = noeud_decl->morceau.chaine;

	auto donnees_structure = DonneesStructure{};
	donnees_structure.est_enum = true;
	donnees_structure.noeud_decl = noeud_decl;

	m_contexte.ajoute_donnees_structure(nom, donnees_structure);

	noeud_decl->type_declare = analyse_declaration_type();

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{' après 'énum'");
	}

	while (true) {
		if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			/* nous avons terminé */
			break;
		}

		analyse_expression_droite(id_morceau::VIRGULE, id_morceau::EGAL);
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de l'énum");
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_ENUM);
}

DonneesTypeDeclare analyseuse_grammaire::analyse_declaration_type(bool double_point)
{
	if (double_point && !requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':'");
	}

	/* Vérifie si l'on a un pointeur vers une fonction. */
	if (est_identifiant(id_morceau::FONC) || est_identifiant(id_morceau::COROUT)) {
		avance();

		auto dt = DonneesTypeDeclare{};
		dt.pousse(donnees().identifiant);

		if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
			lance_erreur("Attendu un '(' après 'fonction'");
		}

		dt.pousse(id_morceau::PARENTHESE_OUVRANTE);

		while (true) {
			if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
				break;
			}

			auto dtd = analyse_declaration_type(false);
			dt.pousse(dtd);

			if (!est_identifiant(id_morceau::VIRGULE)) {
				break;
			}

			avance();
			dt.pousse(id_morceau::VIRGULE);
		}

		avance();
		dt.pousse(id_morceau::PARENTHESE_FERMANTE);

		bool eu_paren_ouvrante = false;

		if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
			avance();
			eu_paren_ouvrante = true;
		}

		dt.pousse(id_morceau::PARENTHESE_OUVRANTE);

		while (true) {
			if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
				break;
			}

			auto dtd = analyse_declaration_type(false);
			dt.pousse(dtd);

			auto est_virgule = est_identifiant(id_morceau::VIRGULE);

			if ((est_virgule && !eu_paren_ouvrante) || !est_virgule) {
				break;
			}

			avance();
			dt.pousse(id_morceau::VIRGULE);
		}

		if (eu_paren_ouvrante && est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
			avance();
		}

		dt.pousse(id_morceau::PARENTHESE_FERMANTE);

		return dt;
	}

	return analyse_declaration_type_ex();
}

DonneesTypeDeclare analyseuse_grammaire::analyse_declaration_type_ex()
{
	auto dernier_id = id_morceau{};
	auto donnees_type = DonneesTypeDeclare{};

	while (est_specifiant_type(identifiant_courant())) {
		auto id = this->identifiant_courant();
		avance();

		switch (id) {
			case type_id::CROCHET_OUVRANT:
			{
				auto expr = static_cast<noeud::base *>(nullptr);

				if (this->identifiant_courant() != id_morceau::CROCHET_FERMANT) {
					expr = analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT, false);
				}
				else {
					avance();
				}

				donnees_type.pousse(id_morceau::TABLEAU);
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
				donnees_type.pousse(id_morceau::POINTEUR);
				break;
			}
			case type_id::ESPERLUETTE:
			{
				donnees_type.pousse(id_morceau::REFERENCE);
				break;
			}
			case type_id::TYPE_DE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu un '(' après 'type_de'");
				}

				auto expr = analyse_expression_droite(
							id_morceau::PARENTHESE_FERMANTE,
							id_morceau::TYPE_DE,
							false);

				donnees_type.pousse(id_morceau::TYPE_DE);
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

	if (dernier_id == id_morceau::TYPE_DE) {
		type_attendu = false;
	}

	/* Soutiens pour les types des fonctions variadiques externes. */
	if (dernier_id == id_morceau::TROIS_POINTS && est_identifiant(type_id::PARENTHESE_FERMANTE)) {
		type_attendu = false;
	}

	if (type_attendu) {
		if (!requiers_identifiant_type()) {
			lance_erreur("Attendu la déclaration d'un type");
		}

		auto identifiant = donnees().identifiant;

		if (identifiant == id_morceau::CHAINE_CARACTERE) {
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

void analyseuse_grammaire::analyse_construction_structure(noeud::base *noeud)
{
	auto liste_param = dls::tableau<dls::vue_chaine_compacte>{};

	/* ici nous devons être au niveau du premier paramètre */
	while (true) {
		if (est_identifiant(type_id::ACCOLADE_FERMANTE)) {
			avance();
			noeud->drapeaux |= EST_CALCULE;
			noeud->valeur_calculee = liste_param;
			return;
		}

		if (!sont_2_identifiants(type_id::CHAINE_CARACTERE, type_id::EGAL)) {
			lance_erreur(
						"Le nom des membres est requis pour la construction de la structure",
						erreur::type_erreur::MEMBRE_INCONNU);
		}

		avance();

		auto nom = donnees().chaine;
		liste_param.pousse(nom);

		avance();

		analyse_expression_droite(id_morceau::VIRGULE, id_morceau::EGAL);
	}
}

void analyseuse_grammaire::analyse_directive_si()
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

bool analyseuse_grammaire::requiers_identifiant_type()
{
	auto const ok = est_identifiant_type(this->identifiant_courant());
	avance();
	return ok;
}

bool analyseuse_grammaire::requiers_nombre_entier()
{
	auto const ok = est_nombre_entier(this->identifiant_courant());
	avance();
	return ok;
}

void analyseuse_grammaire::lance_erreur(const dls::chaine &quoi, erreur::type_erreur type)
{
	erreur::lance_erreur(quoi, m_contexte, donnees(), type);
}
