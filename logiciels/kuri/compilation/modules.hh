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

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/ensemblon.hh"
#include "biblinternes/structures/tablet.hh"

#include <mutex>

#include "lexemes.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct IdentifiantCode;
struct MetaProgramme;
struct Module;
struct NoeudBloc;
struct NoeudDeclaration;
struct NoeudDeclarationCorpsFonction;

struct Fichier {
	double temps_analyse = 0.0;
	double temps_chargement = 0.0;
	double temps_decoupage = 0.0;
	double temps_tampon = 0.0;

	lng::tampon_source tampon{""};

	dls::tableau<Lexeme> lexemes{};

	dls::ensemblon<dls::vue_chaine_compacte, 16> modules_importes{};

	Module *module = nullptr;

	MetaProgramme *metaprogramme_corps_texte = nullptr;

	size_t id = 0ul;
	dls::chaine nom{""};
	dls::chaine chemin{""};

	bool fut_lexe = false;

	Fichier() = default;

	COPIE_CONSTRUCT(Fichier);

	/**
	 * Retourne vrai si le fichier importe un module du nom spécifié.
	 */
	bool importe_module(dls::vue_chaine_compacte const &nom_module) const;
};

struct Module {
	std::mutex mutex{};
	NoeudBloc *bloc = nullptr;

	dls::tablet<Fichier *, 16> fichiers{};
	size_t id = 0ul;
	dls::chaine nom{""};
	dls::chaine chemin{""};
	bool importe = false;
};

/* ************************************************************************** */

struct PositionLexeme {
	long index_ligne = 0;
	long numero_ligne = 0;
	long pos = 0;
};

PositionLexeme position_lexeme(Lexeme const &lexeme);

void imprime_fichier_ligne(EspaceDeTravail &espace, Lexeme const &lexeme);

/* ************************************************************************** */

NoeudDeclarationCorpsFonction *cherche_fonction_dans_module(
		Compilatrice &compilatrice,
		Module *module,
		dls::vue_chaine_compacte const &nom_fonction);

NoeudDeclarationCorpsFonction *cherche_fonction_dans_module(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		dls::vue_chaine_compacte const &nom_module,
		dls::vue_chaine_compacte const &nom_fonction);

NoeudDeclaration *cherche_symbole_dans_module(
		EspaceDeTravail &espace,
		dls::vue_chaine_compacte const &nom_module,
		IdentifiantCode *ident);
