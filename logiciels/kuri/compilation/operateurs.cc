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
static TypeLexeme operateurs_comparaisons[] = {
	TypeLexeme::EGALITE,
	TypeLexeme::DIFFERENCE,
	TypeLexeme::INFERIEUR,
	TypeLexeme::SUPERIEUR,
	TypeLexeme::INFERIEUR_EGAL,
	TypeLexeme::SUPERIEUR_EGAL
};

// types entiers et réels :
// +, -, *, / (assignés +=, -=, /=, *=)
static TypeLexeme operateurs_entiers_reels[] = {
	TypeLexeme::PLUS,
	TypeLexeme::MOINS,
	TypeLexeme::FOIS,
	TypeLexeme::DIVISE,
};

// types entiers :
// %, <<, >>, &, |, ^ (assignés %=, <<=, >>=, &, |, ^)
static TypeLexeme operateurs_entiers[] = {
	TypeLexeme::POURCENT,
	TypeLexeme::DECALAGE_GAUCHE,
	TypeLexeme::DECALAGE_DROITE,
	TypeLexeme::ESPERLUETTE,
	TypeLexeme::BARRE,
	TypeLexeme::CHAPEAU,
	TypeLexeme::TILDE
};

static bool est_commutatif(TypeLexeme id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case TypeLexeme::PLUS:
		case TypeLexeme::MOINS:
		case TypeLexeme::EGALITE:
		case TypeLexeme::DIFFERENCE:
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

const Operateurs::type_conteneur &Operateurs::trouve(TypeLexeme id) const
{
	return donnees_operateurs.trouve(id)->second;
}

void Operateurs::ajoute_basique(TypeLexeme id, long index_type, long index_type_resultat)
{
	ajoute_basique(id, index_type, index_type, index_type_resultat);
}

void Operateurs::ajoute_basique(TypeLexeme id, long index_type1, long index_type2, long index_type_resultat)
{
	auto op = memoire::loge<DonneesOperateur>("DonneesOpérateur");
	op->index_type1 = index_type1;
	op->index_type2 = index_type2;
	op->index_resultat = index_type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = true;

	donnees_operateurs[id].pousse(op);
}

void Operateurs::ajoute_basique_unaire(TypeLexeme id, long index_type, long index_type_resultat)
{
	ajoute_basique(id, index_type, index_type, index_type_resultat);
}

void Operateurs::ajoute_perso(
		TypeLexeme id,
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
		TypeLexeme id,
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

static double verifie_compatibilite(
		ContexteGenerationCode const &contexte,
		long idx_type_arg,
		long idx_type_enf,
		TransformationType &transformation)
{
	transformation = cherche_transformation(contexte, idx_type_enf, idx_type_arg);

	if (transformation.type == TypeTransformation::INUTILE) {
		return 1.0;
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		return 0.0;
	}

	/* nous savons que nous devons transformer la valeur (par ex. eini), donc
	 * donne un mi-poids à l'argument */
	return 0.5;
}

dls::tableau<OperateurCandidat> cherche_candidats_operateurs(
		ContexteGenerationCode const &contexte,
		long index_type1,
		long index_type2,
		TypeLexeme type_op)
{
	auto op_candidats = dls::tableau<DonneesOperateur const *>();

	for (auto const &op : contexte.operateurs.trouve(type_op)) {
		if (op->index_type1 == index_type1 && op->index_type2 == index_type2) {
			op_candidats.efface();
			op_candidats.pousse(op);
			break;
		}

		if (op->index_type1 == index_type1 || op->index_type2 == index_type2) {
			op_candidats.pousse(op);
		}
		else if (op->est_commutatif && (op->index_type2 == index_type1 || op->index_type1 == index_type2)) {
			op_candidats.pousse(op);
		}
	}

	auto candidats = dls::tableau<OperateurCandidat>();

	for (auto const op : op_candidats) {
		auto seq1 = TransformationType{};
		auto seq2 = TransformationType{};

		auto poids1 = verifie_compatibilite(contexte, op->index_type1, index_type1, seq1);
		auto poids2 = verifie_compatibilite(contexte, op->index_type2, index_type2, seq2);

		auto poids = poids1 * poids2;

		if (poids != 0.0) {
			auto candidat = OperateurCandidat{};
			candidat.op = op;
			candidat.poids = poids;
			candidat.transformation_type1 = seq1;
			candidat.transformation_type2 = seq2;

			candidats.pousse(candidat);
		}

		if (op->est_commutatif && poids != 1.0) {
			poids1 = verifie_compatibilite(contexte, op->index_type1, index_type2, seq2);
			poids2 = verifie_compatibilite(contexte, op->index_type2, index_type1, seq1);

			poids = poids1 * poids2;

			if (poids != 0.0) {
				auto candidat = OperateurCandidat{};
				candidat.op = op;
				candidat.poids = poids;
				candidat.transformation_type1 = seq1;
				candidat.transformation_type2 = seq2;
				candidat.inverse_operandes = true;

				candidats.pousse(candidat);
			}
		}
	}

	return candidats;
}

DonneesOperateur const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		long index_type1,
		TypeLexeme type_op)
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
		contexte.typeuse[TypeBase::N8],
		contexte.typeuse[TypeBase::N16],
		contexte.typeuse[TypeBase::N32],
		contexte.typeuse[TypeBase::N64],
		contexte.typeuse[TypeBase::N128],
		contexte.typeuse[TypeBase::Z8],
		contexte.typeuse[TypeBase::Z16],
		contexte.typeuse[TypeBase::Z32],
		contexte.typeuse[TypeBase::Z64],
		contexte.typeuse[TypeBase::Z128],
	};

	auto type_r16 = contexte.typeuse[TypeBase::R16];
	auto type_r32 = contexte.typeuse[TypeBase::R32];
	auto type_r64 = contexte.typeuse[TypeBase::R64];

	static long types_reels[] = {
		type_r32, type_r64
	};

	auto type_pointeur = contexte.typeuse[TypeBase::PTR_NUL];
	auto type_octet = contexte.typeuse[TypeBase::OCTET];
	auto type_bool = contexte.typeuse[TypeBase::BOOL];
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
	operateurs.ajoute_basique(TypeLexeme::ESP_ESP, type_bool, type_bool);
	operateurs.ajoute_basique(TypeLexeme::BARRE_BARRE, type_bool, type_bool);

	// opérateurs unaires + - ~
	for (auto type : types_entiers) {
		operateurs.ajoute_basique_unaire(TypeLexeme::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(TypeLexeme::MOINS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(TypeLexeme::TILDE, type, type);
	}

	for (auto type : types_reels) {
		operateurs.ajoute_basique_unaire(TypeLexeme::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(TypeLexeme::MOINS_UNAIRE, type, type);
	}

	// opérateurs unaires booléens !
	operateurs.ajoute_basique_unaire(TypeLexeme::EXCLAMATION, type_bool, type_bool);

	// type r16

	// r16 + r32 => DLS_ajoute_r16r32

	static dls::paire<TypeLexeme, dls::vue_chaine> op_r16[] = {
		dls::paire{ TypeLexeme::PLUS, dls::vue_chaine("DLS_ajoute_") },
		dls::paire{ TypeLexeme::MOINS, dls::vue_chaine("DLS_soustrait_") },
		dls::paire{ TypeLexeme::FOIS, dls::vue_chaine("DLS_multiplie_") },
		dls::paire{ TypeLexeme::DIVISE, dls::vue_chaine("DLS_divise_") },
	};

	static dls::paire<TypeLexeme, dls::vue_chaine> op_comp_r16[] = {
		dls::paire{ TypeLexeme::EGALITE, dls::vue_chaine("DLS_compare_egl_") },
		dls::paire{ TypeLexeme::DIFFERENCE, dls::vue_chaine("DLS_compare_non_egl_") },
		dls::paire{ TypeLexeme::INFERIEUR, dls::vue_chaine("DLS_compare_inf_") },
		dls::paire{ TypeLexeme::SUPERIEUR, dls::vue_chaine("DLS_compare_sup_") },
		dls::paire{ TypeLexeme::INFERIEUR_EGAL, dls::vue_chaine("DLS_compare_inf_egl_") },
		dls::paire{ TypeLexeme::SUPERIEUR_EGAL, dls::vue_chaine("DLS_compare_sup_egl_") },
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

	operateurs.ajoute_perso_unaire(TypeLexeme::PLUS_UNAIRE, type_r16, type_r16, "DLS_plus_r16");
	operateurs.ajoute_perso_unaire(TypeLexeme::MOINS_UNAIRE, type_r16, type_r16, "DLS_moins_r16");

	operateurs.ajoute_perso(TypeLexeme::EGAL, type_r16, type_r32, type_r16, "DLS_depuis_r32");
	operateurs.ajoute_perso(TypeLexeme::EGAL, type_r16, type_r64, type_r16, "DLS_depuis_r64");

	operateurs.ajoute_perso(TypeLexeme::EGAL, type_r32, type_r16, type_r32, "DLS_vers_r32");
	operateurs.ajoute_perso(TypeLexeme::EGAL, type_r64, type_r16, type_r64, "DLS_vers_r64");

	// compairaisons de chaines
	auto type_chaine = contexte.typeuse[TypeBase::CHAINE];

	// À FAIRE: le nom de la fonction doit être le nom broyé, le coder en dur
	// n'est pas souhaitable
	operateurs.ajoute_perso(TypeLexeme::EGALITE, type_chaine, type_chaine, type_bool, "_KF4Kuri24sont_chaines_xC3xA9gales_E2_4chn18Kschaine4chn28Kschaine_S1_6Ksbool");
	operateurs.ajoute_perso(TypeLexeme::DIFFERENCE, type_chaine, type_chaine, type_bool, "_KF4Kuri26sont_chaines_inxC3xA9gales_E2_4chn18Kschaine4chn28Kschaine_S1_6Ksbool");
}
