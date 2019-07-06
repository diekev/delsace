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

#pragma once

#include <string_view>

#include "biblinternes/structures/tableau.hh"

#include "morceaux.hh"

namespace lng {
class tampon_source;
}

class decoupeuse_texte {
	lng::tampon_source const &m_tampon;
	const char *m_debut_mot = nullptr;
	const char *m_debut = nullptr;
	const char *m_fin = nullptr;

	size_t m_position_ligne = 0;
	size_t m_compte_ligne = 0;
	size_t m_pos_mot = 0;
	size_t m_taille_mot_courant = 0;

	dls::tableau<DonneesMorceaux> m_morceaux{};

public:
	using iterateur = dls::tableau<DonneesMorceaux>::iteratrice;

	explicit decoupeuse_texte(lng::tampon_source const &tampon);

	/* désactivation de la copie */
	decoupeuse_texte(const decoupeuse_texte &) = delete;
	decoupeuse_texte &operator=(const decoupeuse_texte &) = delete;

	void genere_morceaux();

	/**
	 * Retourne la taille en octets de la mémoire utilisée par les morceaux.
	 */
	size_t memoire_morceaux() const;

	dls::tableau<DonneesMorceaux> &morceaux();

	iterateur begin();

	iterateur end();

private:
	bool fini() const;

	void avance(int n = 1);

	char caractere_courant() const;

	char caractere_voisin(int n = 1) const;

	std::string_view mot_courant() const;

	[[noreturn]] void lance_erreur(const std::string &quoi) const;

	void analyse_caractere_simple();

	void pousse_caractere();

	void pousse_mot(int identifiant);

	void enregistre_position_mot();
};
