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

#include <chrono/outils.hh>
#include <iostream>
#include <set>

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "expression.h"
#include "modules.hh"
#include "nombres.h"

#undef DEBOGUE_EXPRESSION

#ifdef DEBOGUE_EXPRESSION
static constexpr auto g_log_expression = true;
#else
static constexpr auto g_log_expression = false;
#endif

#define DEB_LOG_EXPRESSION if (g_log_expression) { std::cerr
#define FIN_LOG_EXPRESSION '\n';}

/**
 * Limitation du nombre récursif de sous-expressions (par exemple :
 * f(g(h(i(j()))))).
 */
static constexpr auto PROFONDEUR_EXPRESSION_MAX = 32;

/* Tabulations utilisées au début des logs. */
static const char *tabulations[PROFONDEUR_EXPRESSION_MAX] = {
	"",
	" ",
	"  ",
	"   ",
	"    ",
	"     ",
	"      ",
	"       ",
	"        ",
	"         ",
	"          ",
	"           ",
	"            ",
	"             ",
	"              ",
	"               ",
	"                ",
	"                 ",
	"                  ",
	"                   ",
	"                    ",
	"                     ",
	"                      ",
	"                       ",
	"                        ",
	"                         ",
	"                          ",
	"                           ",
	"                            ",
	"                             ",
	"                              ",
	"                               ",
};

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
		case id_morceau::DIVSE_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::MODULO_EGAL:
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

	/* À FAIRE : ceci mélange a[-i] et a[i] - b[i] */
	if (dernier_identifiant == id_morceau::CROCHET_OUVRANT) {
		return false;
	}

	return true;
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(
		ContexteGenerationCode &contexte,
		DonneesModule *module,
		std::string const &racine_kuri)
	: lng::analyseuse<DonneesMorceaux>(module->morceaux)
	, m_contexte(contexte)
	, m_assembleuse(contexte.assembleuse)
	, m_paires_vecteurs(PROFONDEUR_EXPRESSION_MAX)
	, m_racine_kuri(racine_kuri)
	, m_module(module)
{}

void analyseuse_grammaire::lance_analyse(std::ostream &os)
{
	m_position = 0;

	if (m_identifiants.size() == 0) {
		return;
	}

	m_module->temps_analyse = 0.0;
	m_debut_analyse = dls::chrono::maintenant();
	analyse_corps(os);
	m_module->temps_analyse += dls::chrono::delta(m_debut_analyse);
}

void analyseuse_grammaire::analyse_corps(std::ostream &os)
{
	while (!fini()) {
		if (est_identifiant(id_morceau::FONCTION)) {
			analyse_declaration_fonction();
		}
		else if (est_identifiant(id_morceau::SOIT)) {
			avance();
			analyse_declaration_variable(GLOBAL);
		}
		else if (est_identifiant(id_morceau::DYN)) {
			avance();
			analyse_declaration_variable(GLOBAL | DYNAMIC);
		}
		else if (est_identifiant(id_morceau::STRUCTURE)) {
			analyse_declaration_structure();
		}
		else if (est_identifiant(id_morceau::ENUM)) {
			analyse_declaration_enum();
		}
		else if (est_identifiant(id_morceau::IMPORTE)) {
			avance();

			if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
				lance_erreur("Attendu une chaine littérale après 'importe'");
			}

			auto const nom_module = donnees().chaine;
			m_module->modules_importes.insert(nom_module);

			/* désactive le 'chronomètre' car sinon le temps d'analyse prendra
			 * également en compte le chargement, le découpage, et l'analyse du
			 * module importé */
			m_module->temps_analyse += dls::chrono::delta(m_debut_analyse);
			charge_module(os, m_racine_kuri, std::string(nom_module), m_contexte, donnees());
			m_debut_analyse = dls::chrono::maintenant();
		}
		else if (est_identifiant(id_morceau::DIRECTIVE)) {
			avance();

			if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine de caractère après '#!'");
			}

			auto directive = donnees().chaine;

			if (directive == "inclus") {
				if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
					lance_erreur("Attendu une chaine littérale après la directive");
				}

				auto chaine = donnees().chaine;
				m_assembleuse->inclusions.push_back(chaine);
			}
			else if (directive == "bib") {
				if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
					lance_erreur("Attendu une chaine littérale après la directive");
				}

				auto chaine = donnees().chaine;
				m_assembleuse->bibliotheques.push_back(chaine);
			}
			else {
				lance_erreur("Directive inconnue");
			}
		}
		else {
			avance();
			lance_erreur("Identifiant inattendu, doit être 'soit', 'dyn', 'fonction', 'structure', 'importe', ou 'énum'");
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

	auto const nom_fonction = donnees().chaine;
	m_module->fonctions_exportees.insert(nom_fonction);

	auto noeud = m_assembleuse->empile_noeud(type_noeud::DECLARATION_FONCTION, m_contexte, donnees());

	if (externe) {
		noeud->drapeaux |= EST_EXTERNE;
	}

	if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
	}

	auto donnees_type_fonction = DonneesType{};
	donnees_type_fonction.pousse(id_morceau::FONCTION);
	donnees_type_fonction.pousse(id_morceau::PARENTHESE_OUVRANTE);

	auto donnees_fonctions = DonneesFonction{};
	donnees_fonctions.est_externe = externe;
	donnees_fonctions.noeud_decl = noeud;

	analyse_parametres_fonction(noeud, donnees_fonctions, &donnees_type_fonction);

	if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une parenthèse fermante après la liste des paramètres de la fonction");
	}

	donnees_type_fonction.pousse(id_morceau::PARENTHESE_FERMANTE);

	/* À FAIRE : inférence de type retour. */
	noeud->index_type = analyse_declaration_type(&donnees_type_fonction);
	donnees_fonctions.index_type_retour = noeud->index_type;
	donnees_fonctions.index_type = m_contexte.magasin_types.ajoute_type(donnees_type_fonction);

	if (m_module->fonction_existe(nom_fonction)) {
		auto const &vdf = m_module->donnees_fonction(nom_fonction);

		for (auto const &df : vdf) {
			if (df.index_type == donnees_fonctions.index_type) {
				lance_erreur("Redéfinition de la fonction", erreur::type_erreur::FONCTION_REDEFINIE);
			}
		}
	}

	m_module->ajoute_donnees_fonctions(nom_fonction, donnees_fonctions);

	if (externe) {
		if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
			lance_erreur("Attendu un point-virgule ';' après la déclaration de la fonction externe");
		}
	}
	else {
		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante après la liste des paramètres de la fonction");
		}

		m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
		analyse_corps_fonction();
		m_assembleuse->depile_noeud(type_noeud::BLOC);

		if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante à la fin de la fonction");
		}
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_FONCTION);
}

void analyseuse_grammaire::analyse_parametres_fonction(
		noeud::base *noeud,
		DonneesFonction &donnees_fonction,
		DonneesType *donnees_type_fonction)
{
	if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		/* La liste est vide. */
		return;
	}

	auto est_dynamic = false;

	if (est_identifiant(id_morceau::DYN)) {
		est_dynamic = true;
		avance();
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu le nom de la variable");
	}

	auto nom_parametre = donnees().chaine;

	if (donnees_fonction.args.find(nom_parametre) != donnees_fonction.args.end()) {
		lance_erreur("Redéfinition de l'argument", erreur::type_erreur::ARGUMENT_REDEFINI);
	}

	if (!requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':' après le nom de l'argument");
	}

	auto index_dt = size_t{-1ul};

	if (!est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		index_dt = analyse_declaration_type(donnees_type_fonction, false);

		auto &dt = m_contexte.magasin_types.donnees_types[index_dt];

		if (dt.type_base() == type_id::TROIS_POINTS) {
			noeud->drapeaux |= VARIADIC;

			if (!possede_drapeau(noeud->drapeaux, EST_EXTERNE) && dt.derefence().est_invalide()) {
				lance_erreur("La déclaration de fonction variadique sans type n'est"
							 " implémentée que pour les fonctions externes");
			}
		}
	}

	DonneesArgument donnees_arg;
	donnees_arg.index = donnees_fonction.args.size();
	donnees_arg.donnees_type = index_dt;
	/* doit être vrai uniquement pour le dernier argument */
	donnees_arg.est_variadic = (noeud->drapeaux & VARIADIC) != 0;
	donnees_arg.est_dynamic = est_dynamic;

	donnees_fonction.args.insert({nom_parametre, donnees_arg});
	donnees_fonction.nom_args.push_back(nom_parametre);
	donnees_fonction.est_variadique = (noeud->drapeaux & VARIADIC) != 0;

	/* fin des paramètres */
	if (!requiers_identifiant(id_morceau::VIRGULE)) {
		recule();
		return;
	}

	donnees_type_fonction->pousse(id_morceau::VIRGULE);

	if ((noeud->drapeaux & VARIADIC) == 0) {
		analyse_parametres_fonction(noeud, donnees_fonction, donnees_type_fonction);
	}
}

void analyseuse_grammaire::analyse_controle_si(type_noeud tn)
{
	m_assembleuse->empile_noeud(tn, m_contexte, donnees());

	analyse_expression_droite(id_morceau::ACCOLADE_OUVRANTE, id_morceau::SI);

	m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());

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
	if (!requiers_identifiant(id_morceau::POUR)) {
		lance_erreur("Attendu la déclaration 'pour'");
	}

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

	m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());

	analyse_corps_fonction();

	m_assembleuse->depile_noeud(type_noeud::BLOC);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'pour'");
	}

	/* enfant 4 : bloc sansarrêt (optionel) */
	if (est_identifiant(id_morceau::SANSARRET)) {
		avance();

		m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());

		if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
			lance_erreur("Attendu une accolade ouvrante '{' au début du bloc de 'sinon'");
		}

		analyse_corps_fonction();

		m_assembleuse->depile_noeud(type_noeud::BLOC);

		if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'sinon'");
		}
	}

	/* enfant 4 ou 5 : bloc sinon (optionel) */
	if (est_identifiant(id_morceau::SINON)) {
		avance();

		m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());

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
		auto const pos = position();

		/* assignement|déclaration constante : soit x = a + b; */
		if (est_identifiant(id_morceau::SOIT)) {
			avance();
			analyse_declaration_variable(0);
		}
		/* assignement|déclaration dynamique : dyn x = a + b; */
		else if (est_identifiant(id_morceau::DYN)) {
			avance();
			analyse_declaration_variable(DYNAMIC);
		}
		/* retour : retourne a + b; */
		else if (est_identifiant(id_morceau::RETOURNE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETOUR, m_contexte, donnees());

			/* Considération du cas où l'on ne retourne rien 'retourne;'. */
			if (!est_identifiant(id_morceau::POINT_VIRGULE)) {
				analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::RETOURNE);
			}
			else {
				avance();
			}

			if (!est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante après l'expression de retour.");
			}

			m_assembleuse->depile_noeud(type_noeud::RETOUR);
		}
		else if (est_identifiant(id_morceau::RETIENS)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::RETIENS, m_contexte, donnees());

			analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::RETOURNE);

			m_assembleuse->depile_noeud(type_noeud::RETIENS);
		}
		else if (est_identifiant(id_morceau::SI)) {
			avance();
			analyse_controle_si(type_noeud::SI);
		}
		else if (est_identifiant(id_morceau::SAUFSI)) {
			avance();
			analyse_controle_si(type_noeud::SAUFSI);
		}
		else if (est_identifiant(id_morceau::POUR)) {
			analyse_controle_pour();
		}
		else if (est_identifiant(id_morceau::BOUCLE)) {
			avance();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'boucle'");
			}

			m_assembleuse->empile_noeud(type_noeud::BOUCLE, m_contexte, donnees());
			m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
			analyse_corps_fonction();
			m_assembleuse->depile_noeud(type_noeud::BLOC);

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'boucle'");
			}

			m_assembleuse->depile_noeud(type_noeud::BOUCLE);
		}
		else if (est_identifiant(id_morceau::TANTQUE)) {
			avance();
			m_assembleuse->empile_noeud(type_noeud::TANTQUE, m_contexte, donnees());

			++m_profondeur;
			analyse_expression_droite(type_id::ACCOLADE_OUVRANTE, type_id::TANTQUE);
			--m_profondeur;

			/* recule pour être de nouveau synchronisé */
			recule();

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après l'expression de 'tanque'");
			}

			m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
			analyse_corps_fonction();
			m_assembleuse->depile_noeud(type_noeud::BLOC);

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'boucle'");
			}

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
				lance_erreur("Attendu une accolade ouvrante '{' après 'défère'");
			}

			m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
			analyse_corps_fonction();
			m_assembleuse->depile_noeud(type_noeud::BLOC);

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'défère'");
			}

			m_assembleuse->depile_noeud(type_noeud::DIFFERE);
		}
		else if (est_identifiant(id_morceau::NONSUR)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::NONSUR, m_contexte, donnees());

			if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante '{' après 'nonsûr'");
			}

			m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
			analyse_corps_fonction();
			m_assembleuse->depile_noeud(type_noeud::BLOC);

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'défère'");
			}

			m_assembleuse->depile_noeud(type_noeud::NONSUR);
		}
		else if (est_identifiant(id_morceau::ASSOCIE)) {
			avance();

			m_assembleuse->empile_noeud(type_noeud::ASSOCIE, m_contexte, donnees());

			++m_profondeur;
			analyse_expression_droite(type_id::ACCOLADE_OUVRANTE, type_id::ASSOCIE);
			--m_profondeur;

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

				++m_profondeur;
				analyse_expression_droite(type_id::ACCOLADE_OUVRANTE, type_id::ASSOCIE);
				--m_profondeur;

				/* recule pour être de nouveau synchronisé */
				recule();

				if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
					lance_erreur("Attendu une accolade ouvrante '{' après l'expression de 'associe'");
				}

				m_assembleuse->empile_noeud(type_noeud::BLOC, m_contexte, donnees());
				analyse_corps_fonction();
				m_assembleuse->depile_noeud(type_noeud::BLOC);

				if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
					lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'associe'");
				}

				m_assembleuse->depile_noeud(type_noeud::PAIRE_ASSOCIATION);
			}

			if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin du bloc de 'associe'");
			}

			m_assembleuse->depile_noeud(type_noeud::ASSOCIE);
		}
		/* appel : fais_quelque_chose(); */
		else if (sont_2_identifiants(id_morceau::CHAINE_CARACTERE, id_morceau::PARENTHESE_OUVRANTE)) {
			analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::PARENTHESE_OUVRANTE);
		}
		else {
			analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::EGAL, false, true);
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

void analyseuse_grammaire::analyse_expression_droite(
		id_morceau identifiant_final,
		id_morceau racine_expr,
		bool const calcul_expression,
		bool const assignation)
{
	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */

	if (m_profondeur >= m_paires_vecteurs.size()) {
		lance_erreur("Excès de la pile d'expression autorisée");
	}

	auto &expression = m_paires_vecteurs[m_profondeur].first;
	expression.clear();

	auto &pile = m_paires_vecteurs[m_profondeur].second;
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
	auto dernier_identifiant = donnees().identifiant;

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	DEB_LOG_EXPRESSION << tabulations[m_profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

	while (!requiers_identifiant(identifiant_final)) {
		auto &morceau = donnees();

		DEB_LOG_EXPRESSION << tabulations[m_profondeur] << '\t' << chaine_identifiant(morceau.identifiant) << FIN_LOG_EXPRESSION;

		switch (morceau.identifiant) {
			case id_morceau::CHAINE_CARACTERE:
			{
				/* appel fonction : chaine + ( */
				if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					avance();

					auto noeud = m_assembleuse->empile_noeud(type_noeud::APPEL_FONCTION, m_contexte, morceau, false);

					analyse_appel_fonction(noeud);

					m_assembleuse->depile_noeud(type_noeud::APPEL_FONCTION);

					expression.push_back(noeud);
				}
				/* construction structure : chaine + { */
				else if (racine_expr == id_morceau::EGAL && est_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_STRUCTURE, m_contexte, morceau, false);

					avance();

					analyse_construction_structure(noeud);

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_STRUCTURE);

					expression.push_back(noeud);
				}
				/* variable : chaine */
				else {
					auto noeud = m_assembleuse->cree_noeud(type_noeud::VARIABLE, m_contexte, morceau);
					expression.push_back(noeud);

					/* nous avons la déclaration d'un type dans la structure */
					if (racine_expr == id_morceau::STRUCTURE && est_identifiant(id_morceau::DOUBLE_POINTS)) {
						noeud->index_type = analyse_declaration_type();
					}
				}

				break;
			}
			case id_morceau::NOMBRE_REEL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_REEL, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::NOMBRE_BINAIRE:
			case id_morceau::NOMBRE_ENTIER:
			case id_morceau::NOMBRE_HEXADECIMAL:
			case id_morceau::NOMBRE_OCTAL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_ENTIER, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::CHAINE_LITTERALE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CHAINE_LITTERALE, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::CARACTERE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CARACTERE, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::NUL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NUL, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::TAILLE_DE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'taille_de'");
				}

				auto noeud = m_assembleuse->cree_noeud(type_noeud::TAILLE_DE, m_contexte, morceau);

				auto donnees_type = size_t{-1ul};
				donnees_type = analyse_declaration_type(nullptr, false);
				noeud->valeur_calculee = donnees_type;

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après le type de 'taille_de'");
				}

				expression.push_back(noeud);
				break;
			}
			case id_morceau::TYPE_DE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'type_de'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::TYPE_DE, m_contexte, morceau, false);

				++m_profondeur;
				analyse_expression_droite(id_morceau::INCONNU, id_morceau::TYPE_DE);
				--m_profondeur;

				m_assembleuse->depile_noeud(type_noeud::TYPE_DE);

				/* vérifie mais n'avance pas */
				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après l'expression de 'taille_de'");
				}

				expression.push_back(noeud);
				break;
			}
			case id_morceau::VRAI:
			case id_morceau::FAUX:
			{
				/* remplace l'identifiant par id_morceau::BOOL */
				morceau.identifiant = id_morceau::BOOL;
				auto noeud = m_assembleuse->cree_noeud(type_noeud::BOOLEEN, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::TRANSTYPE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'transtype'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::TRANSTYPE, m_contexte, morceau, false);

				++m_profondeur;
				analyse_expression_droite(id_morceau::DOUBLE_POINTS, id_morceau::TRANSTYPE);
				--m_profondeur;

				noeud->index_type = analyse_declaration_type(nullptr, false);

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après la déclaration du type");
				}

				m_assembleuse->depile_noeud(type_noeud::TRANSTYPE);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::MEMOIRE:
			{
				if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					lance_erreur("Attendu '(' après 'mémoire'");
				}

				auto noeud = m_assembleuse->empile_noeud(type_noeud::MEMOIRE, m_contexte, morceau, false);

				++m_profondeur;
				analyse_expression_droite(id_morceau::INCONNU, id_morceau::MEMOIRE);
				--m_profondeur;

				if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu ')' après l'expression");
				}

				m_assembleuse->depile_noeud(type_noeud::MEMOIRE);
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
			case id_morceau::PLUS_EGAL:
			case id_morceau::MOINS_EGAL:
			case id_morceau::DIVSE_EGAL:
			case id_morceau::MULTIPLIE_EGAL:
			case id_morceau::MODULO_EGAL:
			case id_morceau::VIRGULE:
			{
				/* Correction de crash d'aléatest, improbable dans la vrai vie. */
				if (expression.empty() && est_operateur_binaire(morceau.identifiant)) {
					lance_erreur("Opérateur binaire utilisé en début d'expression");
				}

				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
				pile.push_back(noeud);

				break;
			}
			case id_morceau::TROIS_POINTS:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::PLAGE, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::DE:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::POINT:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE_POINT, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::EGAL:
			{
				if (!assignation) {
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				vide_pile_operateur(morceau.identifiant);

				auto noeud = m_assembleuse->cree_noeud(type_noeud::ASSIGNATION_VARIABLE, m_contexte, morceau);
				pile.push_back(noeud);
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
					pile.push_back(noeud);

					++m_profondeur;
					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT);
					--m_profondeur;

					/* Extrait le noeud enfant, il sera de nouveau ajouté dans
					 * la compilation de l'expression à la fin de la fonction. */
					auto noeud_expr = noeud->enfants.front();
					noeud->enfants.clear();

					/* Si la racine de l'expression est un opérateur, il faut
					 * l'empêcher d'être prise en compte pour l'expression
					 * courante. */
					noeud_expr->drapeaux |= IGNORE_OPERATEUR;

					expression.push_back(noeud_expr);

					m_assembleuse->depile_noeud(type_noeud::OPERATION_BINAIRE);
				}
				else {
					/* change l'identifiant pour ne pas le confondre avec l'opérateur binaire [] */
					morceau.identifiant = id_morceau::TABLEAU;
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_TABLEAU, m_contexte, morceau, false);

					++m_profondeur;
					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT);
					--m_profondeur;

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_TABLEAU);

					expression.push_back(noeud);
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
				pile.push_back(noeud);
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
					noeud->index_type = analyse_declaration_type(nullptr, false);

					avance();

					++m_profondeur;
					analyse_expression_droite(type_id::SINON, type_id::LOGE);
					--m_profondeur;

					if (!requiers_identifiant(type_id::PARENTHESE_FERMANTE)) {
						lance_erreur("Attendu une paranthèse fermante ')");
					}
				}
				else {
					noeud->index_type = analyse_declaration_type(nullptr, false);
				}

				if (est_identifiant(type_id::SINON)) {
					avance();
					m_assembleuse->empile_noeud(
								type_noeud::BLOC,
								m_contexte,
								morceau);

					if (!requiers_identifiant(type_id::ACCOLADE_OUVRANTE)) {
						lance_erreur("Attendu une accolade ouvrante '{'");
					}

					++m_profondeur;
					analyse_corps_fonction();
					--m_profondeur;

					if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
						lance_erreur("Attendu une accolade fermante '}'");
					}

					m_assembleuse->depile_noeud(type_noeud::BLOC);

					termine_boucle = true;
				}

				m_assembleuse->depile_noeud(type_noeud::LOGE);

				expression.push_back(noeud);
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

				++m_profondeur;
				analyse_expression_droite(type_id::DOUBLE_POINTS, type_id::RELOGE);
				--m_profondeur;

				if (est_identifiant(type_id::CHAINE)) {
					noeud_reloge->index_type = analyse_declaration_type(nullptr, false);

					avance();

					++m_profondeur;
					analyse_expression_droite(type_id::SINON, type_id::RELOGE);
					--m_profondeur;

					if (!requiers_identifiant(type_id::PARENTHESE_FERMANTE)) {
						lance_erreur("Attendu une paranthèse fermante ')");
					}
				}
				else {
					noeud_reloge->index_type = analyse_declaration_type(nullptr, false);
				}

				if (est_identifiant(type_id::SINON)) {
					avance();
					m_assembleuse->empile_noeud(
								type_noeud::BLOC,
								m_contexte,
								morceau);

					if (!requiers_identifiant(type_id::ACCOLADE_OUVRANTE)) {
						lance_erreur("Attendu une accolade ouvrante '{'");
					}

					++m_profondeur;
					analyse_corps_fonction();
					--m_profondeur;

					if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
						lance_erreur("Attendu une accolade fermante '}'");
					}

					m_assembleuse->depile_noeud(type_noeud::BLOC);

					termine_boucle = true;
				}

				m_assembleuse->depile_noeud(type_noeud::RELOGE);

				expression.push_back(noeud_reloge);
				break;
			}
			case id_morceau::DELOGE:
			{
				auto noeud = m_assembleuse->empile_noeud(
							type_noeud::DELOGE,
							m_contexte,
							morceau,
							false);

				++m_profondeur;
				analyse_expression_droite(type_id::POINT_VIRGULE, type_id::DELOGE);
				--m_profondeur;

				/* besoin de reculer car l'analyse va jusqu'au point-virgule, ce
				 * qui nous fait absorber le code de l'expression suivante */
				recule();

				m_assembleuse->depile_noeud(type_noeud::DELOGE);

				expression.push_back(noeud);

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
	if (expression.empty()) {
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

	DEB_LOG_EXPRESSION << tabulations[m_profondeur] << "Expression :" << FIN_LOG_EXPRESSION;

	for (auto noeud : expression) {
		DEB_LOG_EXPRESSION << tabulations[m_profondeur] << '\t' << chaine_identifiant(noeud->identifiant()) << FIN_LOG_EXPRESSION;

		if (!possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_binaire(noeud->identifiant())) {
			if (pile.size() < 2) {
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

			if (est_constant(n1) && est_constant(n2)) {
				if (est_operateur_constant(noeud->identifiant())) {
					noeud = calcul_expression_double(*m_assembleuse, m_contexte, noeud, n1, n2);

					if (noeud == nullptr) {
						lance_erreur("Ne peut pas calculer l'expression");
					}
				}
				else if (calcul_expression) {
					lance_erreur("Ne peut pas calculer l'expression car l'opérateur n'est pas constant");
				}
				else {
					noeud->ajoute_noeud(n1);
					noeud->ajoute_noeud(n2);
				}
			}
			else if (calcul_expression) {
				if (pile.size() < 1) {
					erreur::lance_erreur(
								"Expression malformée pour opérateur unaire",
								m_contexte,
								noeud->donnees_morceau(),
								erreur::type_erreur::NORMAL);
				}

				lance_erreur("Ne peut pas calculer l'expression pour la constante");
			}
			else {
				noeud->ajoute_noeud(n1);
				noeud->ajoute_noeud(n2);
			}

			pile.push_back(noeud);
		}
		else if (!possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_unaire(noeud->identifiant())) {
			auto n1 = pile.back();
			pile.pop_back();

			if (est_constant(n1)) {
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
					m_contexte,
					premier_noeud->donnees_morceau(),
					dernier_noeud->donnees_morceau());
	}
}

/* f(g(5, 6 + 3 * (2 - 5)), h()); */
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
		++m_profondeur;
		analyse_expression_droite(id_morceau::VIRGULE, id_morceau::EGAL);
		--m_profondeur;
	}
}

void analyseuse_grammaire::analyse_declaration_variable(unsigned short drapeaux)
{
	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		if ((drapeaux & DYNAMIC) != 0) {
			lance_erreur("Attendu une chaine de caractère après 'dyn'");
		}
		else {
			lance_erreur("Attendu une chaine de caractère après 'soit'");
		}
	}

	const auto &morceau_variable = donnees();
	auto donnees_type = size_t{-1ul};

	if (est_identifiant(id_morceau::DOUBLE_POINTS)) {
		donnees_type = analyse_declaration_type();
	}

	/* À FAIRE : ceci est principalement pour pouvoir déclarer des
	 * structures ou des tableaux en attendant de pouvoir les initialiser
	 * par une assignation directe. par exemple : soit x = Vecteur3D(); */
	if (!est_identifiant(id_morceau::EGAL)) {
		if ((drapeaux & DYNAMIC) == 0) {
			avance();
			lance_erreur("Attendu '=' après chaine de caractère");
		}

		auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_VARIABLE, m_contexte, morceau_variable);
		noeud_decl->index_type = donnees_type;
		noeud_decl->drapeaux |= drapeaux;
		m_assembleuse->depile_noeud(type_noeud::DECLARATION_VARIABLE);

		if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
			lance_erreur("Attendu ';' à la fin de la déclaration de la variable");
		}
	}
	else {
		avance();

		auto const &morceau_egal = donnees();

		auto noeud = m_assembleuse->empile_noeud(type_noeud::ASSIGNATION_VARIABLE, m_contexte, morceau_egal);
		noeud->index_type = donnees_type;

		auto noeud_decl = m_assembleuse->cree_noeud(type_noeud::DECLARATION_VARIABLE, m_contexte, morceau_variable);
		noeud_decl->index_type = donnees_type;
		noeud_decl->drapeaux |= drapeaux;
		noeud->ajoute_noeud(noeud_decl);

		analyse_expression_droite(id_morceau::POINT_VIRGULE, id_morceau::EGAL);

		m_assembleuse->depile_noeud(type_noeud::ASSIGNATION_VARIABLE);
	}
}

void analyseuse_grammaire::analyse_declaration_structure()
{
	if (!requiers_identifiant(id_morceau::STRUCTURE)) {
		lance_erreur("Attendu la déclaration 'structure'");
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaine de caractères après 'structure'");
	}

	auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_STRUCTURE, m_contexte, donnees());
	auto nom_structure = donnees().chaine;

	if (m_contexte.structure_existe(nom_structure)) {
		lance_erreur("Redéfinition de la structure", erreur::type_erreur::STRUCTURE_REDEFINIE);
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{'");
	}

	auto donnees_structure = DonneesStructure{};
	donnees_structure.noeud_decl = noeud_decl;
	donnees_structure.est_enum = false;

	m_contexte.ajoute_donnees_structure(nom_structure, donnees_structure);

	while (true) {
		if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			/* nous avons terminé */
			break;
		}

		analyse_expression_droite(id_morceau::POINT_VIRGULE, type_id::STRUCTURE, false, true);
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de la structure");
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_STRUCTURE);
}

void analyseuse_grammaire::analyse_declaration_enum()
{
	if (!requiers_identifiant(id_morceau::ENUM)) {
		lance_erreur("Attendu la déclaration 'énum'");
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Attendu un nom après 'énum'");
	}

	auto noeud_decl = m_assembleuse->empile_noeud(type_noeud::DECLARATION_ENUM, m_contexte, donnees());
	auto nom = noeud_decl->morceau.chaine;

	auto donnees_structure = DonneesStructure{};
	donnees_structure.est_enum = true;
	donnees_structure.noeud_decl = noeud_decl;

	m_contexte.ajoute_donnees_structure(nom, donnees_structure);

	noeud_decl->index_type = analyse_declaration_type();

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{' après 'énum'");
	}

	while (true) {
		if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
			/* nous avons terminé */
			break;
		}

		analyse_expression_droite(id_morceau::VIRGULE, id_morceau::EGAL, false, true);
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de l'énum");
	}

	m_assembleuse->depile_noeud(type_noeud::DECLARATION_ENUM);
}

size_t analyseuse_grammaire::analyse_declaration_type(DonneesType *donnees_type_fonction, bool double_point)
{
	if (double_point && !requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
		lance_erreur("Attendu ':'");
	}

	/* Vérifie si l'on a un pointeur vers une fonction. */
	if (est_identifiant(id_morceau::FONCTION)) {
		avance();

		auto dt = DonneesType{};
		dt.pousse(id_morceau::FONCTION);

		if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
			lance_erreur("Attendu un '(' après 'fonction'");
		}

		dt.pousse(id_morceau::PARENTHESE_OUVRANTE);

		while (true) {
			if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
				break;
			}

			analyse_declaration_type(&dt, false);

			if (!est_identifiant(id_morceau::VIRGULE)) {
				break;
			}

			avance();
			dt.pousse(id_morceau::VIRGULE);
		}

		avance();
		dt.pousse(id_morceau::PARENTHESE_FERMANTE);

		analyse_declaration_type(&dt, false);

		if (donnees_type_fonction) {
			donnees_type_fonction->pousse(dt);
		}

		return m_contexte.magasin_types.ajoute_type(dt);
	}

	return analyse_declaration_type_ex(donnees_type_fonction);
}

size_t analyseuse_grammaire::analyse_declaration_type_ex(DonneesType *donnees_type_fonction)
{
	auto dernier_id = id_morceau{};
	auto donnees_type = DonneesType{};

	while (est_specifiant_type(identifiant_courant())) {
		auto id = this->identifiant_courant();
		avance();

		switch (id) {
			case type_id::CROCHET_OUVRANT:
			{
				auto taille = 0;

				if (this->identifiant_courant() != id_morceau::CROCHET_FERMANT) {
					/* À FAIRE */
#if 0
					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT, true);
#else
					if (!requiers_nombre_entier()) {
						lance_erreur("Attendu un nombre entier après [");
					}

					auto const &morceau = donnees();
					taille = static_cast<int>(converti_chaine_nombre_entier(morceau.chaine, morceau.identifiant));
#endif
				}

				if (!requiers_identifiant(id_morceau::CROCHET_FERMANT)) {
					lance_erreur("Attendu ']'");
				}

				/* À FAIRE ? : meilleure manière de stocker la taille. */
				donnees_type.pousse(id_morceau::TABLEAU | (taille << 8));

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
			default:
			{
				break;
			}
		}

		dernier_id = id;
	}

	/* Soutiens pour les types des fonctions variadiques externes. */
	if (dernier_id != id_morceau::TROIS_POINTS || !est_identifiant(type_id::PARENTHESE_FERMANTE)) {
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

		if (donnees_type_fonction) {
			donnees_type_fonction->pousse(donnees_type);
		}
	}

	return m_contexte.magasin_types.ajoute_type(donnees_type);
}

void analyseuse_grammaire::analyse_construction_structure(noeud::base *noeud)
{
	auto liste_param = std::vector<std::string_view>{};

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
		liste_param.push_back(nom);

		avance();

		++m_profondeur;
		analyse_expression_droite(id_morceau::VIRGULE, id_morceau::EGAL);
		--m_profondeur;
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

void analyseuse_grammaire::lance_erreur(const std::string &quoi, erreur::type_erreur type)
{
	erreur::lance_erreur(quoi, m_contexte, donnees(), type);
}
