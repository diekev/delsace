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

namespace dls {

template <typename T>
struct plage_continue {
	using type_valeur = T;
	using type_reference = T&;
	using type_pointeur = T*;
	using type_reference_const = T const&;
	using type_pointeur_const = T const*;
	using type_difference = long;

private:
	type_pointeur m_debut{};
	type_pointeur m_fin{};

public:
	plage_continue(type_pointeur d, type_pointeur f)
		: m_debut(d)
		, m_fin(f)
	{}

	bool est_finie() const
	{
		return m_debut >= m_fin;
	}

	void effronte()
	{
		++m_debut;
	}

	void ecule()
	{
		--m_fin;
	}

	type_difference taille() const
	{
		return m_fin - m_debut;
	}

	type_reference front()
	{
		return *m_debut;
	}

	type_reference_const front() const
	{
		return *m_debut;
	}

	type_reference cul()
	{
		return *(m_fin - 1);
	}

	type_reference_const cul() const
	{
		return *(m_fin - 1);
	}

	plage_continue premiere_moitie() const
	{
		return plage_continue(m_debut, m_debut + (m_fin - m_debut) / 2);
	}

	plage_continue deuxieme_moitie() const
	{
		return plage_continue(m_debut + (m_fin - m_debut) / 2, m_fin);
	}
};

}  /* namespace dls */
