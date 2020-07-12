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

#pragma once

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "erreur.h"
#include "lexemes.hh"

struct Compilatrice;
struct Fichier;
struct NoeudBloc;
struct NoeudExpression;

enum class GenreNoeud : char;

enum class Associativite : int {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	int precedence = 0;
	Associativite associativite = Associativite::GAUCHE;
};

struct Syntaxeuse {
private:
	Compilatrice &m_compilatrice;
	Fichier *m_fichier = nullptr;
	dls::tableau<Lexeme> &m_lexemes;
	long m_position = 0;

	dls::chrono::metre_seconde m_chrono_analyse{};

	Lexeme *m_lexeme_courant{};

	// pour séparer les expression de la taille des tableaux pour les expressions loge et reloge
	NoeudExpressionLogement *m_noeud_logement = nullptr;

	dls::chaine m_racine_kuri{};

	/* Pour les messages d'erreurs. */
	struct DonneesEtatSyntaxage {
		Lexeme *lexeme = nullptr;
		const char *message = nullptr;

		DonneesEtatSyntaxage() = default;

		COPIE_CONSTRUCT(DonneesEtatSyntaxage);
	};

	dls::tablet<DonneesEtatSyntaxage, 33> m_donnees_etat_syntaxage{};

public:
	Syntaxeuse(Compilatrice &compilatrice,
			   Fichier *fichier,
			   dls::chaine const &racine_kuri);

	COPIE_CONSTRUCT(Syntaxeuse);

	void lance_analyse();

private:
	Lexeme consomme();

	Lexeme consomme(GenreLexeme genre_lexeme, const char *message);

	Lexeme *lexeme_courant();
	Lexeme const *lexeme_courant() const;

	bool fini() const;

	bool apparie(GenreLexeme genre_lexeme) const;
	bool apparie_expression() const;
	bool apparie_expression_unaire() const;
	bool apparie_expression_secondaire() const;
	bool apparie_instruction() const;

	/* NOTE: racine_expression n'est pour le moment utilisé que pour éviter de consommer les expressions des types pour les expressions de relogement. */
	/* NOTE: lexeme_final n'est utilisé que pour éviter de traiter les virgules comme des opérateurs dans les expressions des appels et déclarations de paramètres de fonctions. */
	NoeudExpression *analyse_expression(DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final);
	NoeudExpression *analyse_expression_unaire(GenreLexeme lexeme_final);
	NoeudExpression *analyse_expression_primaire(GenreLexeme racine_expression, GenreLexeme lexeme_final);
	NoeudExpression *analyse_expression_secondaire(NoeudExpression *gauche, DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final);

	NoeudBloc *analyse_bloc();

	NoeudExpression *analyse_appel_fonction(NoeudExpression *gauche);

	NoeudExpression *analyse_declaration_enum(NoeudExpression *gauche);
	NoeudExpression *analyse_declaration_fonction(Lexeme const *lexeme);
	NoeudExpression *analyse_declaration_operateur();
	NoeudExpression *analyse_declaration_structure(NoeudExpression *gauche);

	NoeudExpression *analyse_instruction();
	NoeudExpression *analyse_instruction_boucle();
	NoeudExpression *analyse_instruction_discr();
	NoeudExpression *analyse_instruction_pour();
	NoeudExpression *analyse_instruction_pousse_contexte();
	NoeudExpression *analyse_instruction_repete();
	NoeudExpression *analyse_instruction_si(GenreNoeud genre_noeud);
	NoeudExpression *analyse_instruction_tantque();

	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaine passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance Lexeme lui correspondant.
	 */
	[[noreturn]] void lance_erreur(
			const dls::chaine &quoi,
			erreur::type_erreur type = erreur::type_erreur::SYNTAXAGE);

	void empile_etat(const char *message, Lexeme *lexeme);

	void depile_etat();
};
