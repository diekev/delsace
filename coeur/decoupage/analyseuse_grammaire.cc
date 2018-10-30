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

#include <iostream>
#include <set>

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "expression.h"
#include "nombres.h"

/**
 * Limitation du nombre récursif de sous-expressions (par exemple :
 * f(g(h(i(j()))))).
 */
static constexpr auto PROFONDEUR_EXPRESSION_MAX = 32;

/* À FAIRE :
 * - gabarit
 */

static bool est_specifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::FOIS:
		case id_morceau::ESPERLUETTE:
		case id_morceau::CROCHET_OUVRANT:
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
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::BOOL:
		case id_morceau::RIEN:
		case id_morceau::CHAINE_CARACTERE:
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
		case id_morceau::EGAL:
			return true;
		default:
			return false;
	}
}

static bool est_operateur_constant(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
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
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
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

	return true;
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(
		ContexteGenerationCode &contexte,
		const std::vector<DonneesMorceaux> &identifiants,
		const TamponSource &tampon,
		assembleuse_arbre *assembleuse)
	: Analyseuse(identifiants, tampon)
	, m_assembleuse(assembleuse)
	, m_contexte(contexte)
	, m_paires_vecteurs(PROFONDEUR_EXPRESSION_MAX)
{}

void analyseuse_grammaire::lance_analyse()
{
	m_position = 0;

	if (m_identifiants.size() == 0) {
		return;
	}

	m_assembleuse->empile_noeud(type_noeud::RACINE, DonneesMorceaux{"racine", 0ul, id_morceau::INCONNU });

	analyse_corps();
}

void analyseuse_grammaire::analyse_corps()
{
	while (m_position != m_identifiants.size()) {
		if (est_identifiant(id_morceau::FONCTION)) {
			analyse_declaration_fonction();
		}
		else if (est_identifiant(id_morceau::SOIT)) {
			analyse_declaration_constante();
		}
		else if (est_identifiant(id_morceau::STRUCTURE)) {
			analyse_declaration_structure();
		}
		else if (est_identifiant(id_morceau::ENUM)) {
			analyse_declaration_enum();
		}
		else {
			avance();
			lance_erreur("Identifiant inattendu, doit être 'soit', 'fonction', 'structure', ou 'énum'");
		}
	}
}

void analyseuse_grammaire::analyse_declaration_fonction()
{
	if (!requiers_identifiant(id_morceau::FONCTION)) {
		lance_erreur("Attendu la déclaration du mot-clé 'fonction'");
	}

	auto externe = false;

	if (est_identifiant(id_morceau::EXTERNE)) {
		avance();
		externe = true;
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu la déclaration du nom de la fonction");
	}

	// crée noeud fonction
	const auto nom_fonction = m_identifiants[position()].chaine;

	if (m_contexte.fonction_existe(nom_fonction)) {
		lance_erreur("Redéfinition de la fonction", erreur::type_erreur::FONCTION_REDEFINIE);
	}

	auto noeud = m_assembleuse->empile_noeud(type_noeud::DECLARATION_FONCTION, m_identifiants[position()]);
	auto noeud_declaration = dynamic_cast<NoeudDeclarationFonction *>(noeud);
	noeud_declaration->est_externe = externe;

	if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
	}

	auto donnees_fonctions = DonneesFonction{};

	analyse_parametres_fonction(noeud_declaration, donnees_fonctions);

	if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une parenthèse fermante après la liste des paramètres de la fonction");
	}

	/* À FAIRE : inférence de type retour. */
	analyse_declaration_type(noeud_declaration->donnees_type);
	donnees_fonctions.donnees_type = noeud_declaration->donnees_type;

	m_contexte.ajoute_donnees_fonctions(nom_fonction, donnees_fonctions);

	if (externe) {
		if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
			lance_erreur("Attendu un point-virgule ';' après la déclaration de la fonction externe");
		}
	}
	else {
		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante après la liste des paramètres de la fonction");
		}

		m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);
		analyse_corps_fonction();
		m_assembleuse->depile_noeud(type_noeud::BLOC);

		if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante à la fin de la fonction");
		}
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_FONCTION);
}

void analyseuse_grammaire::analyse_parametres_fonction(NoeudDeclarationFonction *noeud, DonneesFonction &donnees)
{
	if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		/* La liste est vide. */
		return;
	}

	ArgumentFonction arg;

	if (est_identifiant(id_morceau::VARIABLE)) {
		arg.est_variable = true;
		avance();
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu le nom de la variable");
	}

	arg.chaine = m_identifiants[position()].chaine;

	if (donnees.args.find(arg.chaine) != donnees.args.end()) {
		lance_erreur("Redéfinition de l'argument", erreur::type_erreur::ARGUMENT_REDEFINI);
	}

	if (!requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':' après le nom de l'argument");
	}

	if (est_identifiant(id_morceau::TROIS_POINTS)) {
		avance();

		if (!noeud->est_externe) {
			lance_erreur("La déclaration de fonction variadique n'est"
						 " implémentée que pour les fonctions externes");
		}

		noeud->est_variable = true;
	}

	auto donnees_type = DonneesType{};

	if (!noeud->est_variable || !est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		analyse_declaration_type(donnees_type, false);
	}

	arg.donnees_type = donnees_type;

	DonneesArgument donnees_arg;
	donnees_arg.index = donnees.args.size();
	donnees_arg.donnees_type = donnees_type;
	/* doit être vrai uniquement pour le dernier argument */
	donnees_arg.est_variadic = noeud->est_variable;

	donnees.args.insert({arg.chaine, donnees_arg});

	noeud->ajoute_argument(arg);

	/* fin des paramètres */
	if (!requiers_identifiant(id_morceau::VIRGULE)) {
		recule();
		return;
	}

	if (!noeud->est_variable) {
		analyse_parametres_fonction(noeud, donnees);
	}
}

void analyseuse_grammaire::analyse_controle_si()
{
	if (!requiers_identifiant(id_morceau::SI)) {
		lance_erreur("Attendu la déclaration 'si'");
	}

	m_assembleuse->empile_noeud(type_noeud::SI, m_identifiants[position()]);

	analyse_expression_droite(id_morceau::ACCOLADE_OUVRANTE);

	m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

	analyse_corps_fonction();

	m_assembleuse->depile_noeud(type_noeud::BLOC);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin du contrôle 'si'");
	}

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
		m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

		if (est_identifiant(id_morceau::SI)) {
			analyse_controle_si();
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

	m_assembleuse->depile_noeud(type_noeud::SI);
}

/* Arbre :
 * NoeudPour
 * - enfant 1 : déclaration variable
 * - enfant 2 : expr début
 * - enfant 3 : expr fin
 * - enfant 4 : bloc
 * - enfant 5 : bloc sansarrêt ou sinon
 * - enfant 6 : bloc sinon
 */
void analyseuse_grammaire::analyse_controle_pour()
{
	if (!requiers_identifiant(id_morceau::POUR)) {
		lance_erreur("Attendu la déclaration 'pour'");
	}

	m_assembleuse->empile_noeud(type_noeud::POUR, m_identifiants[position()]);

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'pour'");
	}

	/* enfant 1 : déclaration variable */

	auto noeud = m_assembleuse->cree_noeud(type_noeud::DECLARATION_VARIABLE, m_identifiants[position()]);
	m_assembleuse->ajoute_noeud(noeud);

	if (!requiers_identifiant(id_morceau::DANS)) {
		lance_erreur("Attendu le mot 'dans' après la chaîne de caractère");
	}

	/* enfant 2 : expr début */

	analyse_expression_droite(id_morceau::TROIS_POINTS);

	/* enfant 3 : expr fin */

	analyse_expression_droite(id_morceau::ACCOLADE_OUVRANTE);

	recule();

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'pour'");
	}

	/* enfant 4 : bloc */

	m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

	analyse_corps_fonction();

	m_assembleuse->depile_noeud(type_noeud::BLOC);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'pour'");
	}

	/* enfant 5 : bloc sansarrêt (optionel) */
	if (est_identifiant(id_morceau::SANSARRET)) {
		avance();

		m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");
		}

		analyse_corps_fonction();

		m_assembleuse->depile_noeud(type_noeud::BLOC);

		if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'sinon'");
		}
	}

	/* enfant 5 ou 6 : bloc sinon (optionel) */
	if (est_identifiant(id_morceau::SINON)) {
		avance();

		m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");
		}

		analyse_corps_fonction();

		m_assembleuse->depile_noeud(type_noeud::BLOC);

		if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'sinon'");
		}
	}

	m_assembleuse->depile_noeud(type_noeud::POUR);
}

void analyseuse_grammaire::analyse_corps_fonction()
{
	/* Il est possible qu'une fonction soit vide, donc vérifie d'abord que
	 * l'on n'ait pas terminé. */
	while (!est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		const auto pos = position();

		/* assignement : soit x = a + b; */
		if (est_identifiant(id_morceau::SOIT)) {
			avance();

			auto est_variable = false;

			if (est_identifiant(id_morceau::VARIABLE)) {
				est_variable = true;
				avance();
			}

			if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaîne de caractère après 'soit'");
			}

			const auto &morceau_variable = m_identifiants[position()];
			auto donnees_type = DonneesType{};

			if (est_identifiant(id_morceau::DOUBLE_POINTS)) {
				analyse_declaration_type(donnees_type);
			}

			/* À FAIRE : ceci est principalement pour pouvoir déclarer des
			 * structures ou des tableaux en attendant de pouvoir les initialiser
			 * par une assignation directe. par exemple : soit x = Vecteur3D(); */
			if (!est_identifiant(id_morceau::EGAL)) {
				if (!est_variable) {
					avance();
					lance_erreur("Attendu '=' après chaîne de caractère");
				}

				auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_VARIABLE, morceau_variable);
				noeud_decl->donnees_type = donnees_type;
				noeud_decl->est_variable = est_variable;
				m_assembleuse->depile_noeud(type_noeud::DECLARATION_VARIABLE);

				if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
					lance_erreur("Attendu ';' à la fin de la déclaration de la variable");
				}
			}
			else {
				avance();

				const auto &morceau_egal = m_identifiants[position()];

				auto noeud = m_assembleuse->empile_noeud(type_noeud::ASSIGNATION_VARIABLE, morceau_egal);
				noeud->donnees_type = donnees_type;

				auto noeud_decl = m_assembleuse->cree_noeud(type_noeud::DECLARATION_VARIABLE, morceau_variable);
				noeud_decl->donnees_type = donnees_type;
				noeud_decl->est_variable = est_variable;
				noeud->ajoute_noeud(noeud_decl);

				analyse_expression_droite(id_morceau::POINT_VIRGULE);

				m_assembleuse->depile_noeud(type_noeud::ASSIGNATION_VARIABLE);
			}
		}
		/* retour : retourne a + b; */
		else if (est_identifiant(id_morceau::RETOURNE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETOUR, m_identifiants[position()]);

			/* Considération du cas où l'on ne retourne rien 'retourne;'. */
			if (!est_identifiant(id_morceau::POINT_VIRGULE)) {
				analyse_expression_droite(id_morceau::POINT_VIRGULE);
			}
			else {
				avance();
			}

			if (!est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante après l'expression de retour.");
			}

			m_assembleuse->depile_noeud(type_noeud::RETOUR);
		}
		/* controle de flux : si */
		else if (est_identifiant(id_morceau::SI)) {
			analyse_controle_si();
		}
		else if (est_identifiant(id_morceau::POUR)) {
			analyse_controle_pour();
		}
		else if (est_identifiant(id_morceau::BOUCLE)) {
			avance();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'boucle'");
			}

			m_assembleuse->empile_noeud(type_noeud::BOUCLE, m_identifiants[position()]);
			m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);
			analyse_corps_fonction();
			m_assembleuse->depile_noeud(type_noeud::BLOC);

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'boucle'");
			}

			/* enfant 2 : bloc sinon (optionel) */
			if (est_identifiant(id_morceau::SINON)) {
				avance();

				if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
					lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");
				}

				m_assembleuse->empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

				analyse_corps_fonction();

				m_assembleuse->depile_noeud(type_noeud::BLOC);

				if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
					lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'sinon'");
				}
			}

			m_assembleuse->depile_noeud(type_noeud::BOUCLE);
		}
		else if (est_identifiant(id_morceau::ARRETE) || est_identifiant(id_morceau::CONTINUE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::CONTINUE_ARRETE, m_identifiants[position()]);
			m_assembleuse->depile_noeud(type_noeud::CONTINUE_ARRETE);

			if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
				lance_erreur("Attendu un point virgule ';'");
			}
		}
		/* appel : fais_quelque_chose(); */
		else if (sont_2_identifiants(id_morceau::CHAINE_CARACTERE, id_morceau::PARENTHESE_OUVRANTE)) {
			analyse_expression_droite(id_morceau::POINT_VIRGULE);
		}
		else {
			analyse_expression_droite(id_morceau::POINT_VIRGULE, false, true);
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

static auto NOEUD_PARENTHESE = reinterpret_cast<Noeud *>(id_morceau::PARENTHESE_OUVRANTE);

void analyseuse_grammaire::analyse_expression_droite(id_morceau identifiant_final, const bool calcul_expression, const bool assignation)
{
	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */

	if (m_profondeur >= m_paires_vecteurs.size()) {
		lance_erreur("Excès de la pile d'expression autorisée");
	}

	std::vector<Noeud *> &expression = m_paires_vecteurs[m_profondeur].first;
	expression.clear();

	std::vector<Noeud *> &pile = m_paires_vecteurs[m_profondeur].second;
	pile.clear();

	auto vide_pile_operateur = [&](id_morceau id_operateur)
	{
		while (!pile.empty()
			   && pile.back() != NOEUD_PARENTHESE
			   && (precedence_faible(id_operateur, pile.back()->identifiant())))
		{
			expression.push_back(pile.back());
			pile.pop_back();
		}
	};

	/* Nous tenons compte du nombre de paranthèse pour pouvoir nous arrêter en
	 * cas d'analyse d'une expression en dernier paramètre d'un appel de
	 * fontion. */
	auto paren = 0;
	auto dernier_identifiant = m_identifiants[position()].identifiant;

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	while (!est_identifiant(identifiant_final)) {
		const auto &morceau = m_identifiants[position() + 1];

		switch (morceau.identifiant) {
			case id_morceau::CHAINE_CARACTERE:
			{
				avance();

				/* appel fonction : chaine + ( */
				if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					avance();

					auto noeud = m_assembleuse->empile_noeud(type_noeud::APPEL_FONCTION, morceau, false);

					analyse_appel_fonction(dynamic_cast<NoeudAppelFonction *>(noeud));

					m_assembleuse->depile_noeud(type_noeud::APPEL_FONCTION);

					expression.push_back(noeud);
				}
				/* variable : chaine */
				else {
					recule();

					auto noeud = m_assembleuse->cree_noeud(type_noeud::VARIABLE, morceau);
					expression.push_back(noeud);
				}

				break;
			}
			case id_morceau::NOMBRE_REEL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_REEL, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::NOMBRE_BINAIRE:
			case id_morceau::NOMBRE_ENTIER:
			case id_morceau::NOMBRE_HEXADECIMAL:
			case id_morceau::NOMBRE_OCTAL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_ENTIER, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::CHAINE_LITTERALE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CHAINE_LITTERALE, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::CARACTERE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CARACTERE, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::NUL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NUL, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::TAILLE_DE:
			{
				avance();

				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'taille_de'");
				}

				auto noeud = m_assembleuse->cree_noeud(type_noeud::TAILLE_DE, morceau);

				auto donnees_type = DonneesType{};
				analyse_declaration_type(donnees_type, false);
				noeud->valeur_calculee = donnees_type;

				/* vérifie mais n'avance pas */
				if (!est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après le type de 'taille_de'");
				}

				expression.push_back(noeud);
				break;
			}
			case id_morceau::VRAI:
			case id_morceau::FAUX:
			{
				/* remplace l'identifiant par id_morceau::BOOL */
				auto morceau_bool = DonneesMorceaux{ morceau.chaine, morceau.ligne_pos, id_morceau::BOOL };
				auto noeud = m_assembleuse->cree_noeud(type_noeud::BOOLEEN, morceau_bool);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::TRANSTYPE:
			{
				avance();

				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'transtype'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::TRANSTYPE, morceau, false);

				++m_profondeur;
				analyse_expression_droite(id_morceau::INCONNU);
				--m_profondeur;

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après la déclaration de l'expression");
				}

				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après ')'");
				}

				analyse_declaration_type(noeud->donnees_type, false);

				/* vérifie mais n'avance pas */
				if (!est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après la déclaration du type");
				}

				m_assembleuse->depile_noeud(type_noeud::TRANSTYPE);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::PARENTHESE_OUVRANTE:
			{
				++paren;
				pile.push_back(NOEUD_PARENTHESE);
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

					/* À FAIRE */
					termine_boucle = true;
					break;
				}

				if (pile.empty()) {
					lance_erreur("Il manque une paranthèse dans l'expression !");
				}

				while (pile.back() != NOEUD_PARENTHESE) {
					expression.push_back(pile.back());
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
				auto noeud = static_cast<Noeud *>(nullptr);

				if (precede_unaire_valide(dernier_identifiant)) {
					if (id_operateur == id_morceau::PLUS) {
						id_operateur = id_morceau::PLUS_UNAIRE;
					}
					else if (id_operateur == id_morceau::MOINS) {
						id_operateur = id_morceau::MOINS_UNAIRE;
					}

					auto morceau_unaire = DonneesMorceaux{morceau.chaine, morceau.ligne_pos, id_operateur};
					noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, morceau_unaire);
				}
				else {
					noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, morceau);
				}

				vide_pile_operateur(id_operateur);

				pile.push_back(noeud);

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
			{
				/* Correction de crash d'aléatest, improbable dans la vrai vie. */
				if (expression.empty() && est_operateur_binaire(morceau.identifiant)) {
					avance();
					lance_erreur("Opérateur binaire utilisé en début d'expression");
				}

				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, morceau);
				pile.push_back(noeud);

				break;
			}
			case id_morceau::DE:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::EGAL:
			{
				if (!assignation) {
					avance();
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				vide_pile_operateur(morceau.identifiant);

				auto noeud = m_assembleuse->cree_noeud(type_noeud::ASSIGNATION_VARIABLE, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::CROCHET_OUVRANT:
			{
				vide_pile_operateur(morceau.identifiant);
				avance();

				auto noeud = m_assembleuse->empile_noeud(type_noeud::OPERATION_BINAIRE, morceau, false);
				pile.push_back(noeud);

				++m_profondeur;
				analyse_expression_droite(id_morceau::CROCHET_FERMANT);
				--m_profondeur;

				m_assembleuse->depile_noeud(type_noeud::OPERATION_BINAIRE);

				/* nous reculons, car on avance de nouveau avant de recommencer
				 * la boucle plus bas */
				recule();
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
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, morceau);
				pile.push_back(noeud);
				break;
			}
			default:
			{
				avance();
				lance_erreur("Identifiant inattendu dans l'expression");
			}
		}

		if (termine_boucle) {
			break;
		}

		dernier_identifiant = morceau.identifiant;

		avance();
	}

	/* Retourne s'il n'y a rien dans l'expression, ceci est principalement pour
	 * éviter de crasher lors des fuzz-tests. */
	if (expression.empty()) {
		avance();
		return;
	}

	while (!pile.empty()) {
		if (pile.back() == NOEUD_PARENTHESE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		expression.push_back(pile.back());
		pile.pop_back();
	}

	pile.reserve(expression.size());

	for (Noeud *noeud : expression) {
		if (est_operateur_binaire(noeud->identifiant())) {
			auto n2 = pile.back();
			pile.pop_back();

			auto n1 = pile.back();
			pile.pop_back();

			if (n1->est_constant() && n2->est_constant()) {
				if (est_operateur_constant(noeud->identifiant())) {
					noeud = calcul_expression_double(*m_assembleuse, noeud, n1, n2);

					if (noeud == nullptr) {
						lance_erreur("Ne peut pas calculer l'expression");
					}
				}
				else if (calcul_expression) {
					lance_erreur("Ne peut pas calculer l'expression car l'opérateur n'est pas constant");
				}
			}
			else if (calcul_expression) {
				lance_erreur("Ne peut pas calculer l'expression pour la constante");
			}
			else {
				noeud->reserve_enfants(2);
				noeud->ajoute_noeud(n1);
				noeud->ajoute_noeud(n2);
			}

			pile.push_back(noeud);
		}
		else if (est_operateur_unaire(noeud->identifiant())) {
			auto n1 = pile.back();
			pile.pop_back();

			if (n1->est_constant()) {
				if (est_operateur_constant(noeud->identifiant())) {
					noeud = calcul_expression_simple(*m_assembleuse, noeud, n1);

					if (noeud == nullptr) {
						lance_erreur("Ne peut pas calculer l'expression");
					}
				}
				else if (calcul_expression) {
					lance_erreur("Ne peut pas calculer l'expression car l'opérateur n'est pas constant");
				}
			}
			else if (calcul_expression) {
				lance_erreur("Ne peut pas calculer l'expression car le noeud n'est pas constant");
			}
			else {
				noeud->reserve_enfants(1);
				noeud->ajoute_noeud(n1);
			}

			pile.push_back(noeud);
		}
		else {
			pile.push_back(noeud);
		}
	}

	m_assembleuse->ajoute_noeud(pile.back());
	pile.pop_back();

	if (pile.size() != 0) {
		auto premier_noeud = pile.back();
		auto dernier_noeud = premier_noeud;
		pile.pop_back();

		auto pos_premier = premier_noeud->donnees_morceau().ligne_pos & 0xffffffff;
		auto pos_dernier = pos_premier;

		while (!pile.empty()) {
			auto n = pile.back();
			pile.pop_back();

			auto pos_n = n->donnees_morceau().ligne_pos & 0xffffffff;

			if (pos_n < pos_premier) {
				premier_noeud = n;
			}
			if (pos_n > pos_dernier) {
				dernier_noeud = n;
			}
		}

		erreur::lance_erreur_plage(
					"Expression malformée, il est possible qu'il manque un opérateur",
					m_tampon,
					premier_noeud->donnees_morceau(),
					dernier_noeud->donnees_morceau());
	}

	/* saute l'identifiant final */
	avance();
}

/* f(g(5, 6 + 3 * (2 - 5)), h()); */
void analyseuse_grammaire::analyse_appel_fonction(NoeudAppelFonction *noeud)
{
	/* ici nous devons être au niveau du premier paramètre */

	auto index = 0ul;
	auto arguments_nommes = false;
	std::set<std::string_view> args;
	auto dernier_arg_variadique = false;

	const auto &donnees_fonction = m_contexte.donnees_fonction(noeud->chaine());

	const auto nombre_args = donnees_fonction.args.size();

	while (true) {
		/* aucun paramètre, ou la liste de paramètre est vide */
		if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
			return;
		}

		if (sont_2_identifiants(id_morceau::CHAINE_CARACTERE, id_morceau::EGAL)) {
			avance();

			arguments_nommes = true;

			auto nom_argument = m_identifiants[position()].chaine;

			auto iter = donnees_fonction.args.find(nom_argument);

			if (iter == donnees_fonction.args.end()) {
				erreur::lance_erreur_argument_inconnu(
							nom_argument,
							m_tampon,
							noeud->donnees_morceau());
			}

			if ((args.find(nom_argument) != args.end()) && !iter->second.est_variadic) {
				lance_erreur("Argument déjà nommé", erreur::type_erreur::ARGUMENT_REDEFINI);
			}

			dernier_arg_variadique = iter->second.est_variadic;

			args.insert(nom_argument);
			noeud->ajoute_nom_argument(nom_argument);

			avance();
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				avance();
				lance_erreur("Attendu le nom de l'argument", erreur::type_erreur::ARGUMENT_INCONNU);
			}

			/* par défaut nous nommons les arguments manuellement
			 * À FAIRE : trouver mieux. */
			for (const auto &arg : donnees_fonction.args) {
				if (arg.second.index == index) {
					auto nom_argument = arg.first;
					args.insert(nom_argument);
					noeud->ajoute_nom_argument(nom_argument);
				}
			}
		}

		/* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
		 * si identifiant final == ')', alors l'algorithme s'arrête quand une
		 * paranthèse fermante est trouvé et que la pile est vide */
		++m_profondeur;
		analyse_expression_droite(id_morceau::VIRGULE);
		--m_profondeur;

		index = std::min(index + 1, nombre_args - 1);
	}
}

void analyseuse_grammaire::analyse_declaration_constante()
{
	if (!requiers_identifiant(id_morceau::SOIT)) {
		lance_erreur("Attendu la déclaration 'soit'");
	}

	if (!requiers_identifiant(id_morceau::CONSTANTE)) {
		lance_erreur("Attendu la déclaration 'constante' après 'soit'");
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'constante'");
	}

	auto pos = position();
	auto donnees_type = DonneesType{};

	/* Vérifie s'il y a typage explicit */
	if (est_identifiant(id_morceau::DOUBLE_POINTS)) {
		analyse_declaration_type(donnees_type);
	}

	auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTANTE, m_identifiants[pos]);
	noeud->donnees_type = donnees_type;

	if (!requiers_identifiant(id_morceau::EGAL)) {
		lance_erreur("Attendu '=' après la déclaration de la constante");
	}

	analyse_expression_droite(id_morceau::POINT_VIRGULE, true);

	m_assembleuse->depile_noeud(type_noeud::CONSTANTE);
}

void analyseuse_grammaire::analyse_declaration_structure()
{
	if (!requiers_identifiant(id_morceau::STRUCTURE)) {
		lance_erreur("Attendu la déclaration 'structure'");
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractères après 'structure'");
	}

	auto nom_structure = m_identifiants[position()].chaine;

	if (m_contexte.structure_existe(nom_structure)) {
		lance_erreur("Redéfinition de la structure", erreur::type_erreur::STRUCTURE_REDEFINIE);
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{'");
	}

	auto donnees_structure = DonneesStructure{};

	/* chaine : type ; */
	while (true) {
		if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
			/* nous avons terminé */
			recule();
			break;
		}

		auto nom_membre = m_identifiants[position()].chaine;

		if (donnees_structure.index_membres.find(nom_membre) != donnees_structure.index_membres.end()) {
			lance_erreur("Redéfinition du membre", erreur::type_erreur::MEMBRE_REDEFINI);
		}

		auto donnees_type = DonneesType{};
		analyse_declaration_type(donnees_type);

		if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
			lance_erreur("Attendu ';'");
		}

		donnees_structure.index_membres.insert({nom_membre, donnees_structure.donnees_types.size()});
		donnees_structure.donnees_types.push_back(donnees_type);
	}

	m_contexte.ajoute_donnees_structure(nom_structure, donnees_structure);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de la structure");
	}
}

void analyseuse_grammaire::analyse_declaration_enum()
{
	if (!requiers_identifiant(id_morceau::ENUM)) {
		lance_erreur("Attendu la déclaration 'énum'");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{' après 'énum'");
	}

	while (true) {
		if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
			/* nous avons terminé */
			recule();
			break;
		}

		auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTANTE, m_identifiants[position()]);
		noeud->donnees_type.pousse(id_morceau::N32);

		if (est_identifiant(id_morceau::EGAL)) {
			avance();
			analyse_expression_droite(id_morceau::VIRGULE, true);

			/* recule pour tester la virgule après */
			recule();
		}

		m_assembleuse->depile_noeud(type_noeud::CONSTANTE);

		if (!requiers_identifiant(id_morceau::VIRGULE)) {
			lance_erreur("Attendu ',' à la fin de la déclaration");
		}
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de l'énum");
	}
}

void analyseuse_grammaire::analyse_declaration_type(DonneesType &donnees_type, bool double_point)
{
	if (double_point && !requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':'");
	}

	while (est_specifiant_type(identifiant_courant())) {
		bool est_pointeur = true;
		bool est_tableau  = false;
		int taille = 0;

		if (requiers_identifiant(id_morceau::CROCHET_OUVRANT)) {
			if (this->identifiant_courant() != id_morceau::CROCHET_FERMANT) {
				est_pointeur = false;
				est_tableau = true;

				/* À FAIRE */
#if 0
				analyse_expression_droite(id_morceau::CROCHET_FERMANT, true);
#else
				if (!requiers_nombre_entier()) {
					lance_erreur("Attendu un nombre entier après [");
				}

				const auto &morceau = m_identifiants[position()];
				taille = static_cast<int>(converti_chaine_nombre_entier(morceau.chaine, morceau.identifiant));
#endif
			}

			if (!requiers_identifiant(id_morceau::CROCHET_FERMANT)) {
				lance_erreur("Attendu ']'");
			}
		}

		if (est_pointeur) {
			donnees_type.pousse(id_morceau::POINTEUR);
		}
		else if (est_tableau) {
			/* À FAIRE ? : meilleure manière de stocker la taille. */
			donnees_type.pousse(id_morceau::TABLEAU | (taille << 8));
		}
	}

	if (!requiers_identifiant_type()) {
		lance_erreur("Attendu la déclaration d'un type");
	}

	auto identifiant = m_identifiants[position()].identifiant;

	if (identifiant == id_morceau::CHAINE_CARACTERE) {
		const auto nom_type = m_identifiants[position()].chaine;

		if (!m_contexte.structure_existe(nom_type)) {
			lance_erreur("Structure inconnue", erreur::type_erreur::STRUCTURE_INCONNUE);
		}

		const auto &donnees_structure = m_contexte.donnees_structure(nom_type);
		identifiant = (identifiant | (static_cast<int>(donnees_structure.id) << 8));
	}

	donnees_type.pousse(identifiant);
}

bool analyseuse_grammaire::requiers_identifiant_type()
{
	const auto ok = est_identifiant_type(this->identifiant_courant());
	avance();
	return ok;
}

bool analyseuse_grammaire::requiers_nombre_entier()
{
	const auto ok = est_nombre_entier(this->identifiant_courant());
	avance();
	return ok;
}
