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

[[nodiscard]] inline auto entier_depuis_bits(float x)
{
	union U {
		int i;
		float f;
	};

	U u;
	u.f = x;
	return u.i;
}

/* Source: http://burtleburtle.net/bob/c/lookup3.c */

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

#define mix(a, b, c) \
  { \
	a -= c; \
	a ^= rot(c, 4); \
	c += b; \
	b -= a; \
	b ^= rot(a, 6); \
	a += c; \
	c -= b; \
	c ^= rot(b, 8); \
	b += a; \
	a -= c; \
	a ^= rot(c, 16); \
	c += b; \
	b -= a; \
	b ^= rot(a, 19); \
	a += c; \
	c -= b; \
	c ^= rot(b, 4); \
	b += a; \
  }

#define finalise(a, b, c) \
  { \
	c ^= b; \
	c -= rot(b, 14); \
	a ^= c; \
	a -= rot(c, 11); \
	b ^= a; \
	b -= rot(a, 25); \
	c ^= b; \
	c -= rot(b, 16); \
	a ^= c; \
	a -= rot(c, 4); \
	b ^= a; \
	b -= rot(a, 14); \
	c ^= b; \
	c -= rot(b, 24); \
  }

[[nodiscard]] inline auto empreinte_n32(unsigned kx)
{
	unsigned a = 0xdeadbeef + (1 << 2) + 13;
	auto b = a;
	auto c = a;

	a += kx;

	finalise(a, b, c);

	return c;
}

[[nodiscard]] inline auto empreinte_n32(unsigned kx, unsigned ky)
{
	unsigned a = 0xdeadbeef + (1 << 2) + 13;
	auto b = a;
	auto c = a;

	a += kx;
	b += ky;

	finalise(a, b, c);

	return c;
}

[[nodiscard]] inline auto empreinte_n32(unsigned kx, unsigned ky, unsigned kz)
{
	unsigned a = 0xdeadbeef + (1 << 2) + 13;
	auto b = a;
	auto c = a;

	a += kx;
	b += ky;
	c += kz;

	finalise(a, b, c);

	return c;
}

[[nodiscard]] inline auto empreinte_n32(unsigned kx, unsigned ky, unsigned kz, unsigned kw)
{
	unsigned a = 0xdeadbeef + (1 << 2) + 13;
	auto b = a;
	auto c = a;

	a += kx;
	b += ky;
	c += kz;
	mix(a, b, c);

	a+= kw;
	finalise(a, b, c);

	return c;
}

[[nodiscard]] inline auto empreinte_n32_vers_r32(unsigned kx)
{
	return static_cast<float>(empreinte_n32(kx)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_n32_vers_r32(unsigned kx, unsigned ky)
{
	return static_cast<float>(empreinte_n32(kx, ky)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_n32_vers_r32(unsigned kx, unsigned ky, unsigned kz)
{
	return static_cast<float>(empreinte_n32(kx, ky, kz)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_n32_vers_r32(unsigned kx, unsigned ky, unsigned kz, unsigned kw)
{
	return static_cast<float>(empreinte_n32(kx, ky, kz, kw)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_r32_vers_r32(float kx)
{
	auto a = static_cast<unsigned>(entier_depuis_bits(kx));
	return static_cast<float>(empreinte_n32(a)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_r32_vers_r32(float kx, float ky)
{
	auto a = static_cast<unsigned>(entier_depuis_bits(kx));
	auto b = static_cast<unsigned>(entier_depuis_bits(ky));
	return static_cast<float>(empreinte_n32(a, b)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_r32_vers_r32(float kx, float ky, float kz)
{
	auto a = static_cast<unsigned>(entier_depuis_bits(kx));
	auto b = static_cast<unsigned>(entier_depuis_bits(ky));
	auto c = static_cast<unsigned>(entier_depuis_bits(kz));
	return static_cast<float>(empreinte_n32(a, b, c)) / static_cast<float>(0xFFFFFFFFu);
}

[[nodiscard]] inline auto empreinte_r32_vers_r32(float kx, float ky, float kz, float kw)
{
	auto a = static_cast<unsigned>(entier_depuis_bits(kx));
	auto b = static_cast<unsigned>(entier_depuis_bits(ky));
	auto c = static_cast<unsigned>(entier_depuis_bits(kz));
	auto d = static_cast<unsigned>(entier_depuis_bits(kw));
	return static_cast<float>(empreinte_n32(a, b, c, d)) / static_cast<float>(0xFFFFFFFFu);
}
