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

#include "operateurs.hh"

#include "biblinternes/structures/dico_fixe.hh"

#include "contexte_generation_code.h"

// types comparaisons :
// ==, !=, <, >, <=, =>
static id_morceau operateurs_comparaisons[] = {
	id_morceau::EGALITE,
	id_morceau::DIFFERENCE,
	id_morceau::INFERIEUR,
	id_morceau::SUPERIEUR,
	id_morceau::INFERIEUR_EGAL,
	id_morceau::SUPERIEUR_EGAL
};

// types entiers et réels :
// +, -, *, / (assignés +=, -=, /=, *=)
static id_morceau operateurs_entiers_reels[] = {
	id_morceau::PLUS,
	id_morceau::MOINS,
	id_morceau::FOIS,
	id_morceau::DIVISE,
};

// types entiers :
// %, <<, >>, &, |, ^ (assignés %=, <<=, >>=, &, |, ^)
static id_morceau operateurs_entiers[] = {
	id_morceau::POURCENT,
	id_morceau::DECALAGE_GAUCHE,
	id_morceau::DECALAGE_DROITE,
	id_morceau::ESPERLUETTE,
	id_morceau::BARRE,
	id_morceau::CHAPEAU
};

static bool est_commutatif(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::EGALITE:
		case id_morceau::DIFFERENCE:
		{
			return true;
		}
	}
}

Operateurs::~Operateurs()
{
	for (auto &paire : donnees_operateurs) {
		for (auto &op : paire.second) {
			memoire::deloge("DonneesOpérateur", op);
		}
	}
}

const Operateurs::type_conteneur &Operateurs::trouve(id_morceau id) const
{
	return donnees_operateurs.trouve(id)->second;
}

void Operateurs::ajoute_basique(id_morceau id, long index_type, long index_type_resultat)
{
	ajoute_basique(id, index_type, index_type, index_type_resultat);
}

void Operateurs::ajoute_basique(id_morceau id, long index_type1, long index_type2, long index_type_resultat)
{
	auto op = memoire::loge<DonneesOperateur>("DonneesOpérateur");
	op->index_type1 = index_type1;
	op->index_type2 = index_type2;
	op->index_resultat = index_type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = true;

	donnees_operateurs[id].pousse(op);
}

void Operateurs::ajoute_basique_unaire(id_morceau id, long index_type, long index_type_resultat)
{
	ajoute_basique(id, index_type, index_type, index_type_resultat);
}

void Operateurs::ajoute_perso(
		id_morceau id,
		long index_type1,
		long index_type2,
		long index_type_resultat,
		const dls::chaine &nom_fonction)
{
	auto op = memoire::loge<DonneesOperateur>("DonneesOpérateur");
	op->index_type1 = index_type1;
	op->index_type2 = index_type2;
	op->index_resultat = index_type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = false;
	op->nom_fonction = nom_fonction;

	donnees_operateurs[id].pousse(op);
}

void Operateurs::ajoute_perso_unaire(
		id_morceau id,
		long index_type,
		long index_type_resultat,
		const dls::chaine &nom_fonction)
{
	auto op = memoire::loge<DonneesOperateur>("DonneesOpérateur");
	op->index_type1 = index_type;
	op->index_resultat = index_type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = false;
	op->nom_fonction = nom_fonction;

	donnees_operateurs[id].pousse(op);
}

void Operateurs::ajoute_operateur_basique_enum(long index_type)
{
	for (auto op : operateurs_comparaisons) {
		this->ajoute_basique(op, index_type, type_bool);
	}

	for (auto op : operateurs_entiers) {
		this->ajoute_basique(op, index_type, index_type);
	}
}

DonneesOperateur const *cherche_operateur(
		Operateurs const &operateurs,
		long index_type1,
		long index_type2,
		id_morceau type_op)
{
	auto op_commutatif = static_cast<DonneesOperateur const *>(nullptr);

	for (auto const &op : operateurs.trouve(type_op)) {
		if (op->index_type1 == index_type1 && op->index_type2 == index_type2) {
			return op;
		}

		if (op->est_commutatif && op->index_type1 == index_type2 && op->index_type2 == index_type1) {
			op_commutatif = op;
			//op_cummutatif->inverse_parametres = true;
			// ne retourne pas au cas où nous avons un opérateur avec les arguments dans le bon ordre
		}
	}

	return op_commutatif;
}

DonneesOperateur const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		long index_type1,
		id_morceau type_op)
{
	for (auto const &op : operateurs.trouve(type_op)) {
		if (op->index_type1 == index_type1) {
			return op;
		}
	}

	return nullptr;
}

void enregistre_operateurs_basiques(
		ContexteGenerationCode &contexte,
		Operateurs &operateurs)
{
	static long types_entiers[] = {
		contexte.magasin_types[TYPE_N8],
		contexte.magasin_types[TYPE_N16],
		contexte.magasin_types[TYPE_N32],
		contexte.magasin_types[TYPE_N64],
		contexte.magasin_types[TYPE_N128],
		contexte.magasin_types[TYPE_Z8],
		contexte.magasin_types[TYPE_Z16],
		contexte.magasin_types[TYPE_Z32],
		contexte.magasin_types[TYPE_Z64],
		contexte.magasin_types[TYPE_Z128],
	};

	auto type_r16 = contexte.magasin_types[TYPE_R16];
	auto type_r32 = contexte.magasin_types[TYPE_R32];
	auto type_r64 = contexte.magasin_types[TYPE_R64];

	static long types_reels[] = {
		type_r32, type_r64
	};

	auto type_pointeur = contexte.magasin_types[TYPE_PTR_NUL];
	auto type_octet = contexte.magasin_types[TYPE_OCTET];
	auto type_bool = contexte.magasin_types[TYPE_BOOL];
	operateurs.type_bool = type_bool;

	for (auto op : operateurs_entiers_reels) {
		for (auto type : types_entiers) {
			operateurs.ajoute_basique(op, type, type);
		}

		for (auto type : types_reels) {
			operateurs.ajoute_basique(op, type, type);
		}

		for (auto type : types_entiers) {
			operateurs.ajoute_basique(op, type, type_pointeur, type_pointeur);
			operateurs.ajoute_basique(op, type_pointeur, type, type_pointeur);
		}

		operateurs.ajoute_basique(op, type_octet, type_octet);
		operateurs.ajoute_basique(op, type_pointeur, type_pointeur);
	}

	for (auto op : operateurs_comparaisons) {
		for (auto type : types_entiers) {
			operateurs.ajoute_basique(op, type, type_bool);
		}

		for (auto type : types_reels) {
			operateurs.ajoute_basique(op, type, type_bool);
		}

		operateurs.ajoute_basique(op, type_octet, type_bool);
		operateurs.ajoute_basique(op, type_pointeur, type_bool);
	}

	for (auto op : operateurs_entiers) {
		for (auto type : types_entiers) {
			operateurs.ajoute_basique(op, type, type);
		}

		operateurs.ajoute_basique(op, type_octet, type_octet);
		operateurs.ajoute_basique(op, type_pointeur, type_pointeur);
	}

	// operateurs booléens && ||
	operateurs.ajoute_basique(id_morceau::ESP_ESP, type_bool, type_bool);
	operateurs.ajoute_basique(id_morceau::BARRE_BARRE, type_bool, type_bool);

	// opérateurs unaires + - ~
	for (auto type : types_entiers) {
		operateurs.ajoute_basique_unaire(id_morceau::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(id_morceau::MOINS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(id_morceau::TILDE, type, type);
	}

	for (auto type : types_reels) {
		operateurs.ajoute_basique_unaire(id_morceau::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(id_morceau::MOINS_UNAIRE, type, type);
	}

	// opérateurs unaires booléens !
	operateurs.ajoute_basique_unaire(id_morceau::EXCLAMATION, type_bool, type_bool);

	// type r16

	// r16 + r32 => DLS_ajoute_r16r32

	static dls::paire<id_morceau, dls::vue_chaine> op_r16[] = {
		dls::paire{ id_morceau::PLUS, dls::vue_chaine("DLS_ajoute_") },
		dls::paire{ id_morceau::MOINS, dls::vue_chaine("DLS_soustrait_") },
		dls::paire{ id_morceau::FOIS, dls::vue_chaine("DLS_multiplie_") },
		dls::paire{ id_morceau::DIVISE, dls::vue_chaine("DLS_divise_") },
	};

	static dls::paire<id_morceau, dls::vue_chaine> op_comp_r16[] = {
		dls::paire{ id_morceau::EGALITE, dls::vue_chaine("DLS_compare_egl_") },
		dls::paire{ id_morceau::DIFFERENCE, dls::vue_chaine("DLS_compare_non_egl_") },
		dls::paire{ id_morceau::INFERIEUR, dls::vue_chaine("DLS_compare_inf_") },
		dls::paire{ id_morceau::SUPERIEUR, dls::vue_chaine("DLS_compare_sup_") },
		dls::paire{ id_morceau::INFERIEUR_EGAL, dls::vue_chaine("DLS_compare_inf_egl_") },
		dls::paire{ id_morceau::SUPERIEUR_EGAL, dls::vue_chaine("DLS_compare_sup_egl_") },
	};

	for (auto paire_op : op_r16) {
		auto op = paire_op.premier;
		auto chaine = dls::chaine(paire_op.second);

		operateurs.ajoute_perso(op, type_r16, type_r16, type_r16, chaine + "r16r16");
		operateurs.ajoute_perso(op, type_r16, type_r32, type_r16, chaine + "r16r32");
		operateurs.ajoute_perso(op, type_r32, type_r16, type_r16, chaine + "r32r16");
		operateurs.ajoute_perso(op, type_r16, type_r64, type_r16, chaine + "r16r64");
		operateurs.ajoute_perso(op, type_r64, type_r16, type_r16, chaine + "r64r16");
	}

	for (auto paire_op : op_comp_r16) {
		auto op = paire_op.premier;
		auto chaine = paire_op.second;

		operateurs.ajoute_perso(op, type_r16, type_r16, type_bool, chaine + "r16r16");
		operateurs.ajoute_perso(op, type_r16, type_r32, type_bool, chaine + "r16r32");
		operateurs.ajoute_perso(op, type_r32, type_r16, type_bool, chaine + "r32r16");
		operateurs.ajoute_perso(op, type_r16, type_r64, type_bool, chaine + "r16r64");
		operateurs.ajoute_perso(op, type_r64, type_r16, type_bool, chaine + "r64r16");
	}

	operateurs.ajoute_perso_unaire(id_morceau::PLUS_UNAIRE, type_r16, type_r16, "DLS_plus_r16");
	operateurs.ajoute_perso_unaire(id_morceau::MOINS_UNAIRE, type_r16, type_r16, "DLS_moins_r16");

	operateurs.ajoute_perso(id_morceau::EGAL, type_r16, type_r32, type_r16, "DLS_depuis_r32");
	operateurs.ajoute_perso(id_morceau::EGAL, type_r16, type_r64, type_r16, "DLS_depuis_r64");

	operateurs.ajoute_perso(id_morceau::EGAL, type_r32, type_r16, type_r32, "DLS_vers_r32");
	operateurs.ajoute_perso(id_morceau::EGAL, type_r64, type_r16, type_r64, "DLS_vers_r64");
}
