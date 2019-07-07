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

#include "biblinternes/structures/chaine.hh"

#include "morceaux.hh"

struct DonneesType;

struct DonneesMorceaux;
struct ContexteGenerationCode;

namespace erreur {

enum class type_erreur : int {
	NORMAL,
	DECOUPAGE,
	NOMBRE_ARGUMENT,
	TYPE_ARGUMENT,
	ARGUMENT_INCONNU,
	ARGUMENT_REDEFINI,
	VARIABLE_INCONNUE,
	VARIABLE_REDEFINIE,
	FONCTION_INCONNUE,
	FONCTION_REDEFINIE,
	ASSIGNATION_RIEN,
	TYPE_INCONNU,
	TYPE_DIFFERENTS,
	STRUCTURE_INCONNUE,
	STRUCTURE_REDEFINIE,
	MEMBRE_INCONNU,
	MEMBRE_REDEFINI,
	ASSIGNATION_INVALIDE,
	ASSIGNATION_MAUVAIS_TYPE,
	CONTROLE_INVALIDE,
	MODULE_INCONNU,

	AUCUNE_ERREUR,
};

class frappe {
	dls::chaine m_message;
	type_erreur m_type;

public:
	frappe(const char *message, type_erreur type);

	type_erreur type() const;

	const char *message() const;
};

[[noreturn]] void lance_erreur(const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau,
		type_erreur type = type_erreur::NORMAL);

[[noreturn]] void lance_erreur_plage(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &premier_morceau,
		const DonneesMorceaux &dernier_morceau,
		type_erreur type = type_erreur::NORMAL);

[[noreturn]] void lance_erreur_nombre_arguments(
		const size_t nombre_arguments,
		const size_t nombre_recus,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_type_arguments(
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau_enfant,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_argument_inconnu(
		const dls::vue_chaine &nom_arg,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_redeclaration_argument(
		const dls::vue_chaine &nom_arg,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_assignation_type_differents(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_type_operation(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau);

}
