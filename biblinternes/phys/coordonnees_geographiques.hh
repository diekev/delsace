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

#include "biblinternes/math/outils.hh"
#include "biblinternes/math/vecteur.hh"

/* simple bibliothèque pour travailler sur des coordonnées de latitude/longitude */

template <typename T>
auto sphere(T u, T v, T r)
{
	return dls::math::vec3<T>(
				std::cos(u) + std::sin(v) * r,
				std::cos(v) * r,
				std::sin(u) * std::sin(v) * r);
}

// converti latitude / longitude en angle u / v

// longitude 0-180 -> 0 PI
// latitude 0 90 -> 0 PI/2

enum class dir_longitude {
	EST,
	OUEST,
};

enum class dir_latitude {
	NORD,
	SUD,
};

template <typename T, typename type_dir>
struct arc_geo {
	using type_valeur = T;

	T degrees{};
	T minutes{};
	T secondes{};
	type_dir dir;

	T angle() const
	{
		return degrees + minutes / static_cast<T>(60) + secondes / static_cast<T>(3600);
	}
};

using latitude  = arc_geo<double, dir_latitude>;
using longitude = arc_geo<double, dir_longitude>;

static auto operator+(longitude const &lng1, longitude const &lng2)
{
	auto degrees = lng1.degrees + lng2.degrees;
	auto minutes = lng1.minutes + lng2.minutes;
	auto secondes = lng1.secondes + lng2.secondes;
	auto dir = lng1.dir;

	if (secondes >= 60.0) {
		secondes -= 60.0;
		minutes += 1.0;
	}

	if (minutes >= 60.0) {
		minutes -= 60.0;
		degrees += 1.0;
	}

	if (degrees > 180.0) {
		degrees = 360.0 - degrees;

		if (dir == dir_longitude::EST) {
			dir = dir_longitude::OUEST;
		}
		else {
			dir = dir_longitude::EST;
		}
	}

	return longitude{degrees, minutes, secondes, dir};
}

static auto operator-(longitude const &lng1, longitude const &lng2)
{
	auto degrees = lng1.degrees + lng2.degrees;
	auto minutes = lng1.minutes + lng2.minutes;
	auto secondes = lng1.secondes + lng2.secondes;
	auto dir = lng1.dir;

	if (secondes >= 60.0) {
		secondes -= 60.0;
		minutes += 1.0;
	}

	if (minutes >= 60.0) {
		minutes -= 60.0;
		degrees += 1.0;
	}

	if (degrees > 180.0) {
		degrees = 360.0 - degrees;

		if (dir == dir_longitude::EST) {
			dir = dir_longitude::OUEST;
		}
		else {
			dir = dir_longitude::EST;
		}
	}

	return longitude{degrees, minutes, secondes, dir};
}

auto converti_vers_radians(longitude const &lng)
{
	if (lng.dir == dir_longitude::OUEST) {
		return dls::math::degrees_vers_radians(lng.angle());
	}

	return dls::math::degrees_vers_radians(static_cast<longitude::type_valeur>(360) - lng.angle());
}

auto converti_vers_radians(latitude const &lat)
{
	// 90 degrée nord = 0
	// 0 degrée = 90
	// 90 degrée sud = 180
	auto angle = lat.angle();

	if (lat.dir == dir_latitude::NORD) {
		angle = -angle;
	}

	return dls::math::degrees_vers_radians(angle + static_cast<latitude::type_valeur>(90));
}

auto greenwitch_vers_paris(longitude const &lng)
{
	// par convention, utilisation de la valeur de l'IGN, sinon 2°20'13,82"
	return lng + longitude{2.0, 20.0, 14.025, dir_longitude::EST};
}

auto paris_vers_greenwitch(longitude const &lng)
{
	// par convention, utilisation de la valeur de l'IGN, sinon 2°20'13,82"
	return lng - longitude{2.0, 20.0, 14.025, dir_longitude::EST};
}

struct coord_geo {
	latitude lat;
	longitude lng;
};

auto converti_vers_vec3(coord_geo const &geo)
{
	auto u = converti_vers_radians(geo.lng);
	auto v = converti_vers_radians(geo.lat);

	return dls::math::vec3d(std::sin(u), 0.0, std::cos(v)); //normalise(sphere(u, v, 1.0)) * 2.0;
}
