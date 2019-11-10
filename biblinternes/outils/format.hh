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
