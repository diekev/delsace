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

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "version.h"

/* TODO:
 * - Make regular type:
 *   - Copyable
 *   - Assignable
 *   - Movable
 *   - Destructible
 *   - Equality-comparable
 */

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

enum class Nuanceur : char {
	VERTEX = 0,
	FRAGMENT = 1,
	GEOMETRIE = 2,
	CONTROLE_TESSELATION = 3,
	EVALUATION_TESSELATION = 4,
	CALCUL = 5,
};

enum {
	NOMBRE_NUANCEURS = static_cast<int>(Nuanceur::CALCUL) + 6,
};

class Programme {
	unsigned int m_programme = 0;
	unsigned int m_nuanceurs[NOMBRE_NUANCEURS] = { 0, 0, 0, 0, 0, 0 };
	std::unordered_map<std::string, int> m_attributs{};
	std::unordered_map<std::string, int> m_uniformes{};

public:
	Programme() = default;
	~Programme();

	using Ptr = std::unique_ptr<Programme>;
	using SPtr = std::shared_ptr<Programme>;

	/**
	 * @brief Create and return a unique pointer to this object.
	 */
	static Ptr cree_unique();

	/**
	 * @brief Create and return a shared pointer to this object.
	 */
	static SPtr cree_partage();

	/**
	 * @brief take the source code and compiles it
	 *
	 * @param shader_type the type of the shader to load
	 * @param source the source code of the shader, must be the actual code, not
	 *               the file path!
	 * @param os the stream used to redirect the error log.
	 */
	void charge(Nuanceur type_nuanceur, std::string const &source, std::ostream &os = std::cerr);

	/**
	 * @brief create a program and attach the various shaders to it.
	 * @param os the stream used to redirect the error log.
	 */
	void cree_et_lie_programme(std::ostream &os = std::cerr);

	void active() const;
	void desactive() const;

	void ajoute_attribut(std::string const &attribut);
	void ajoute_uniforme(std::string const &uniforme);

	/**
	 * @brief Check whether or not the program can be used for display.
	 * @param os the stream used to redirect the error log.
	 */
	bool est_valide(std::ostream &os = std::cerr) const;

	/**
	 * @brief Return the location of the given attribute value.
	 */
	int operator[](std::string const &attribute);

	/**
	 * @brief Return the location of the given uniform value.
	 */
	int operator()(std::string const &uniforme);

	void uniforme(std::string const &uniforme, double const value);
	void uniforme(std::string const &uniforme, double const v0, double const v1);
	void uniforme(std::string const &uniforme, double const v0, double const v1, double const v2);
	void uniforme(std::string const &uniforme, double const v0, double const v1, double const v2, double const v4);
	void uniforme(std::string const &uniforme, float const value);
	void uniforme(std::string const &uniforme, float const v0, float const v1);
	void uniforme(std::string const &uniforme, float const v0, float const v1, float const v2);
	void uniforme(std::string const &uniforme, float const v0, float const v1, float const v2, float const v4);
	void uniforme(std::string const &uniforme, int const value);
	void uniforme(std::string const &uniforme, int const v0, int const v1);
	void uniforme(std::string const &uniforme, int const v0, int const v1, int const v2);
	void uniforme(std::string const &uniforme, int const v0, int const v1, int const v2, int const v4);
	void uniforme(std::string const &uniforme, unsigned int const value);
	void uniforme(std::string const &uniforme, unsigned int const v0, unsigned int const v1);
	void uniforme(std::string const &uniforme, unsigned int const v0, unsigned int const v1, unsigned int const v2);
	void uniforme(std::string const &uniforme, unsigned int const v0, unsigned int const v1, unsigned int const v2, unsigned int const v4);
	void uniforme(std::string const &uniforme, double const *value, int const n);
	void uniforme(std::string const &uniforme, float const *value, int const n);
	void uniforme(std::string const &uniforme, int const *value, int const n);
	void uniforme(std::string const &uniforme, unsigned int const *value, int const n);
};

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
