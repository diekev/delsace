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

#include "modules.hh"

#include "compilatrice.hh"
#include "portee.hh"

/* ************************************************************************** */

bool Fichier::importe_module(dls::vue_chaine_compacte const &nom_module) const
{
	return modules_importes.possede(nom_module);
}

/* ************************************************************************** */

PositionLexeme position_lexeme(Lexeme const &lexeme)
{
	auto pos = PositionLexeme{};
	pos.pos = lexeme.colonne;
	pos.numero_ligne = lexeme.ligne + 1;
	pos.index_ligne = lexeme.ligne;
	return pos;
}

NoeudDeclarationCorpsFonction *cherche_fonction_dans_module(
		Compilatrice &compilatrice,
		Module *module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto ident = compilatrice.table_identifiants->identifiant_pour_chaine(nom_fonction);
	auto decl = trouve_dans_bloc(module->bloc, ident);

	return static_cast<NoeudDeclarationCorpsFonction *>(decl);
}

NoeudDeclarationCorpsFonction *cherche_fonction_dans_module(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		dls::vue_chaine_compacte const &nom_module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto module = espace.module(nom_module);
	return cherche_fonction_dans_module(compilatrice, module, nom_fonction);
}

NoeudDeclaration *cherche_symbole_dans_module(
		EspaceDeTravail &espace,
		dls::vue_chaine_compacte const &nom_module,
		IdentifiantCode *ident)
{
	auto module = espace.module(nom_module);
	return trouve_dans_bloc(module->bloc, ident);
}

void imprime_fichier_ligne(EspaceDeTravail &espace, const Lexeme &lexeme)
{
	auto fichier = espace.fichier(lexeme.fichier);
	std::cerr << fichier->chemin << ':' << lexeme.ligne + 1 << '\n';
}
