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

#include "syntaxeuse.hh"

#include <filesystem>

#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "compilatrice.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
#include "profilage.hh"
#include "typage.hh"

// Pour les bibliothèques externes ou les inclusions, détermine le chemin absolu selon le fichier courant, au cas où la bibliothèque serait dans le même dossier que le fichier
static auto trouve_chemin_si_dans_dossier(Module *module, dls::chaine const &chaine)
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

template <typename T, unsigned long N>
static auto copie_tablet_tableau(dls::tablet<T, N> const &src, kuri::tableau<T> &dst)
{
	dst.reserve(src.taille());

	POUR (src) {
		dst.pousse(it);
	}
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

static constexpr int PRECEDENCE_TYPE = 4;

static int precedence_pour_operateur(GenreLexeme genre_operateur)
{
	switch (genre_operateur) {
		case GenreLexeme::VIRGULE:
		{
			return 1;
		}
		case GenreLexeme::TROIS_POINTS:
		{
			return 2;
		}
		case GenreLexeme::EGAL:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DECLARATION_CONSTANTE:
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
		{
			return 3;
		}
		// La précédence de 4 est réservée pour les déclarations de type après ':' où nous devons nous
		// arrêter avant le '=' ou la ',' ; arrêt réalisé via une précédence plus forte. Par contre
		// nous ne désidérons transformer ':' en un opérateur.
		case GenreLexeme::BARRE_BARRE:
		{
			return 5;
		}
		case GenreLexeme::ESP_ESP:
		{
			return 6;
		}
		case GenreLexeme::BARRE:
		{
			return 7;
		}
		case GenreLexeme::CHAPEAU:
		{
			return 8;
		}
		case GenreLexeme::ESPERLUETTE:
		{
			return 9;
		}
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
		{
			return 10;
		}
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			return 11;
		}
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECALAGE_DROITE:
		{
			return 12;
		}
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		{
			return 13;
		}
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::POURCENT:
		{
			return 14;
		}
		case GenreLexeme::COMME:
		{
			return 15;
		}
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::AROBASE:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
		case GenreLexeme::FOIS_UNAIRE:
		case GenreLexeme::ESP_UNAIRE:
		case GenreLexeme::EXPANSION_VARIADIQUE:
		{
			return 16;
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		case GenreLexeme::POINT:
		case GenreLexeme::CROCHET_OUVRANT:
		{
			return 17;
		}
		default:
		{
			std::cerr << "Aucune précédence pour l'opérateur : " << chaine_du_lexeme(genre_operateur) << '\n';
			assert(false);
			return -1;
		}
	}
}

static Associativite associativite_pour_operateur(GenreLexeme genre_operateur)
{
	switch (genre_operateur) {
		case GenreLexeme::VIRGULE:
		case GenreLexeme::TROIS_POINTS:
		case GenreLexeme::EGAL:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DECLARATION_CONSTANTE:
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
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::POURCENT:
		case GenreLexeme::POINT:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::PARENTHESE_OUVRANTE:
		case GenreLexeme::COMME:
		{
			return Associativite::GAUCHE;
		}
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::AROBASE:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::FOIS_UNAIRE:
		case GenreLexeme::ESP_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
		case GenreLexeme::EXPANSION_VARIADIQUE:
		{
			return Associativite::DROITE;
		}
		default:
		{
			std::cerr << "Aucune associativité pour l'opérateur : " << chaine_du_lexeme(genre_operateur) << '\n';
			assert(false);
			return static_cast<Associativite>(-1);
		}
	}
}

#define CREE_NOEUD(Type, Genre, Lexeme) static_cast<Type *>(m_fichier->module->assembleuse->cree_noeud(Genre, Lexeme))

Syntaxeuse::Syntaxeuse(
		Compilatrice &compilatrice,
		Fichier *fichier,
		const dls::chaine &racine_kuri)
	: m_compilatrice(compilatrice)
	, m_fichier(fichier)
	, m_lexemes(fichier->lexemes)
	, m_racine_kuri(racine_kuri)
{
	if (m_lexemes.taille() > 0) {
		m_lexeme_courant = &m_lexemes[m_position];
	}
}

void Syntaxeuse::lance_analyse(std::ostream &os)
{
	PROFILE_FONCTION;

	m_position = 0;
	m_fichier->temps_analyse = 0.0;

	if (m_lexemes.taille() == 0) {
		return;
	}

	m_chrono_analyse.commence();

	while (!fini()) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		auto genre_lexeme = lexeme_courant()->genre;

		if (genre_lexeme == GenreLexeme::IMPORTE) {
			consomme();

			if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine littérale après 'importe'");
			}

			auto const nom_module = lexeme_courant()->chaine;
			m_fichier->modules_importes.insere(nom_module);

			/* désactive le 'chronomètre' car sinon le temps d'analyse prendra
			 * également en compte le chargement, le découpage, et l'analyse du
			 * module importé */
			m_fichier->temps_analyse += m_chrono_analyse.arrete();
			importe_module(os, m_racine_kuri, dls::chaine(nom_module), m_compilatrice, *lexeme_courant());
			m_chrono_analyse.reprend();

			consomme();
		}
		else if (genre_lexeme == GenreLexeme::CHARGE) {
			consomme();

			if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine littérale après 'charge'");
			}

			auto const nom_fichier = lexeme_courant()->chaine;

			/* désactive le 'chronomètre' car sinon le temps d'analyse prendra
			 * également en compte le chargement, le découpage, et l'analyse du
			 * fichier chargé */
			m_fichier->temps_analyse += m_chrono_analyse.arrete();
			charge_fichier(os, m_fichier->module, m_racine_kuri, dls::chaine(nom_fichier), m_compilatrice, *lexeme_courant());
			m_chrono_analyse.reprend();

			consomme();
		}
		else if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

			// noeud peut-être nul si nous avons une directive
			if (noeud != nullptr) {
				if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
					decl_var->expression_type = noeud->expression_type;
					decl_var->valeur = noeud;

					decl_var->bloc_parent->membres.pousse(decl_var);
					decl_var->bloc_parent->expressions.pousse(decl_var);
					decl_var->drapeaux |= EST_GLOBALE;
					m_compilatrice.file_typage.pousse(decl_var);
				}
				else if (est_declaration(noeud->genre)) {
					noeud->bloc_parent->membres.pousse(static_cast<NoeudDeclaration *>(noeud));
					noeud->bloc_parent->expressions.pousse(noeud);
					noeud->drapeaux |= EST_GLOBALE;
					m_compilatrice.file_typage.pousse(noeud);
				}
				else {
					noeud->bloc_parent->expressions.pousse(noeud);
					m_compilatrice.file_typage.pousse(noeud);
				}
			}
		}
		else {
			lance_erreur("attendu une expression ou une instruction");
		}
	}

	m_fichier->temps_analyse += m_chrono_analyse.arrete();
}

Lexeme Syntaxeuse::consomme()
{
	auto vieux_lexeme = m_lexemes[m_position];
	m_position += 1;

	if (!fini()) {
		m_lexeme_courant = &m_lexemes[m_position];
	}

	return vieux_lexeme;
}

Lexeme Syntaxeuse::consomme(GenreLexeme genre_lexeme, const char *message)
{
	if (m_lexemes[m_position].genre != genre_lexeme) {
		lance_erreur(message);
	}

	return consomme();
}

Lexeme *Syntaxeuse::lexeme_courant()
{
	return m_lexeme_courant;
}

const Lexeme *Syntaxeuse::lexeme_courant() const
{
	return m_lexeme_courant;
}

bool Syntaxeuse::fini() const
{
	return m_position >= m_lexemes.taille();
}

bool Syntaxeuse::apparie(GenreLexeme genre_lexeme) const
{
	return m_lexeme_courant->genre == genre_lexeme;
}

bool Syntaxeuse::apparie_expression() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::CARACTERE:
		case GenreLexeme::CHAINE_CARACTERE:
		case GenreLexeme::CHAINE_LITTERALE:
		case GenreLexeme::COROUT:
		case GenreLexeme::CROCHET_OUVRANT: // construit tableau
		case GenreLexeme::DIRECTIVE:
		case GenreLexeme::DELOGE:
		case GenreLexeme::DOLLAR:
		case GenreLexeme::EMPL:
		case GenreLexeme::ENUM:
		case GenreLexeme::ENUM_DRAPEAU:
		case GenreLexeme::ERREUR:
		case GenreLexeme::EXTERNE:
		case GenreLexeme::FAUX:
		case GenreLexeme::FONC:
		case GenreLexeme::INFO_DE:
		case GenreLexeme::INIT_DE:
		case GenreLexeme::LOGE:
		case GenreLexeme::MEMOIRE:
		case GenreLexeme::NOMBRE_ENTIER:
		case GenreLexeme::NOMBRE_REEL:
		case GenreLexeme::NON_INITIALISATION:
		case GenreLexeme::NUL:
		case GenreLexeme::OPERATEUR:
		case GenreLexeme::PARENTHESE_OUVRANTE: // expression entre parenthèse
		case GenreLexeme::RELOGE:
		case GenreLexeme::STRUCT:
		case GenreLexeme::TAILLE_DE:
		case GenreLexeme::TYPE_DE:
		case GenreLexeme::TENTE:
		case GenreLexeme::UNION:
		case GenreLexeme::VRAI:
		{
			return true;
		}
		default:
		{
			return apparie_expression_unaire() || est_identifiant_type(genre);
		}
	}
}

bool Syntaxeuse::apparie_expression_unaire() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::AROBASE:
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::MOINS:
		case GenreLexeme::PLUS:
		case GenreLexeme::TILDE:
		case GenreLexeme::FOIS:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::TROIS_POINTS:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool Syntaxeuse::apparie_expression_secondaire() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::BARRE:
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECLARATION_CONSTANTE:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::DIVISE:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::EGAL:
		case GenreLexeme::EGALITE:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::FOIS:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::MOINS:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::PARENTHESE_OUVRANTE:
		case GenreLexeme::PLUS:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::POINT:
		case GenreLexeme::POURCENT:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::TROIS_POINTS:
		case GenreLexeme::VIRGULE:
		case GenreLexeme::COMME:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool Syntaxeuse::apparie_instruction() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::ACCOLADE_OUVRANTE:
		case GenreLexeme::ARRETE:
		case GenreLexeme::BOUCLE:
		case GenreLexeme::CONTINUE:
		case GenreLexeme::DIFFERE:
		case GenreLexeme::DISCR:
		case GenreLexeme::NONSUR:
		case GenreLexeme::POUR:
		case GenreLexeme::POUSSE_CONTEXTE:
		case GenreLexeme::REPETE:
		case GenreLexeme::RETIENS:
		case GenreLexeme::RETOURNE:
		case GenreLexeme::SAUFSI:
		case GenreLexeme::SI:
		case GenreLexeme::TANTQUE:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_expression(DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	PROFILE_FONCTION;

	auto expression = analyse_expression_primaire(racine_expression, lexeme_final);

	while (apparie_expression_secondaire() && lexeme_courant()->genre != lexeme_final) {
		auto nouvelle_precedence = precedence_pour_operateur(lexeme_courant()->genre);

		if (nouvelle_precedence < donnees_precedence.precedence) {
			break;
		}

		if (nouvelle_precedence == donnees_precedence.precedence && donnees_precedence.associativite == Associativite::GAUCHE) {
			break;
		}

		auto nouvelle_associativite = associativite_pour_operateur(lexeme_courant()->genre);
		expression = analyse_expression_secondaire(expression, { nouvelle_precedence, nouvelle_associativite }, racine_expression, lexeme_final);
	}

	return expression;
}

NoeudExpression *Syntaxeuse::analyse_expression_unaire(GenreLexeme lexeme_final)
{
	PROFILE_FONCTION;

	auto lexeme = lexeme_courant();
	auto genre_noeud = GenreNoeud::OPERATEUR_UNAIRE;

	switch (lexeme->genre) {
		case GenreLexeme::MOINS:
		{
			lexeme->genre = GenreLexeme::MOINS_UNAIRE;
			break;
		}
		case GenreLexeme::PLUS:
		{
			lexeme->genre = GenreLexeme::PLUS_UNAIRE;
			break;
		}
		case GenreLexeme::TROIS_POINTS:
		{
			lexeme->genre = GenreLexeme::EXPANSION_VARIADIQUE;
			genre_noeud = GenreNoeud::EXPANSION_VARIADIQUE;
			break;
		}
		case GenreLexeme::FOIS:
		{
			lexeme->genre = GenreLexeme::FOIS_UNAIRE;
			break;
		}
		case GenreLexeme::ESPERLUETTE:
		{
			lexeme->genre = GenreLexeme::ESP_UNAIRE;
			break;
		}
		case GenreLexeme::AROBASE:
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		{
			break;
		}
		default:
		{
			assert(false);
			return nullptr;
		}
	}

	consomme();

	auto precedence = precedence_pour_operateur(lexeme->genre);
	auto associativite = associativite_pour_operateur(lexeme->genre);

	auto noeud = CREE_NOEUD(NoeudExpressionUnaire, genre_noeud, lexeme);

	// cette vérification n'est utile que pour les arguments variadiques sans type
	if (apparie_expression()) {
		noeud->expr = analyse_expression({ precedence, associativite }, GenreLexeme::INCONNU, lexeme_final);
	}

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_primaire(GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	PROFILE_FONCTION;

	if (apparie_expression_unaire()) {
		return analyse_expression_unaire(lexeme_final);
	}

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::CARACTERE:
		{
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_CARACTERE, lexeme);
		}
		case GenreLexeme::CHAINE_CARACTERE:
		{
			consomme();
			auto noeud = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme);

			if (apparie(GenreLexeme::DOUBLE_POINTS) && racine_expression != GenreLexeme::RELOGE) {
				consomme();
				noeud->expression_type = analyse_expression({ PRECEDENCE_TYPE, Associativite::GAUCHE }, GenreLexeme::DOUBLE_POINTS, GenreLexeme::INCONNU);
			}

			return noeud;
		}
		case GenreLexeme::CHAINE_LITTERALE:
		{
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_CHAINE, lexeme);
		}
		case GenreLexeme::CROCHET_OUVRANT:
		{
			consomme();
			lexeme->genre = GenreLexeme::TABLEAU;

			auto expression_entre_crochets = static_cast<NoeudExpression *>(nullptr);
			if (apparie_expression()) {
				expression_entre_crochets = analyse_expression({}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);
			}

			// point-virgule implicite dans l'expression
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}

			consomme(GenreLexeme::CROCHET_FERMANT, "Attendu un crochet fermant");

			if (apparie_expression()) {
				// nous avons l'expression d'un type
				auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, lexeme);

				if (m_noeud_logement) {
					m_noeud_logement->expr_taille = expression_entre_crochets;
				}
				else {
					noeud->expr1 = expression_entre_crochets;
				}

				noeud->expr2 = analyse_expression_primaire(GenreLexeme::INCONNU, GenreLexeme::INCONNU);

				return noeud;
			}

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU, lexeme);
			noeud->expr = expression_entre_crochets;

			return noeud;
		}
		case GenreLexeme::EMPL:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme_courant());
			noeud->drapeaux |= EMPLOYE;

			consomme();

			if (apparie(GenreLexeme::DOUBLE_POINTS)) {
				consomme();
				noeud->expression_type = analyse_expression_primaire(GenreLexeme::DOUBLE_POINTS, GenreLexeme::INCONNU);
			}

			return noeud;
		}
		case GenreLexeme::EXTERNE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme_courant());
			noeud->drapeaux |= EST_EXTERNE;

			consomme();

			if (apparie(GenreLexeme::DOUBLE_POINTS)) {
				consomme();
				noeud->expression_type = analyse_expression_primaire(GenreLexeme::DOUBLE_POINTS, GenreLexeme::INCONNU);
			}

			return noeud;
		}
		case GenreLexeme::FAUX:
		case GenreLexeme::VRAI:
		{
			lexeme->genre = GenreLexeme::BOOL;
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN, lexeme);
		}
		case GenreLexeme::INFO_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_INFO_DE, lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::INFO_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'taille_de'");

			return noeud;
		}
		case GenreLexeme::INIT_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_INIT_DE, lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::INIT_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'init_de'");

			return noeud;
		}
		case GenreLexeme::MEMOIRE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_MEMOIRE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::MEMOIRE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

			return noeud;
		}
		case GenreLexeme::NOMBRE_ENTIER:
		{
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER, lexeme);
		}
		case GenreLexeme::NOMBRE_REEL:
		{
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL, lexeme);
		}
		case GenreLexeme::NON_INITIALISATION:
		{
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::INSTRUCTION_NON_INITIALISATION, lexeme);
		}
		case GenreLexeme::NUL:
		{
			consomme();
			return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_LITTERALE_NUL, lexeme);
		}
		case GenreLexeme::OPERATEUR:
		{
			return analyse_declaration_operateur();
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_PARENTHESE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::PARENTHESE_OUVRANTE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "attenud une parenthèse fermante");

			return noeud;
		}
		case GenreLexeme::TAILLE_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_TAILLE_DE, lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::TAILLE_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

			return noeud;
		}
		case GenreLexeme::TYPE_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'type_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_TYPE_DE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::TYPE_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'type_de'");

			return noeud;
		}
		case GenreLexeme::LOGE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_LOGE, lexeme);
			m_noeud_logement = noeud;
			noeud->expression_type = analyse_expression_primaire(GenreLexeme::LOGE, GenreLexeme::INCONNU);
			m_noeud_logement = nullptr;

			if (noeud->expression_type->lexeme->genre == GenreLexeme::CHAINE) {
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une paranthèse ouvrante '(");
				noeud->expr_taille = analyse_expression({}, GenreLexeme::LOGE, GenreLexeme::INCONNU);
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu une paranthèse fermante ')");
			}

			if (apparie(GenreLexeme::SINON)) {
				consomme();
				noeud->bloc = analyse_bloc();
			}

			return noeud;
		}
		case GenreLexeme::DELOGE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_DELOGE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::DELOGE, GenreLexeme::INCONNU);
			return noeud;
		}
		case GenreLexeme::RELOGE:
		{
			consomme();

			/* reloge nom : type; */
			auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_RELOGE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::RELOGE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::DOUBLE_POINTS, "Attendu un double-points ':' après l'expression à reloger");

			m_noeud_logement = noeud;
			noeud->expression_type = analyse_expression_primaire(GenreLexeme::RELOGE, GenreLexeme::INCONNU);
			m_noeud_logement = nullptr;

			if (noeud->expression_type->lexeme->genre == GenreLexeme::CHAINE) {
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une paranthèse ouvrante '(");
				noeud->expr_taille = analyse_expression({}, GenreLexeme::RELOGE, GenreLexeme::INCONNU);
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu une paranthèse fermante ')");
			}

			if (apparie(GenreLexeme::SINON)) {
				consomme();
				noeud->bloc = analyse_bloc();
			}

			return noeud;
		}
		case GenreLexeme::TENTE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudTente, GenreNoeud::INSTRUCTION_TENTE, lexeme);
			noeud->expr_appel = analyse_expression({}, GenreLexeme::TENTE, GenreLexeme::INCONNU);

			if (apparie(GenreLexeme::PIEGE)) {
				consomme();

				if (apparie(GenreLexeme::NONATTEIGNABLE)) {
					consomme();
				}
				else {
					noeud->expr_piege = analyse_expression({}, GenreLexeme::PIEGE, GenreLexeme::INCONNU);
					noeud->bloc = analyse_bloc();
				}
			}

			return noeud;
		}
		case GenreLexeme::DIRECTIVE:
		{
			consomme();

			if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
				auto directive = lexeme_courant()->chaine;

				consomme();

				if (directive == "inclus") {
					auto chaine_inclus = lexeme_courant()->chaine;
					consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

					auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, chaine_inclus);
					m_compilatrice.ajoute_inclusion(chaine);
				}
				else if (directive == "bibliothèque_dynamique") {
					auto chaine_bib = lexeme_courant()->chaine;
					consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

					auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, chaine_bib);
					m_compilatrice.bibliotheques_dynamiques.pousse(chaine);
				}
				else if (directive == "bibliothèque_statique") {
					auto chaine_bib = lexeme_courant()->chaine;
					consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

					auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, chaine_bib);
					m_compilatrice.bibliotheques_statiques.pousse(chaine);
				}
				else if (directive == "def") {
					auto chaine = lexeme_courant()->chaine;
					consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

					m_compilatrice.definitions.pousse(chaine);
				}
				else if (directive == "exécute") {
					auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::DIRECTIVE_EXECUTION, lexeme);
					noeud->expr = analyse_expression({}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);

					m_compilatrice.noeuds_a_executer.pousse(noeud);

					return noeud;
				}
				else if (directive == "chemin") {
					auto chaine = lexeme_courant()->chaine;
					consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

					m_compilatrice.chemins.pousse(chaine);
				}
				else if (directive == "nulctx") {
					lexeme = lexeme_courant();
					auto noeud_fonc = analyse_declaration_fonction(lexeme);
					noeud_fonc->drapeaux |= FORCE_NULCTX;
					return noeud_fonc;
				}
				else {
					lance_erreur("Directive inconnue");
				}
			}
			else {
				lance_erreur("Directive inconnue");
			}

			return nullptr;
		}
		case GenreLexeme::DOLLAR:
		{
			consomme();
			lexeme = lexeme_courant();

			consomme(GenreLexeme::CHAINE_CARACTERE, "attendu une chaine de caractère après '$'");

			auto noeud = CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme);
			noeud->drapeaux |= DECLARATION_TYPE_POLYMORPHIQUE;
			return noeud;
		}
		case GenreLexeme::FONC:
		{
			return analyse_declaration_fonction(lexeme);
		}
		/* Ceux-ci doivent déjà avoir été gérés. */
		case GenreLexeme::COROUT:
		case GenreLexeme::ENUM:
		case GenreLexeme::ENUM_DRAPEAU:
		case GenreLexeme::ERREUR:
		case GenreLexeme::STRUCT:
		case GenreLexeme::UNION:
		{
			std::cerr << "Ceux-ci doivent déjà avoir été gérés.\n";
			assert(false);
			return nullptr;
		}
		default:
		{
			if (est_identifiant_type(lexeme->genre)) {
				consomme();
				return CREE_NOEUD(NoeudExpression, GenreNoeud::EXPRESSION_REFERENCE_TYPE, lexeme);
			}

			lance_erreur("attendu une expression primaire");
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_expression_secondaire(NoeudExpression *gauche, const DonneesPrecedence &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	PROFILE_FONCTION;

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::BARRE:
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::DIVISE:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::EGALITE:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::FOIS:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::MOINS:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::PLUS:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::POURCENT:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::VIRGULE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::CROCHET_OUVRANT:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_INDEXAGE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression({}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);

			consomme(GenreLexeme::CROCHET_FERMANT, "attendu un crochet fermant");

			return noeud;
		}
		case GenreLexeme::DECLARATION_CONSTANTE:
		{
			consomme();

			switch (lexeme_courant()->genre) {
				default:
				{
					break;
				}
				case GenreLexeme::COROUT:
				case GenreLexeme::FONC:
				{
					return analyse_declaration_fonction(gauche->lexeme);
				}
				case GenreLexeme::STRUCT:
				case GenreLexeme::UNION:
				{
					return analyse_declaration_structure(gauche);
				}
				case GenreLexeme::ENUM:
				case GenreLexeme::ENUM_DRAPEAU:
				case GenreLexeme::ERREUR:
				{
					return analyse_declaration_enum(gauche);
				}
			}

			auto noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, lexeme);
			noeud->valeur = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			noeud->drapeaux |= EST_CONSTANTE;
			gauche->drapeaux |= EST_CONSTANTE;

			return noeud;
		}
		case GenreLexeme::DECLARATION_VARIABLE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, lexeme);
			noeud->ident = gauche->ident;
			noeud->valeur = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::EGAL:
		{
			consomme();

			if (gauche->expression_type != nullptr) {
				auto noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, lexeme);
				noeud->ident = gauche->ident;
				noeud->expression_type = gauche->expression_type;
				noeud->valeur = gauche;
				noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
				return noeud;
			}

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::POINT:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionMembre, GenreNoeud::EXPRESSION_REFERENCE_MEMBRE, lexeme);
			noeud->accede = gauche;
			noeud->membre = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::TROIS_POINTS:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_PLAGE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		{
			return analyse_appel_fonction(gauche);
		}
		case GenreLexeme::COMME:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_COMME, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression_primaire(GenreLexeme::COMME, GenreLexeme::INCONNU);
			return noeud;
		}
		default:
		{
			assert(false);
			return nullptr;
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_instruction()
{
	PROFILE_FONCTION;

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::ACCOLADE_OUVRANTE:
		{
			return analyse_bloc();
		}
		case GenreLexeme::DIFFERE:
		{
			consomme();
			auto bloc = analyse_bloc();
			bloc->est_differe = true;
			return bloc;
		}
		case GenreLexeme::NONSUR:
		{
			consomme();
			auto bloc = analyse_bloc();
			bloc->est_nonsur = true;
			return bloc;
		}
		case GenreLexeme::ARRETE:
		case GenreLexeme::CONTINUE:
		{
			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, lexeme);
			consomme();

			if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
				noeud->expr = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme_courant());
				consomme();
			}

			return noeud;
		}
		case GenreLexeme::RETIENS:
		{
			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_RETIENS, lexeme);
			consomme();

			if (apparie_expression()) {
				noeud->expr = analyse_expression({}, GenreLexeme::RETIENS, GenreLexeme::INCONNU);
			}

			return noeud;
		}
		case GenreLexeme::RETOURNE:
		{
			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_RETOUR, lexeme);
			consomme();

			if (apparie_expression()) {
				noeud->expr = analyse_expression({}, GenreLexeme::RETOURNE, GenreLexeme::INCONNU);
			}

			return noeud;
		}
		case GenreLexeme::BOUCLE:
		{
			return analyse_instruction_boucle();
		}
		case GenreLexeme::DISCR:
		{
			return analyse_instruction_discr();
		}
		case GenreLexeme::POUR:
		{
			return analyse_instruction_pour();
		}
		case GenreLexeme::POUSSE_CONTEXTE:
		{
			return analyse_instruction_pousse_contexte();
		}
		case GenreLexeme::REPETE:
		{
			return analyse_instruction_repete();
		}
		case GenreLexeme::SAUFSI:
		{
			return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
		}
		case GenreLexeme::SI:
		{
			return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
		}
		case GenreLexeme::TANTQUE:
		{
			return analyse_instruction_tantque();
		}
		default:
		{
			assert(false);
			return nullptr;
		}
	}
}

NoeudBloc *Syntaxeuse::analyse_bloc()
{
	PROFILE_FONCTION;

	empile_etat("dans l'analyse du bloc", lexeme_courant());

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{'");

	auto bloc = m_fichier->module->assembleuse->empile_bloc();
	auto expressions = dls::tablet<NoeudExpression *, 32>();
	auto membres = dls::tablet<NoeudDeclaration *, 32>();

	while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (apparie_instruction()) {
			auto noeud = analyse_instruction();
			expressions.pousse(noeud);
		}
		else if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

			if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
				decl_var->expression_type = noeud->expression_type;
				decl_var->valeur = noeud;
				membres.pousse(decl_var);
				expressions.pousse(decl_var);
			}
			else if (est_declaration(noeud->genre)) {
				membres.pousse(static_cast<NoeudDeclaration *>(noeud));
				expressions.pousse(noeud);
			}
			else {
				expressions.pousse(noeud);
			}
		}
		else {
			lance_erreur("attendu une expression ou une instruction");
		}
	}

	copie_tablet_tableau(membres, bloc->membres);
	copie_tablet_tableau(expressions, bloc->expressions);
	m_fichier->module->assembleuse->depile_bloc();

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}'");

	depile_etat();

	return bloc;
}

NoeudExpression *Syntaxeuse::analyse_appel_fonction(NoeudExpression *gauche)
{
	PROFILE_FONCTION;

	auto noeud = CREE_NOEUD(NoeudExpressionAppel, GenreNoeud::EXPRESSION_APPEL_FONCTION, lexeme_courant());
	noeud->appelee = gauche;

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "attendu une parenthèse ouvrante");

	auto params = dls::tablet<NoeudExpression *, 16>();

	while (apparie_expression()) {
		auto expr = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
		params.pousse(expr);

		/* point-virgule implicite */
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "attenu ')' à la fin des argument de l'appel");

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_boucle()
{
	PROFILE_FONCTION;

	auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_BOUCLE, lexeme_courant());
	consomme();
	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;
	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_discr()
{
	PROFILE_FONCTION;

	auto noeud_discr = CREE_NOEUD(NoeudDiscr, GenreNoeud::INSTRUCTION_DISCR, lexeme_courant());
	consomme();

	noeud_discr->expr = analyse_expression({}, GenreLexeme::DISCR, GenreLexeme::INCONNU);

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

	auto sinon_rencontre = false;

	auto paires_discr = dls::tablet<std::pair<NoeudExpression *, NoeudBloc *>, 32>();

	while (!apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::SINON)) {
			consomme();

			if (sinon_rencontre) {
				lance_erreur("Redéfinition d'un bloc sinon");
			}

			sinon_rencontre = true;

			noeud_discr->bloc_sinon = analyse_bloc();
		}
		else {
			auto expr = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
			auto bloc = analyse_bloc();

			paires_discr.pousse({ expr, bloc });
		}
	}

	copie_tablet_tableau(paires_discr, noeud_discr->paires_discr);

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

	return noeud_discr;
}

NoeudExpression *Syntaxeuse::analyse_instruction_pour()
{
	PROFILE_FONCTION;

	auto noeud = CREE_NOEUD(NoeudPour, GenreNoeud::INSTRUCTION_POUR, lexeme_courant());
	consomme();

	auto expression = analyse_expression({}, GenreLexeme::POUR, GenreLexeme::INCONNU);

	if (apparie(GenreLexeme::DANS)) {
		consomme();
		noeud->variable = expression;
		noeud->expression = analyse_expression({}, GenreLexeme::DANS, GenreLexeme::INCONNU);
	}
	else {
		static Lexeme lexeme_it = { "it", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
		auto noeud_it = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, &lexeme_it);

		static Lexeme lexeme_index = { "index_it", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
		auto noeud_index = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, &lexeme_index);

		static Lexeme lexeme_virgule = { ",", {}, GenreLexeme::VIRGULE, 0, 0, 0 };
		auto noeud_virgule = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, &lexeme_virgule);
		noeud_virgule->expr1 = noeud_it;
		noeud_virgule->expr2 = noeud_index;

		noeud->variable = noeud_virgule;
		noeud->expression = expression;
	}

	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	if (apparie(GenreLexeme::SANSARRET)) {
		consomme();
		noeud->bloc_sansarret = analyse_bloc();
	}

	if (apparie(GenreLexeme::SINON)) {
		consomme();
		noeud->bloc_sinon = analyse_bloc();
	}

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_pousse_contexte()
{
	PROFILE_FONCTION;

	auto noeud = CREE_NOEUD(NoeudPousseContexte, GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE, lexeme_courant());
	consomme();

	noeud->expr = analyse_expression({}, GenreLexeme::POUSSE_CONTEXTE, GenreLexeme::INCONNU);
	noeud->bloc = analyse_bloc();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_repete()
{
	PROFILE_FONCTION;

	auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_REPETE, lexeme_courant());
	consomme();

	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	consomme(GenreLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

	noeud->condition = analyse_expression({}, GenreLexeme::REPETE, GenreLexeme::INCONNU);

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si(GenreNoeud genre_noeud)
{
	PROFILE_FONCTION;

	empile_etat("dans l'analyse de l'instruction si", lexeme_courant());

	auto noeud = CREE_NOEUD(NoeudSi, genre_noeud, lexeme_courant());
	consomme();

	noeud->condition = analyse_expression({}, GenreLexeme::SI, GenreLexeme::INCONNU);

	noeud->bloc_si_vrai = analyse_bloc();

	if (apparie(GenreLexeme::SINON)) {
		consomme();

		/* Si le 'sinon' contient un « si » ou un « saufsi », nous ajoutons un
		 * bloc pour créer un niveau d'indirection. Car dans le cas où nous
		 * avons un contrôle du type si/sinon si dans une boucle, la génération
		 * de blocs LLVM dans l'arbre syntactic devient plus compliquée sans
		 * cette indirection : certaines instructions de branchage ne sont pas
		 * ajoutées alors qu'elles devraient l'être et la logique pour
		 * correctement traiter ce cas sans l'indirection semble être complexe.
		 * LLVM devrait pouvoir effacer cette indirection en enlevant les
		 * branchements redondants.
		 */

		if (apparie(GenreLexeme::SI)) {
			noeud->bloc_si_faux = m_fichier->module->assembleuse->empile_bloc();

			auto noeud_si = analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
			noeud->bloc_si_faux->expressions.pousse(noeud_si);

			m_fichier->module->assembleuse->depile_bloc();
		}
		else if (apparie(GenreLexeme::SAUFSI)) {
			noeud->bloc_si_faux = m_fichier->module->assembleuse->empile_bloc();

			auto noeud_saufsi = analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
			noeud->bloc_si_faux->expressions.pousse(noeud_saufsi);

			m_fichier->module->assembleuse->depile_bloc();
		}
		else {
			noeud->bloc_si_faux = analyse_bloc();
		}
	}

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_tantque()
{
	PROFILE_FONCTION;

	auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_TANTQUE, lexeme_courant());
	consomme();

	noeud->condition = analyse_expression({}, GenreLexeme::TANTQUE, GenreLexeme::INCONNU);
	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_enum(NoeudExpression *gauche)
{
	PROFILE_FONCTION;

	auto lexeme = lexeme_courant();
	empile_etat("dans le syntaxage de l'énumération", lexeme);
	consomme();

	auto noeud_decl = CREE_NOEUD(NoeudEnum, GenreNoeud::DECLARATION_ENUM, gauche->lexeme);

	if (lexeme->genre != GenreLexeme::ERREUR) {
		if (!apparie(GenreLexeme::ACCOLADE_OUVRANTE)) {
			noeud_decl->expression_type = analyse_expression_primaire(GenreLexeme::ENUM, GenreLexeme::INCONNU);
		}

		auto type = m_compilatrice.typeuse.reserve_type_enum(noeud_decl);
		type->est_drapeau = lexeme->genre == GenreLexeme::ENUM_DRAPEAU;
		noeud_decl->type = type;
	}
	else {
		auto type = m_compilatrice.typeuse.reserve_type_erreur(noeud_decl);
		type->est_erreur = true;
		noeud_decl->type = type;
	}

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

	auto bloc = m_fichier->module->assembleuse->empile_bloc();

	auto membres = dls::tablet<NoeudDeclaration *, 16>();
	auto expressions = dls::tablet<NoeudExpression *, 16>();

	while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

			if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
				decl_var->expression_type = noeud->expression_type;
				decl_var->valeur = noeud;

				membres.pousse(decl_var);
				expressions.pousse(decl_var);
			}
			else {
				membres.pousse(static_cast<NoeudDeclaration *>(noeud));
				expressions.pousse(noeud);
			}
		}
	}

	copie_tablet_tableau(membres, bloc->membres);
	copie_tablet_tableau(expressions, bloc->expressions);

	m_fichier->module->assembleuse->depile_bloc();
	noeud_decl->bloc = bloc;

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

	depile_etat();

	return noeud_decl;
}

NoeudExpression *Syntaxeuse::analyse_declaration_fonction(Lexeme const *lexeme)
{
	PROFILE_FONCTION;

	auto lexeme_mot_cle = lexeme_courant();

	empile_etat("dans le syntaxage de la fonction", lexeme_mot_cle);
	consomme();

	auto externe = false;

	if (apparie(GenreLexeme::EXTERNE)) {
		consomme();
		externe = true;

		if (lexeme_mot_cle->genre == GenreLexeme::COROUT && externe) {
			lance_erreur("Une coroutine ne peut pas être externe");
		}
	}

	auto noeud = CREE_NOEUD(NoeudDeclarationFonction, GenreNoeud::DECLARATION_FONCTION, lexeme);
	noeud->est_coroutine = lexeme_mot_cle->genre == GenreLexeme::COROUT;

	if (externe) {
		noeud->drapeaux |= EST_EXTERNE;
		noeud->est_externe = true;
	}

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	/* analyse les paramètres de la fonction */
	auto params = dls::tablet<NoeudDeclaration *, 16>();

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

		if (param->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, param->lexeme);
			decl_var->expression_type = param->expression_type;
			decl_var->valeur = param;

			params.pousse(decl_var);
		}
		else {
			params.pousse(static_cast<NoeudDeclaration *>(param));
		}

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

	POUR (noeud->params) {
		aplatis_arbre(it, noeud->arbre_aplatis_entete, drapeaux_noeud::AUCUN);
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction */
	if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
		// nous avons la déclaration d'un type
		noeud->est_declaration_type = true;
		consomme();

		while (true) {
			auto type_declare = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::INCONNU);
			noeud->params_sorties.pousse(type_declare);
			aplatis_arbre(type_declare, noeud->arbre_aplatis_entete, drapeaux_noeud::AUCUN);

			if (!apparie(GenreLexeme::VIRGULE)) {
				break;
			}

			consomme();
		}

		consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante");
	}
	else {
		// nous avons la déclaration d'une fonction
		if (apparie(GenreLexeme::RETOUR_TYPE)) {
			consomme();

			while (true) {
				noeud->noms_retours.pousse("__ret" + dls::vers_chaine(noeud->noms_retours.taille));

				auto type_declare = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::INCONNU);
				noeud->params_sorties.pousse(type_declare);
				aplatis_arbre(type_declare, noeud->arbre_aplatis_entete, drapeaux_noeud::AUCUN);

				if (!apparie(GenreLexeme::VIRGULE)) {
					break;
				}

				consomme();
			}
		}
		else {
			static const Lexeme lexeme_rien = { "rien", {}, GenreLexeme::RIEN, 0, 0, 0 };
			auto type_declare = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_TYPE, &lexeme_rien);
			noeud->noms_retours.pousse("__ret0");
			noeud->params_sorties.pousse(type_declare);
			aplatis_arbre(type_declare, noeud->arbre_aplatis_entete, drapeaux_noeud::AUCUN);
		}

		while (apparie(GenreLexeme::DIRECTIVE)) {
			consomme();

			auto chn_directive = lexeme_courant()->chaine;

			if (chn_directive == "enligne") {
				noeud->drapeaux |= FORCE_ENLIGNE;
			}
			else if (chn_directive == "horsligne") {
				noeud->drapeaux |= FORCE_HORSLIGNE;
			}
			else if (chn_directive == "nulctx") {
				noeud->drapeaux |= FORCE_NULCTX;
				noeud->drapeaux |= FORCE_SANSTRACE;
			}
			else if (chn_directive == "externe") {
				noeud->drapeaux |= EST_EXTERNE;
				noeud->est_externe = true;
			}
			else if (chn_directive == "sanstrace") {
				noeud->drapeaux |= FORCE_SANSTRACE;
			}

			consomme();
		}

		if (externe) {
			consomme(GenreLexeme::POINT_VIRGULE, "Attendu un point-virgule ';' après la déclaration de la fonction externe");

			if (noeud->params_sorties.taille > 1) {
				lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
			}
		}
		else {
			/* ignore les points-virgules implicites */
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}

			auto nombre_noeuds_alloues = m_compilatrice.allocatrice_noeud.nombre_noeuds();
			noeud->bloc = analyse_bloc();
			nombre_noeuds_alloues = m_compilatrice.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;

			/* À FAIRE : quand nous aurons des fonctions dans des fonctions, il
			 * faudra soustraire le nombre de noeuds des fonctions enfants. Il
			 * faudra également faire attention au moultfilage futur.
			 */
			noeud->arbre_aplatis.reserve(static_cast<long>(nombre_noeuds_alloues));
			aplatis_arbre(noeud->bloc, noeud->arbre_aplatis, drapeaux_noeud::AUCUN);

//			std::cerr << "Abre aplatis pour fonction " << noeud->ident->nom << " :\n";

//			POUR (noeud->arbre_aplatis) {
//				std::cerr << "-- " << chaine_genre_noeud(it->genre) << '\n';
//			}
		}

		m_fichier->module->fonctions_exportees.insere(lexeme->chaine);
	}

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_operateur()
{
	PROFILE_FONCTION;

	empile_etat("dans le syntaxage de l'opérateur", lexeme_courant());
	consomme();

	auto lexeme = lexeme_courant();
	auto genre_operateur = lexeme->genre;

	if (!est_operateur_surchargeable(genre_operateur)) {
		lance_erreur("L'opérateur n'est pas surchargeable");
	}

	consomme();

	// :: fonc
	consomme(GenreLexeme::DECLARATION_CONSTANTE, "Attendu :: après la déclaration de l'opérateur");
	consomme(GenreLexeme::FONC, "Attendu fonc après ::");

	auto noeud = CREE_NOEUD(NoeudDeclarationFonction, GenreNoeud::DECLARATION_OPERATEUR, lexeme);

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	/* analyse les paramètres de la fonction */
	auto params = dls::tablet<NoeudDeclaration *, 16>();

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

		if (param->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
			auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, param->lexeme);
			decl_var->expression_type = param->expression_type;
			decl_var->valeur = param;

			params.pousse(decl_var);
		}
		else {
			params.pousse(static_cast<NoeudDeclaration *>(param));
		}

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

	if (noeud->params.taille > 2) {
		erreur::lance_erreur(
					"La surcharge d'opérateur ne peut prendre au plus 2 paramètres",
					m_compilatrice,
					lexeme);
	}
	else if (noeud->params.taille == 1) {
		if (genre_operateur == GenreLexeme::PLUS) {
			lexeme->genre = GenreLexeme::PLUS_UNAIRE;
		}
		else if (genre_operateur == GenreLexeme::MOINS) {
			lexeme->genre = GenreLexeme::MOINS_UNAIRE;
		}
		else if (genre_operateur != GenreLexeme::TILDE && genre_operateur != GenreLexeme::EXCLAMATION) {
			erreur::lance_erreur(
						"La surcharge d'opérateur unaire n'est possible que pour '+', '-', '~', ou '!'",
						m_compilatrice,
						lexeme);
		}
	}

	POUR (noeud->params) {
		aplatis_arbre(it, noeud->arbre_aplatis_entete, drapeaux_noeud::AUCUN);
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction */
	consomme(GenreLexeme::RETOUR_TYPE, "Attendu un retour de type");

	while (true) {
		noeud->noms_retours.pousse("__ret" + dls::vers_chaine(noeud->noms_retours.taille));

		auto type_declare = analyse_expression_primaire(GenreLexeme::OPERATEUR, GenreLexeme::INCONNU);
		noeud->params_sorties.pousse(type_declare);
		aplatis_arbre(type_declare, noeud->arbre_aplatis_entete, drapeaux_noeud::AUCUN);

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	if (noeud->params_sorties.taille > 1) {
		lance_erreur("Il est impossible d'avoir plusieurs de sortie pour un opérateur");
	}

	while (apparie(GenreLexeme::DIRECTIVE)) {
		consomme();
		consomme();

		auto chn_directive = lexeme_courant()->chaine;

		if (chn_directive == "enligne") {
			noeud->drapeaux |= FORCE_ENLIGNE;
		}
		else if (chn_directive == "horsligne") {
			noeud->drapeaux |= FORCE_HORSLIGNE;
		}
		else if (chn_directive == "nulctx") {
			noeud->drapeaux |= FORCE_NULCTX;
		}
		else if (chn_directive == "externe") {
			noeud->drapeaux |= EST_EXTERNE;
			noeud->est_externe = true;
		}
	}

	/* ignore les points-virgules implicites */
	if (apparie(GenreLexeme::POINT_VIRGULE)) {
		consomme();
	}

	noeud->bloc = analyse_bloc();
	aplatis_arbre(noeud->bloc, noeud->arbre_aplatis, drapeaux_noeud::AUCUN);

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_structure(NoeudExpression *gauche)
{
	PROFILE_FONCTION;

	auto lexeme_mot_cle = lexeme_courant();
	empile_etat("dans le syntaxage de la structure", lexeme_mot_cle);
	consomme();

	auto noeud_decl = CREE_NOEUD(NoeudStruct, GenreNoeud::DECLARATION_STRUCTURE, gauche->lexeme);
	noeud_decl->est_union = (lexeme_mot_cle->genre == GenreLexeme::UNION);

	if (gauche->lexeme->chaine == "InfoType") {
		noeud_decl->type = m_compilatrice.typeuse.type_info_type_;
		m_compilatrice.typeuse.type_info_type_->decl = noeud_decl;
		m_compilatrice.typeuse.type_info_type_->nom = noeud_decl->ident->nom;
	}
	else {
		if (noeud_decl->est_union) {
			noeud_decl->type = m_compilatrice.typeuse.reserve_type_union(noeud_decl);
		}
		else {
			noeud_decl->type = m_compilatrice.typeuse.reserve_type_structure(noeud_decl);
		}
	}

	if (gauche->lexeme->chaine == "ContexteProgramme") {
		m_compilatrice.type_contexte = noeud_decl->type;
	}

	if (apparie(GenreLexeme::EXTERNE)) {
		noeud_decl->est_externe = true;
		consomme();
	}

	if (apparie(GenreLexeme::NONSUR)) {
		noeud_decl->est_nonsure = true;
		consomme();
	}

	auto analyse_membres = true;

	if (noeud_decl->est_externe) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			analyse_membres = false;
		}
	}

	if (analyse_membres) {
		auto bloc = m_fichier->module->assembleuse->empile_bloc();
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

		auto membres = dls::tablet<NoeudDeclaration *, 16>();
		auto expressions = dls::tablet<NoeudExpression *, 16>();

		while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
				continue;
			}

			if (apparie_expression()) {
				auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

				if (noeud->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					auto decl_var = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, noeud->lexeme);
					decl_var->expression_type = noeud->expression_type;
					decl_var->valeur = noeud;

					membres.pousse(decl_var);
					expressions.pousse(decl_var);
				}
				else {
					membres.pousse(static_cast<NoeudDeclaration *>(noeud));
					expressions.pousse(noeud);
				}
			}
			else {
				lance_erreur("attendu une expression ou une instruction");
			}
		}

		copie_tablet_tableau(membres, bloc->membres);
		copie_tablet_tableau(expressions, bloc->expressions);

		POUR (bloc->membres) {
			aplatis_arbre(it, noeud_decl->arbre_aplatis, drapeaux_noeud::AUCUN);
		}

		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de la structure");

		m_fichier->module->assembleuse->depile_bloc();
		noeud_decl->bloc = bloc;
	}

	depile_etat();

	return noeud_decl;
}

void Syntaxeuse::lance_erreur(const dls::chaine &quoi, erreur::type_erreur type)
{
	auto lexeme = lexeme_courant();
	auto fichier = m_compilatrice.fichier(static_cast<size_t>(lexeme->fichier));

	auto flux = dls::flux_chaine();

	flux << "\n";
	flux << fichier->chemin << ':' << lexeme->ligne + 1 << " : erreur de syntaxage :\n";

	POUR (m_donnees_etat_syntaxage) {
		erreur::imprime_ligne_avec_message(flux, fichier, it.lexeme, it.message);
	}

	erreur::imprime_ligne_avec_message(flux, fichier, lexeme, quoi.c_str());

	throw erreur::frappe(flux.chn().c_str(), type);
}

void Syntaxeuse::empile_etat(const char *message, Lexeme *lexeme)
{
	m_donnees_etat_syntaxage.pousse({ lexeme, message });
}

void Syntaxeuse::depile_etat()
{
	m_donnees_etat_syntaxage.pop_back();
}
