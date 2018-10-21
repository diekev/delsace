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

#include <list>
#include <stack>
#include <string>
#include <vector>

#include "arbre_syntactic.h"

class Noeud;
class NoeudNombreEntier;
class NoeudNombreReel;
class NoeudOperation;

struct ContexteGenerationCode;
struct DonneesMorceaux;

class assembleuse_arbre {
	std::stack<Noeud *> m_pile{};
	std::vector<Noeud *> m_noeuds{};

	std::list<NoeudOperation *> noeuds_op_libres;
	std::list<NoeudNombreEntier *> noeuds_entier_libres;
	std::list<NoeudNombreReel *> noeuds_reel_libres;

	size_t m_memoire_utilisee = 0;

public:
	assembleuse_arbre() = default;
	~assembleuse_arbre();

	/**
	 * Ajoute un noeud au noeud courant et utilise ce noeud comme noeud courant.
	 * Tous les noeuds ajouter par après seront des enfants de celui-ci.
	 *
	 * Retourne un pointeur vers le noeud ajouter.
	 */
	Noeud *ajoute_noeud(type_noeud type, const DonneesMorceaux &morceau, bool ajoute = true);

	/**
	 * Ajoute le noeud spécifié au noeud courant.
	 */
	void ajoute_noeud(Noeud *noeud);

	/**
	 * Crée un noeud sans le désigner comme noeud courant, et retourne un
	 * pointeur vers celui-ci.
	 */
	Noeud *cree_noeud(type_noeud type, const DonneesMorceaux &morceau);

	/**
	 * Sors du noeud courant en vérifiant que le type du noeud courant est bel
	 * et bien le type passé en paramètre.
	 */
	void sors_noeud(type_noeud type);

	/**
	 * Visite les enfants du noeud racine et demande à chacun d'eux d'imprimer
	 * son 'code'. C'est attendu que les différends noeuds demandent à leurs
	 * enfants d'imprimer leurs codes.
	 *
	 * Cette fonction est là pour le débogage.
	 */
	void imprime_code(std::ostream &os);

	/**
	 * Traverse l'arbre et génère le code LLVM.
	 */
	void genere_code_llvm(ContexteGenerationCode &contexte_generation);

	void supprime_noeud(Noeud *noeud);

	/**
	 * Retourne la quantité de mémoire utilisée pour créer et stocker les noeuds
	 * de l'arbre.
	 */
	size_t memoire_utilisee() const;
};

/**
 * Imprime la taille en mémoire des noeuds et des types qu'ils peuvent contenir.
 */
void imprime_taille_memoire_noeud(std::ostream &os);
