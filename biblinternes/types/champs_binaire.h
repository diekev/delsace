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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cstdint>

#include "../outils/definitions.h"

A_FAIRE("finir l'implémentation");

namespace dls {
namespace types {

template <unsigned... >
struct sum;

template <unsigned T>
struct sum<T> {
	enum {
		value = T
	};
};

template <unsigned T, unsigned... Ts>
struct sum<T, Ts...> {
	enum {
		value = T + sum<Ts...>::value
	};
};

template <unsigned bits> struct Store;
template <> struct Store<8>  { typedef std::uint8_t  type; };
template <> struct Store<16> { typedef std::uint16_t type; };
template <> struct Store<32> { typedef std::uint32_t type; };
template <> struct Store<64> { typedef std::uint64_t type; };

template <unsigned... sizes>
class ChampsBinaire {
	typename Store<sum<sizes...>::value>::type store;

	template <unsigned pos, unsigned b4, unsigned size, unsigned... more_sizes>
	friend unsigned getImpl(ChampsBinaire<size, more_sizes...> &bf);

	template <unsigned pos, unsigned... more_sizes>
	friend void set(ChampsBinaire<more_sizes...> bf, unsigned value);

public:
	void setStore(unsigned value)
	{
		store = value;
	}
};

template <unsigned pos, unsigned b4, unsigned size, unsigned... more_sizes>
unsigned getImpl(ChampsBinaire<size, more_sizes...> &bf)
{
	static_assert(pos <= sizeof...(more_sizes), "Invalid bitfield access");

	if (pos == 0) {
		if (size == 1) {
			return (bf.store & (1u << b4)) != 0;
		}

		return (bf.store >> b4) & ((1u << size) - 1);
	}

	return getImpl<pos - (pos ? 1 : 0), b4 + (pos ? size : 0)>(bf);
}

// Getting field's value
template <unsigned pos, unsigned... more_sizes>
unsigned get(ChampsBinaire<more_sizes...> &bf)
{
	return getImpl<pos, 0>(bf);
}

template <unsigned pos, unsigned... more_sizes>
void set(ChampsBinaire<more_sizes...> bf, unsigned value)
{
	bf.store = (value & (1ul << pos)) >> pos;
}

// getNoShift, setNoShift
// maskAt, mask for a given bitfield
// getStore, setStore

#if 0
{
	ChampsBinaire<1, 3, 4, 8> bf;  // 16-bits
	bf.setStore(2);  // clear entire bitfield
	set<1>(bf, 6);  // sets second field to 6
	auto x = get<1>(bf);  // gets third field
}
#endif

}  /* namespace types */
}  /* namespace dls */
