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

#include <string>

#include "morceaux.h"

struct DonneesType;

struct DonneesMorceaux;
struct TamponSource;

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

	AUCUNE_ERREUR,
};

class frappe {
	std::string m_message;
	type_erreur m_type;

public:
	frappe(const char *message, type_erreur type);

	type_erreur type() const;

	const char *message() const;
};

[[noreturn]] void lance_erreur(
		const std::string &quoi,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau,
		type_erreur type = type_erreur::NORMAL);

[[noreturn]] void lance_erreur_nombre_arguments(
		const size_t nombre_arguments,
		const size_t nombre_recus,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_type_arguments(
		const id_morceau type_arg,
		const id_morceau type_enf,
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_argument_inconnu(
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_redeclaration_argument(
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_assignation_type_differents(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_type_operation(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

}
