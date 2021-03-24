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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tuples.hh"
#include "biblinternes/moultfilage/synchrone.hh"

#include "arbre_syntaxique/expression.hh"

#include "structures/tableau.hh"

struct IdentifiantCode;
struct Type;

struct ItemMonomorphisation {
	IdentifiantCode *ident = nullptr;
	Type *type = nullptr;
	ValeurExpression valeur{};
	bool est_type = false;

	bool operator == (ItemMonomorphisation const &autre) const
	{
		if (ident != autre.ident) {
			return false;
		}

		if (type != autre.type) {
			return false;
		}

		if (est_type != autre.est_type) {
			return false;
		}

		if (!est_type) {
			if (valeur.type != autre.valeur.type) {
				return false;
			}

			if (valeur.entier != autre.valeur.entier) {
				return false;
			}
		}

		return true;
	}

	bool operator != (ItemMonomorphisation const &autre) const
	{
		return !(*this == autre);
	}
};

template <typename TypeNoeud>
struct Monomorphisations {
private:
	template <typename T>
	using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

	using tableau_items = kuri::tableau<ItemMonomorphisation, int>;
	tableau_synchrone<dls::paire<tableau_items, TypeNoeud *>> monomorphisations{};

public:
	TypeNoeud *trouve_monomorphisation(tableau_items const &items) const
	{
		auto monomorphisations_ = monomorphisations.verrou_lecture();

		POUR (*monomorphisations_) {
			if (it.premier.taille() != items.taille()) {
				continue;
			}

			auto trouve = true;

			for (auto i = 0; i < items.taille(); ++i) {
				if (it.premier[i] != items[i]) {
					trouve = false;
					break;
				}
			}

			if (!trouve) {
				continue;
			}

			return it.second;
		}

		return nullptr;
	}

	void ajoute(tableau_items const &items, TypeNoeud *noeud)
	{
		monomorphisations->ajoute({ items, noeud });
	}

	long memoire_utilisee() const
	{
		long memoire = 0;
		memoire += monomorphisations->taille() * (taille_de(TypeNoeud *) + taille_de(tableau_items));

		POUR (*monomorphisations.verrou_lecture()) {
			memoire += it.premier.taille() * (taille_de(ItemMonomorphisation));
		}

		return memoire;
	}

	int taille() const
	{
		return monomorphisations->taille();
	}

	int nombre_items_max() const
	{
		int n = 0;

		POUR (*monomorphisations.verrou_lecture()) {
			if (it.premier.taille() > 0) {
				n = it.premier.taille();
			}
		}

		return n;
	}
};
