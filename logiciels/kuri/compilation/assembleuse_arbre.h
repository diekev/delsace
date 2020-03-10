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

#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "arbre_syntactic.h"

struct ContexteGenerationCode;
struct DonneesLexeme;

class assembleuse_arbre {
	tableau_page<NoeudExpression> m_noeuds_expression{};
	tableau_page<NoeudDeclarationVariable> m_noeuds_declaration_variable{};
	tableau_page<NoeudExpressionReference> m_noeuds_expression_reference{};
	tableau_page<NoeudExpressionUnaire> m_noeuds_expression_unaire{};
	tableau_page<NoeudExpressionBinaire> m_noeuds_expression_binaire{};
	tableau_page<NoeudExpressionLogement> m_noeuds_expression_logement{};
	tableau_page<NoeudDeclarationFonction> m_noeuds_declaration_fonction{};
	tableau_page<NoeudStruct> m_noeuds_struct{};
	tableau_page<NoeudEnum> m_noeuds_enum{};
	tableau_page<NoeudSi> m_noeuds_si{};
	tableau_page<NoeudPour> m_noeuds_pour{};
	tableau_page<NoeudBoucle> m_noeuds_boucle{};
	tableau_page<NoeudBloc> m_noeuds_bloc{};
	tableau_page<NoeudDiscr> m_noeuds_discr{};
	tableau_page<NoeudPousseContexte> m_noeuds_pousse_contexte{};
	tableau_page<NoeudExpressionAppel> m_noeuds_appel{};
	tableau_page<NoeudTableauArgsVariadiques> m_noeuds_tableau_args_variadiques{};

	ContexteGenerationCode &m_contexte;

	size_t m_memoire_utilisee = 0;

	dls::pile<NoeudBloc *> m_blocs{};

public:
	dls::ensemble<dls::chaine> deja_inclus{};
	/* certains fichiers d'entête requiers d'être inclus dans un certain ordre,
	 * par exemple pour OpenGL, donc les inclusions finales sont stockées dans
	 * un tableau dans l'ordre dans lequel elles apparaissent dans le code */
	dls::tableau<dls::chaine> inclusions{};

	dls::tableau<dls::chaine> bibliotheques_dynamiques{};

	dls::tableau<dls::chaine> bibliotheques_statiques{};

	dls::tableau<dls::vue_chaine_compacte> chemins{};

	/* définitions passées au compilateur C pour modifier les fichiers d'entête */
	dls::tableau<dls::vue_chaine_compacte> definitions{};

	explicit assembleuse_arbre(ContexteGenerationCode &contexte);
	~assembleuse_arbre() = default;

	NoeudBloc *empile_bloc();

	NoeudBloc *bloc_courant() const;

	void depile_bloc();

	/**
	 * Crée un noeud sans le désigner comme noeud courant, et retourne un
	 * pointeur vers celui-ci.
	 */
	NoeudBase *cree_noeud(GenreNoeud type, DonneesLexeme const *lexeme);

	/**
	 * Retourne la quantité de mémoire utilisée pour créer et stocker les noeuds
	 * de l'arbre.
	 */
	size_t memoire_utilisee() const;

	/**
	 * Retourne le nombre de noeuds dans l'arbre.
	 */
	size_t nombre_noeuds() const;

	void ajoute_inclusion(const dls::chaine &fichier);
};
