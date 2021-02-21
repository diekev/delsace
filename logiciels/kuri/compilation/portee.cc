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

#include "portee.hh"

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique.hh"
#include "compilatrice.hh"
#include "modules.hh"

NoeudDeclaration *trouve_dans_bloc(NoeudBloc *bloc, IdentifiantCode *ident)
{
	auto bloc_courant = bloc;

	while (bloc_courant != nullptr) {
		auto membres = bloc_courant->membres.verrou_lecture();
		bloc_courant->nombre_recherches += 1;
		POUR (*membres) {
			if (it->ident == ident) {
				return it;
			}
		}

		bloc_courant = bloc_courant->bloc_parent;
	}

	return nullptr;
}

NoeudDeclaration *trouve_dans_bloc(NoeudBloc *bloc, NoeudDeclaration *decl)
{
	auto bloc_courant = bloc;

	while (bloc_courant != nullptr) {
		auto membres = bloc_courant->membres.verrou_lecture();
		bloc_courant->nombre_recherches += 1;
		POUR (*membres) {
			if (it != decl && it->ident == decl->ident) {
				return it;
			}
		}

		bloc_courant = bloc_courant->bloc_parent;
	}

	return nullptr;
}

NoeudDeclaration *trouve_dans_bloc_seul(NoeudBloc *bloc, NoeudExpression *noeud)
{
	auto membres = bloc->membres.verrou_lecture();
	bloc->nombre_recherches += 1;
	POUR (*membres) {
		if (it == noeud) {
			continue;
		}

		if (it->ident == noeud->ident) {
			return it;
		}
	}

	return nullptr;
}

NoeudDeclaration *trouve_dans_bloc_ou_module(
		EspaceDeTravail const &espace,
		NoeudBloc *bloc,
		IdentifiantCode *ident,
		Fichier *fichier)
{
	auto decl = trouve_dans_bloc(bloc, ident);

	if (decl != nullptr) {
		return decl;
	}

	/* cherche dans les modules importés */
	dls::pour_chaque_element(fichier->modules_importes, [&](auto &nom_module)
	{
		auto module = espace.module(nom_module);

		decl = trouve_dans_bloc(module->bloc, ident);

		if (decl != nullptr) {
			return dls::DecisionIteration::Arrete;
		}

		return dls::DecisionIteration::Continue;
	});

	return decl;
}

void trouve_declarations_dans_bloc(
		dls::tablet<NoeudDeclaration *, 10> &declarations,
		NoeudBloc *bloc,
		IdentifiantCode *ident)
{
	auto bloc_courant = bloc;

	while (bloc_courant != nullptr) {
		bloc_courant->nombre_recherches += 1;
		auto membres = bloc_courant->membres.verrou_lecture();
		POUR (*membres) {
			if (it->ident == ident) {
				declarations.ajoute(it);
			}
		}

		bloc_courant = bloc_courant->bloc_parent;
	}
}

void trouve_declarations_dans_bloc_ou_module(
		EspaceDeTravail const &espace,
		dls::tablet<NoeudDeclaration *, 10> &declarations,
		NoeudBloc *bloc,
		IdentifiantCode *ident,
		Fichier *fichier)
{
	trouve_declarations_dans_bloc(declarations, bloc, ident);

	/* cherche dans les modules importés */
	dls::pour_chaque_element(fichier->modules_importes, [&](auto& nom_module)
	{
		auto module = espace.module(nom_module);
		trouve_declarations_dans_bloc(declarations, module->bloc, ident);
		return dls::DecisionIteration::Continue;
	});
}

bool bloc_est_dans_boucle(NoeudBloc *bloc, IdentifiantCode *ident_boucle)
{
	while (bloc->bloc_parent) {
		if (bloc->appartiens_a_boucle) {
			auto boucle = bloc->appartiens_a_boucle;

			if (ident_boucle == nullptr) {
				return true;
			}

			if (boucle->genre == GenreNoeud::INSTRUCTION_POUR && boucle->ident == ident_boucle) {
				return true;
			}
		}

		bloc = bloc->bloc_parent;
	}

	return false;
}

NoeudExpression *derniere_instruction(NoeudBloc *b)
{
	auto expressions = b->expressions.verrou_lecture();
	auto taille = expressions->taille();

	if (taille == 0) {
		return NoeudExpression::nul();
	}

	auto di = expressions->a(taille - 1);

	if (di->est_retour() || (di->genre == GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
		return di;
	}

	if (di->genre == GenreNoeud::INSTRUCTION_SI) {
		auto inst = static_cast<NoeudSi *>(di);

		if (inst->bloc_si_faux == nullptr) {
			return NoeudExpression::nul();
		}

		if (!inst->bloc_si_faux->est_bloc()) {
			return inst->bloc_si_faux;
		}

		return derniere_instruction(inst->bloc_si_faux->comme_bloc());
	}

	if (di->genre == GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE) {
		auto inst = static_cast<NoeudPousseContexte *>(di);
		return derniere_instruction(inst->bloc);
	}

	if (di->est_discr()) {
		auto discr = di->comme_discr();

		if (discr->bloc_sinon) {
			return derniere_instruction(discr->bloc_sinon);
		}

		/* vérifie que toutes les branches retournes */
		POUR (discr->paires_discr) {
			di = derniere_instruction(it.second);

			if (di == nullptr || !di->est_retour()) {
				break;
			}
		}

		return di;
	}

	return NoeudExpression::nul();
}
