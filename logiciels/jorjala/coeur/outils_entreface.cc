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

#include "outils_entreface.hh"

#include "biblinternes/outils/fichier.hh"

void initialise_entreface(danjo::Manipulable *manipulable, const ResultatCheminEntreface &res)
{
	std::visit([&](auto &&arg)
	{
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, CheminFichier>) {
			danjo::initialise_entreface(manipulable, dls::contenu_fichier(arg.chemin).c_str());
		}
		else {
			danjo::initialise_entreface(manipulable, arg.texte.c_str());
		}
	}, res);
}


void initialise_entreface(danjo::GestionnaireInterface *gestionnaire, danjo::Manipulable *manipulable, const ResultatCheminEntreface &res)
{
	std::visit([&](auto &&arg)
	{
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, CheminFichier>) {
			gestionnaire->initialise_entreface_texte(manipulable, dls::contenu_fichier(arg.chemin).c_str());
		}
		else {
			gestionnaire->initialise_entreface_texte(manipulable, arg.texte.c_str());
		}
	}, res);
}
