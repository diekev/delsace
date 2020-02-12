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

#include "compilation/contexte_generation_code.h"
#include "compilation/typeuse.hh"

#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/file.hh"
#include "biblinternes/structures/dico_fixe.hh"

static void verifie_transformation(
		Typeuse &typeuse,
		ContexteGenerationCode &contexte,
		DonneesTypeFinal const &dt1,
		DonneesTypeFinal const &dt2,
		bool est_possible)
{
	auto idx_dt1 = typeuse.ajoute_type(dt1);
	auto idx_dt2 = typeuse.ajoute_type(dt2);
	auto transformation = cherche_transformation(contexte, idx_dt1, idx_dt2);

	if (est_possible && transformation.type == TypeTransformation::IMPOSSIBLE) {
		std::cerr << "ERREUR la transformation entre ";
		std::cerr << chaine_type(dt1, contexte);
		std::cerr << " et ";
		std::cerr << chaine_type(dt2, contexte);
		std::cerr << " doit être possible\n";
	}

	if (!est_possible && transformation.type != TypeTransformation::IMPOSSIBLE) {
		std::cerr << "ERREUR la transformation entre ";
		std::cerr << chaine_type(dt1, contexte);
		std::cerr << " et ";
		std::cerr << chaine_type(dt2, contexte);
		std::cerr << " doit être impossible\n";
	}

	if (transformation.type == TypeTransformation::INUTILE) {
		//std::cerr << "La transformation est inutile\n";
		return;
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		//std::cerr << "La transformation est impossible\n";
		return;
	}

	std::cerr << "Pour transformer ";
	std::cerr << chaine_type(dt1, contexte);
	std::cerr << " en ";
	std::cerr << chaine_type(dt2, contexte);
	std::cerr << ", il faut faire : ";
	std::cerr << chaine_transformation(transformation.type);

	if (transformation.type == TypeTransformation::FONCTION) {
		std::cerr << " (" << transformation.nom_fonction << ')';
	}

	std::cerr << '\n';
}

static void verifie_transformation(
		Typeuse &typeuse,
		ContexteGenerationCode &contexte,
		TypeBase type1,
		TypeBase type2,
		bool est_possible)
{
	auto &dt1 = typeuse[typeuse[type1]];
	auto &dt2 = typeuse[typeuse[type2]];
	verifie_transformation(typeuse, contexte, dt1, dt2, est_possible);
}

int main()
{
	auto contexte = ContexteGenerationCode();

	auto ds_contexte = DonneesStructure();
	contexte.ajoute_donnees_structure("ContexteProgramme", ds_contexte);

	auto &typeuse = contexte.typeuse;
	contexte.index_type_contexte = ds_contexte.index_type;

	auto dt_tabl_fixe = DonneesTypeFinal{};
	dt_tabl_fixe.pousse(GenreLexeme::TABLEAU | static_cast<GenreLexeme>(8 << 8));
	dt_tabl_fixe.pousse(GenreLexeme::Z32);

	auto dt_tabl_dyn = DonneesTypeFinal{};
	dt_tabl_dyn.pousse(GenreLexeme::TABLEAU);
	dt_tabl_dyn.pousse(GenreLexeme::Z32);

	typeuse.ajoute_type(dt_tabl_dyn);
	typeuse.ajoute_type(dt_tabl_fixe);

	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::N8, true);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::REF_N8, true);
	verifie_transformation(typeuse, contexte, TypeBase::REF_N8, TypeBase::N8, true);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::PTR_N8, false);
	verifie_transformation(typeuse, contexte, TypeBase::PTR_N8, TypeBase::N8, false);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::Z8, false);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::REF_Z8, false);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::PTR_Z8, false);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::N64, true);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::REF_N64, false);
	verifie_transformation(typeuse, contexte, TypeBase::N8, TypeBase::CHAINE, false);
	verifie_transformation(typeuse, contexte, TypeBase::R64, TypeBase::N8, false);
	verifie_transformation(typeuse, contexte, TypeBase::R64, TypeBase::EINI, true);
	verifie_transformation(typeuse, contexte, TypeBase::EINI, TypeBase::R64, true);
	verifie_transformation(typeuse, contexte, TypeBase::EINI, TypeBase::EINI, true);
	// test []octet -> eini => CONSTRUIT_EINI et non EXTRAIT_TABL_OCTET
	verifie_transformation(typeuse, contexte, TypeBase::TABL_OCTET, TypeBase::EINI, true);
	verifie_transformation(typeuse, contexte, TypeBase::EINI, TypeBase::TABL_OCTET, true);

	verifie_transformation(typeuse, contexte, TypeBase::PTR_Z8, TypeBase::PTR_NUL, true);
	verifie_transformation(typeuse, contexte, TypeBase::PTR_Z8, TypeBase::PTR_RIEN, true);
	verifie_transformation(typeuse, contexte, TypeBase::PTR_Z8, TypeBase::PTR_OCTET, true);

	verifie_transformation(typeuse, contexte, TypeBase::PTR_NUL, TypeBase::PTR_Z8, true);
	verifie_transformation(typeuse, contexte, TypeBase::PTR_RIEN, TypeBase::PTR_Z8, true);

	verifie_transformation(typeuse, contexte, typeuse[typeuse[TypeBase::PTR_NUL]], typeuse[contexte.index_type_contexte], true);
	verifie_transformation(typeuse, contexte, typeuse[typeuse[TypeBase::PTR_RIEN]], typeuse[contexte.index_type_contexte], true);

	verifie_transformation(typeuse, contexte, typeuse[contexte.index_type_contexte], typeuse[typeuse[TypeBase::PTR_NUL]], true);
	verifie_transformation(typeuse, contexte, typeuse[contexte.index_type_contexte], typeuse[typeuse[TypeBase::PTR_RIEN]], true);

	// test [4]z32 -> []z32 et [4]z32 -> eini
	verifie_transformation(typeuse, contexte, TypeBase::TABL_N8, TypeBase::TABL_OCTET, true);

	verifie_transformation(typeuse, contexte, dt_tabl_fixe, dt_tabl_dyn, true);

	auto &dt_eini = typeuse[typeuse[TypeBase::EINI]];

	verifie_transformation(typeuse, contexte, dt_tabl_fixe, dt_eini, true);

	auto &dt_tabl_octet = typeuse[typeuse[TypeBase::TABL_OCTET]];
	verifie_transformation(typeuse, contexte, dt_tabl_fixe, dt_tabl_octet, true);

	/* test : appel fonction */
	verifie_transformation(typeuse, contexte, TypeBase::R16, TypeBase::R32, true);
	verifie_transformation(typeuse, contexte, TypeBase::R32, TypeBase::R16, true);

	// test nul -> fonc()

	return 0;
}
