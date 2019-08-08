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

#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

class BaseDeDonnees;
class Noeud;
class Objet;
struct ContexteEvaluation;

/* ************************************************************************** */

/* un réseau est fait de noeuds et chaque noeuds possède une ou plusieurs
 * entrées, une ou plusieurs sorties
 *
 * Design Presto :
 * des masques sur les connexions permet une granularité des dépendances
 *
 * le réseau est static et ne contient pas d'état ; il représente seulement le
 * calcul à venir -> donc il peut être utilisé par différents threads sans
 * verrous
 */

/* ************************************************************************** */

struct NoeudReseau {
	dls::ensemble<NoeudReseau *> entrees{};
	dls::ensemble<NoeudReseau *> sorties{};

	Objet *objet{};
	Noeud *noeud_objet{};

	int degree = 0;
	int pad = 0;

	NoeudReseau() = default;
	~NoeudReseau() = default;

	NoeudReseau(NoeudReseau const &) = default;
	NoeudReseau &operator=(NoeudReseau const &) = default;
};

/* ************************************************************************** */

struct Reseau {
	dls::tableau<NoeudReseau *> noeuds{};

	/* noeud racine représentant le temps dans le réseau */
	NoeudReseau noeud_temps{};

	~Reseau();

	void reinitialise();
};

/* ************************************************************************** */

struct CompilatriceReseau {
private:
	dls::dico_desordonne<Objet *, NoeudReseau *> m_table_objet_noeud{};

	NoeudReseau *trouve_noeud_pour_objet(Objet *objet);

public:
	Reseau *reseau = nullptr;

	CompilatriceReseau() = default;

	CompilatriceReseau(CompilatriceReseau const &) = default;
	CompilatriceReseau &operator=(CompilatriceReseau const &) = default;

	void cree_noeud(Objet *objet, Noeud *noeud_objet);

	void ajoute_dependance(NoeudReseau *noeud, Objet *objet);

	void ajoute_dependance(NoeudReseau *noeud_de, NoeudReseau *noeud_vers);

	void compile_reseau(ContexteEvaluation &contexte, BaseDeDonnees *bdd, Objet *objet);

	void marque_execution_temps_change();
};
