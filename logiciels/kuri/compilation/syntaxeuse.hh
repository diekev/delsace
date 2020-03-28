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

#pragma once

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/langage/analyseuse.hh"

#include "arbre_syntactic.h"
#include "erreur.h"

struct ContexteGenerationCode;

class Syntaxeuse : public lng::analyseuse<DonneesLexeme> {
	ContexteGenerationCode &m_contexte;

	long m_profondeur = 0;

	dls::chaine m_racine_kuri{};

	Fichier *m_fichier;

	dls::chrono::metre_seconde m_chrono_analyse{};

	bool m_global = false;

public:
	Syntaxeuse(
			ContexteGenerationCode &contexte,
			Fichier *fichier,
			dls::chaine const &racine_kuri);

	/* Désactive la copie, car il ne peut y avoir qu'une seule analyseuse par
	 * module. */
	Syntaxeuse(Syntaxeuse const &) = delete;
	Syntaxeuse &operator=(Syntaxeuse const &) = delete;

	void lance_analyse(std::ostream &os) override;

private:
	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaine passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesLexeme lui correspondant.
	 */
	[[noreturn]] void lance_erreur(
			const dls::chaine &quoi,
			erreur::type_erreur type = erreur::type_erreur::NORMAL);

	void analyse_expression_haut_niveau(std::ostream &os);
	NoeudExpression *analyse_declaration_fonction(GenreLexeme id, DonneesLexeme &lexeme);
	void analyse_corps_fonction();
	NoeudBloc *analyse_bloc();
	NoeudExpression *analyse_expression(GenreLexeme identifiant_final, GenreLexeme racine_expr);
	NoeudExpression *analyse_appel_fonction(DonneesLexeme &lexeme);
	NoeudExpression *analyse_declaration_structure(GenreLexeme id, DonneesLexeme &lexeme);
	NoeudExpression *analyse_declaration_enum(GenreLexeme genre, DonneesLexeme &lexeme);
	DonneesTypeDeclare analyse_declaration_type(bool double_point = true);
	DonneesTypeDeclare analyse_declaration_type_ex();
	void analyse_controle_si(GenreNoeud tn);
	void analyse_controle_pour();
	NoeudExpression *analyse_construction_structure(DonneesLexeme &lexeme);
	void analyse_directive_si();
	NoeudExpression *analyse_declaration_operateur();

	void consomme(GenreLexeme id, const char *message);
	void consomme_type(const char *message);
};
