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

#include "contexte_generation_code.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "modules.hh"

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto &module : modules) {
		memoire::deloge("DonnessModule", module);
	}
}

/* ************************************************************************** */

DonneesModule *ContexteGenerationCode::cree_module(const dls::chaine &nom)
{
	auto module = memoire::loge<DonneesModule>("DonnessModule");
	module->id = static_cast<size_t>(modules.taille());
	module->nom = nom;

	modules.ajoute(module);

	return module;
}

DonneesModule *ContexteGenerationCode::module(size_t index) const
{
	return modules[static_cast<long>(index)];
}

DonneesModule *ContexteGenerationCode::module(const dls::vue_chaine &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return module;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::module_existe(const dls::vue_chaine &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

void ContexteGenerationCode::pousse_locale(const dls::vue_chaine &nom, int valeur, lcc::type_var donnees_type)
{
	for (auto &loc : m_locales) {
		if (loc.first == nom) {
			loc.second.type = valeur;
			loc.second.donnees_type = donnees_type;
			return;
		}
	}

	m_locales.ajoute({nom, {valeur, donnees_type}});
}

int ContexteGenerationCode::valeur_locale(const dls::vue_chaine &nom)
{
	for (auto &loc : m_locales) {
		if (loc.first == nom) {
			return loc.second.type;
		}
	}

	return 0;
}

lcc::type_var ContexteGenerationCode::donnees_type(const dls::vue_chaine &nom)
{
	for (auto &loc : m_locales) {
		if (loc.first == nom) {
			return loc.second.donnees_type;
		}
	}

	return {};
}

bool ContexteGenerationCode::locale_existe(const dls::vue_chaine &nom)
{
#if 0
	auto iter_fin = m_locales.debut() + static_cast<long>(m_nombre_locales);

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine, donnees_variables> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return false;
	}

	return true;
#else
	for (auto &loc : m_locales) {
		if (loc.first == nom) {
			return true;
		}
	}

	return false;
#endif
}
