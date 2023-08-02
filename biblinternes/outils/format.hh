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

#include <iostream>

#include "biblinternes/nombre_decimaux/traits.h"
#include "biblinternes/structures/chaine.hh"

/**
 * Petite bibliothèque de typage pour mieux formatter les messages.
 */

/* ************************************************************************** */

struct temps_seconde {
	double valeur;

	explicit temps_seconde(double v);
};

std::ostream &operator<<(std::ostream &os, temps_seconde const &t);

/* ************************************************************************** */

struct pourcentage {
	double valeur;

	explicit pourcentage(double v);
};

std::ostream &operator<<(std::ostream &os, pourcentage const &p);

/* ************************************************************************** */

struct taille_octet {
	size_t valeur;

	explicit taille_octet(size_t v);
};

std::ostream &operator<<(std::ostream &os, taille_octet const &taille);

/* ************************************************************************** */

template <typename T>
auto formatte_nombre_entier(T nombre)
{
	auto resultat_tmp = dls::vers_chaine(nombre);

	const auto taille = resultat_tmp.taille();

	if (taille <= 3) {
		return resultat_tmp;
	}

	const auto reste = taille % 3;
	auto resultat = dls::chaine{""};
	resultat.reserve(taille + taille / 3);

	for (auto i = 0l; i < reste; ++i) {
		resultat.ajoute(resultat_tmp[i]);
	}

	for (auto i = reste; i < taille; i += 3) {
		if (reste != 0 || i != reste) {
			resultat.ajoute(' ');
		}

		resultat.ajoute(resultat_tmp[i + 0]);
		resultat.ajoute(resultat_tmp[i + 1]);
		resultat.ajoute(resultat_tmp[i + 2]);
	}

	return resultat;
}

template <typename T>
auto formatte_nombre_decimal(T nombre)
{
	using type_entier = typename dls::nombre_decimaux::integral_repr<T>::type;
	auto part_entiere = formatte_nombre_entier(static_cast<type_entier>(nombre));

	auto part_decimale = dls::vers_chaine(nombre);

	auto pos_point = part_decimale.trouve('.');

	if (pos_point != -1) {
		part_entiere.append(part_decimale.sous_chaine(pos_point));
	}

	return part_entiere;
}

template <typename T>
auto formatte_nombre(T nombre)
{
	if constexpr (std::is_integral_v<T>) {
		return formatte_nombre_entier(nombre);
	}
	else if constexpr (std::is_floating_point_v<T>) {
		return formatte_nombre_decimal(nombre);
	}

	return dls::chaine("pas un nombre");
}
