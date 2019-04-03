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

namespace dls::math {

template <typename type_vecteur, typename T, unsigned long N, int... index>
struct swizzler {
	static constexpr auto nombre_composants = sizeof...(index);

	T donnees[N];

	type_vecteur dechoie()
	{
		auto vec = type_vecteur{};
		copie_vers(vec, 0, index...);
		return vec;
	}

	operator type_vecteur() const
	{
		return dechoie();
	}

	operator type_vecteur()
	{
		return dechoie();
	}

	swizzler &operator=(const type_vecteur &vec)
	{
		copie_depuis(vec, 0, index...);
		return *this;
	}

private:
	template <typename... Index>
	void copie_vers(type_vecteur &vec, int i, Index... s_index) const
	{
		((vec[i++] = donnees[s_index]), ...);
	}

	template <typename... Index>
	void copie_depuis(const type_vecteur &vec, int i, Index... s_index) const
	{
		((donnees[s_index] = vec[i++]), ...);
	}
};

}  /* namespace dls::math */
