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
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct NoeudExpressionReference;
struct NoeudExpressionVirgule;
struct NoeudPour;
struct NoeudStruct;
struct Tacheronne;
struct UniteCompilation;

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
	Tacheronne &m_tacheronne;
	Fichier *m_fichier = nullptr;
	UniteCompilation *m_unite = nullptr;
	dls::tableau<Lexeme> &m_lexemes;
	long m_position = 0;

	dls::chrono::metre_seconde m_chrono_analyse{};

	Lexeme *m_lexeme_courant{};

	/* Pour les messages d'erreurs. */
	struct DonneesEtatSyntaxage {
		Lexeme *lexeme = nullptr;
		const char *message = nullptr;
	};

	dls::tablet<DonneesEtatSyntaxage, 33> m_donnees_etat_syntaxage{};

	NoeudExpressionVirgule *m_noeud_expression_virgule = nullptr;

	bool est_dans_fonction = false;
	NoeudDeclarationEnteteFonction *fonction_courante = nullptr;
	NoeudStruct *structure_courante = nullptr;

public:
	Syntaxeuse(Tacheronne &tacheronne, UniteCompilation *unite);

	COPIE_CONSTRUCT(Syntaxeuse);

	void lance_analyse();

private:
	inline Lexeme consomme()
	{
		auto vieux_lexeme = m_lexemes[m_position];
		m_position += 1;

		if (!fini()) {
			m_lexeme_courant = &m_lexemes[m_position];
		}

		return vieux_lexeme;
	}

	inline Lexeme consomme(GenreLexeme genre_lexeme, const char *message)
	{
		if (m_lexemes[m_position].genre != genre_lexeme) {
			lance_erreur(message);
		}

		return consomme();
	}

	inline Lexeme *lexeme_courant()
	{
		return m_lexeme_courant;
	}

	inline Lexeme const *lexeme_courant() const
	{
		return m_lexeme_courant;
	}

	inline bool fini() const
	{
		return m_position >= m_lexemes.taille();
	}

	inline bool apparie(GenreLexeme genre_lexeme) const
	{
		return m_lexeme_courant->genre == genre_lexeme;
	}

	bool apparie_expression() const;
	bool apparie_expression_unaire() const;
	bool apparie_expression_secondaire() const;
	bool apparie_instruction() const;

	/* NOTE: lexeme_final n'est utilisé que pour éviter de traiter les virgules comme des opérateurs dans les expressions des appels et déclarations de paramètres de fonctions. */
	NoeudExpression *analyse_expression(DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final);
	NoeudExpression *analyse_expression_unaire(GenreLexeme lexeme_final);
	NoeudExpression *analyse_expression_primaire(GenreLexeme racine_expression, GenreLexeme lexeme_final);
	NoeudExpression *analyse_expression_secondaire(NoeudExpression *gauche, DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final);

	NoeudBloc *analyse_bloc(bool accolade_requise = true, bool pour_pousse_contexte = false);

	NoeudExpression *analyse_appel_fonction(NoeudExpression *gauche);

	NoeudExpression *analyse_declaration_enum(NoeudExpression *gauche);
	NoeudDeclarationEnteteFonction *analyse_declaration_fonction(Lexeme const *lexeme);
	NoeudExpression *analyse_declaration_operateur();
	NoeudExpression *analyse_declaration_structure(NoeudExpression *gauche);

	NoeudExpression *analyse_instruction();
	NoeudExpression *analyse_instruction_boucle();
	NoeudExpression *analyse_instruction_discr();
	NoeudExpression *analyse_instruction_pour();
	void analyse_specifiants_instruction_pour(NoeudPour *noeud);
	NoeudExpression *analyse_instruction_pousse_contexte();
	NoeudExpression *analyse_instruction_repete();
	NoeudExpression *analyse_instruction_si(GenreNoeud genre_noeud);
	NoeudExpression *analyse_instruction_si_statique(Lexeme *lexeme);
	NoeudExpression *analyse_instruction_tantque();

	/**
	 * Lance une exception de type ErreurSyntaxique contenant la chaine passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance Lexeme lui correspondant.
	 */
	[[noreturn]] void lance_erreur(const dls::chaine &quoi,
			erreur::Genre genre = erreur::Genre::SYNTAXAGE);

	void empile_etat(const char *message, Lexeme *lexeme);

	void depile_etat();
};
