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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <memory>

#include "version.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

class TamponRendu {
	unsigned int m_id;

public:
	TamponRendu();
	~TamponRendu();

	using Ptr = std::unique_ptr<TamponRendu>;
	using SPtr = std::shared_ptr<TamponRendu>;

	/**
	 * Crée et retourn un std::unique_ptr vers une instance de TamponRendu.
	 */
	static Ptr cree();

	/**
	 * Crée et retourn un std::shared_ptr vers une instance de TamponRendu.
	 */
	static SPtr cree_partage();

	/**
	 * Lie ce tampon au contexte courant.
	 */
	void lie() const noexcept;

	/**
	 * Délie ce tampon au contexte courant.
	 */
	void delie() const noexcept;

	/**
	 * Alloue de l'espace pour ce tampon selon la hauteur, la largeur, et le
	 * format interne spécifiés.
	 */
	void alloue(const int largeur, const int hauteur, const unsigned int format_interne) const noexcept;

	/**
	 * Alloue de l'espace pour ce tampon selon la hauteur, la largeur, le format
	 * interne, et le nombre d'échantillons spécifiés.
	 */
	void alloue(const int largeur, const int hauteur, const unsigned int format_interne, int echantillons) const noexcept;

	/**
	 * Retourne le code de liaison de ce tampon.
	 */
	unsigned int code_liaison() const noexcept;
};

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
