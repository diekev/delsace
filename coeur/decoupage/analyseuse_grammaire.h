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

#include "analyseuse.h"

#include "assembleuse_arbre.h"

class NoeudDeclarationFonction;

class analyseuse_grammaire : public Analyseuse {
	assembleuse_arbre *m_assembleuse = nullptr;

public:
	analyseuse_grammaire(const TamponSource &tampon, assembleuse_arbre *assembleuse);

	void lance_analyse(const std::vector<DonneesMorceaux> &identifiants) override;

private:
	void analyse_corps();
	void analyse_declaration_fonction();
	void analyse_parametres_fonction(NoeudDeclarationFonction *noeud);
	void analyse_corps_fonction();
	void analyse_expression_droite(int identifiant_final);
	void analyse_appel_fonction();
	void analyse_declaration_structure();
	void analyse_declaration_constante();
	void analyse_declaration_enum();
	void analyse_declaration_type();
};
