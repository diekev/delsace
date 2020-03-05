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

#include "arbre_syntactic.h"

struct ContexteGenerationCode;
struct DonneesLexeme;

class assembleuse_arbre {
	dls::tableau<NoeudBase *> m_noeuds_base{};
	dls::tableau<NoeudExpression *> m_noeuds_expression{};
	dls::tableau<NoeudDeclarationVariable *> m_noeuds_declaration_variable{};
	dls::tableau<NoeudExpressionReference *> m_noeuds_expression_reference{};
	dls::tableau<NoeudExpressionUnaire *> m_noeuds_expression_unaire{};
	dls::tableau<NoeudExpressionBinaire *> m_noeuds_expression_binaire{};
	dls::tableau<NoeudExpressionLogement *> m_noeuds_expression_logement{};
	dls::tableau<NoeudDeclarationFonction *> m_noeuds_declaration_fonction{};
	dls::tableau<NoeudStruct *> m_noeuds_struct{};
	dls::tableau<NoeudEnum *> m_noeuds_enum{};
	dls::tableau<NoeudSi *> m_noeuds_si{};
	dls::tableau<NoeudPour *> m_noeuds_pour{};
	dls::tableau<NoeudBoucle *> m_noeuds_boucle{};
	dls::tableau<NoeudBloc *> m_noeuds_bloc{};
	dls::tableau<NoeudDiscr *> m_noeuds_discr{};
	dls::tableau<NoeudPousseContexte *> m_noeuds_pousse_contexte{};
	dls::tableau<NoeudExpressionAppel *> m_noeuds_appel{};
	dls::tableau<NoeudTableauArgsVariadiques *> m_noeuds_tableau_args_variadiques{};

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
	~assembleuse_arbre();

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

/**
 * Imprime la taille en mémoire des noeuds et des types qu'ils peuvent contenir.
 */
void imprime_taille_memoire_noeud(std::ostream &os);
