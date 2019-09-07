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

#include "operatrices_rendu.hh"

#include "coeur/donnees_aval.hh"
#include "coeur/noeud.hh"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OpRenduChercheObjets : public OperatriceImage {
public:
	static constexpr auto NOM = "Cherche Objets";
	static constexpr auto AIDE = "";

	OpRenduChercheObjets(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
	{
		INUTILISE(contexte);
		INUTILISE(donnees_aval);
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpMoteurRendu : public OperatriceImage {
public:
	static constexpr auto NOM = "Moteur Rendu";
	static constexpr auto AIDE = "";

	OpMoteurRendu(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(0);
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
	{
		INUTILISE(contexte);
		INUTILISE(donnees_aval);
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_rendu(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpRenduChercheObjets>());
	usine.enregistre_type(cree_desc<OpMoteurRendu>());
}

#pragma clang diagnostic pop
