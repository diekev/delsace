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

#include "analyse.hh"

#include "compilation/arbre_syntaxique.hh"
#include "compilation/erreur.h"

#include "impression.hh"
#include "instructions.hh"

static Instruction *derniere_instruction(AtomeFonction *atome)
{
	auto &instructions = atome->instructions;

	if (instructions.taille == 0) {
		return nullptr;
	}

	auto index = instructions.taille - 1;

	while (instructions[index]->est_restaure_locales()) {
		index -= 1;
	}

	return instructions[index];
}

/* Performes différentes analyses de la RI. Ces analyses nous servent à valider
 * un peu plus la structures du programme. Nous pourrions les faire dans la
 * validation sémantique, mais ce serait un peu plus complexe, la RI nous
 * simplifie la vie.
 *
 * - détecte le manque de retour :
 *        ne peut pas car la génération de la RI peut nous mettre des branches
 *        et des labels après les retours (par exemple dans la génération de code
 *        pour les discriminations)
 */
void analyse_ri(EspaceDeTravail &espace, AtomeFonction *atome)
{
	auto decl = atome->decl;
	auto type = decl->type->comme_fonction();

	if (type->types_sorties.taille > 1 || !type->types_sorties[0]->est_rien()) {
		auto di = derniere_instruction(atome);

		if (!di || !di->est_retour()) {
			if (di) {
				std::cerr << "La dernière instruction est ";
				imprime_instruction(di, std::cerr);
				imprime_fonction(atome, std::cerr);
			}
			else {
				std::cerr << "La dernière instruction est nulle !\n";
			}
			rapporte_erreur(&espace, const_cast<NoeudDeclarationFonction *>(decl), "Instruction de retour manquante");
		}
	}
}
