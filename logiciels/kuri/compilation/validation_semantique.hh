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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "graphe_dependance.hh"

struct Compilatrice;
struct NoeudDeclarationFonction;
struct NoeudExpression;

namespace noeud {

struct ContexteValidationCode {
	Compilatrice &m_compilatrice;
	NoeudDeclarationFonction *fonction_courante = nullptr;

	/* Les données des dépendances d'un noeud syntaxique. */
	DonneesDependance donnees_dependance{};

	using paire_union_membre = std::pair<dls::vue_chaine_compacte, dls::vue_chaine_compacte>;
	dls::tableau<paire_union_membre> membres_actifs{};

	ContexteValidationCode(Compilatrice &compilatrice);

	COPIE_CONSTRUCT(ContexteValidationCode);

	void commence_fonction(NoeudDeclarationFonction *fonction);

	void termine_fonction();

	/* gestion des membres actifs des unions :
	 * cas à considérer :
	 * -- les portées des variables
	 * -- les unions dans les structures (accès par '.')
	 */
	dls::vue_chaine_compacte trouve_membre_actif(dls::vue_chaine_compacte const &nom_union);

	void renseigne_membre_actif(dls::vue_chaine_compacte const &nom_union, dls::vue_chaine_compacte const &nom_membre);

	void valide_semantique_noeud(NoeudExpression *);

	void valide_type_fonction(NoeudDeclarationFonction *);
	void valide_fonction(NoeudDeclarationFonction *);
	void valide_operateur(NoeudDeclarationFonction *);
	void valide_enum(NoeudEnum *);
	void valide_structure(NoeudStruct *);
};

void performe_validation_semantique(Compilatrice &compilatrice);

}  /* namespace noeud */
