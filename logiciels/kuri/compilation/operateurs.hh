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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

enum class id_morceau : unsigned int;
struct ContexteGenerationCode;

struct DonneesOperateur {
	long index_type1;
	long index_type2;
	long index_resultat;

	/* vrai si l'on peut sainement inverser les paramètres,
	 * vrai pour : +, -, !=, == */
	bool est_commutatif = false;

	bool inverse_parametres = false;

	/* faux pour les opérateurs définis par l'utilisateur */
	bool est_basique = true;

	dls::chaine nom_fonction = "";
};

struct Operateurs {
	using type_conteneur = dls::tableau<DonneesOperateur>;

	dls::dico_desordonne<id_morceau, type_conteneur> donnees_operateurs;

	long type_bool = 0;

	type_conteneur const &trouve(id_morceau id) const;

	void ajoute_basique(id_morceau id, long index_type, long index_type_resultat);
	void ajoute_basique(id_morceau id, long index_type1, long index_type2, long index_type_resultat);

	void ajoute_basique_unaire(id_morceau id, long index_type, long index_type_resultat);

	void ajoute_perso(id_morceau id, long index_type1, long index_type2, long index_type_resultat, dls::chaine const &nom_fonction);

	void ajoute_perso_unaire(id_morceau id, long index_type, long index_type_resultat, dls::chaine const &nom_fonction);

	void ajoute_operateur_basique_enum(long index_type);
};

const DonneesOperateur *cherche_operateur(
	Operateurs const &operateurs,
	long index_type1,
	long index_type2,
	id_morceau type_op);

DonneesOperateur const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		long index_type1,
		id_morceau type_op);

void enregistre_operateurs_basiques(
	ContexteGenerationCode &contexte,
	Operateurs &operateurs);
