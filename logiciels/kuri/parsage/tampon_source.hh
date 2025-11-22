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

#include "structures/chaine.hh"
#include "structures/tableau.hh"

class TamponSource {
    kuri::chaine m_tampon{};
    kuri::tableau<kuri::chaine_statique> m_lignes{};

  public:
    TamponSource() = default;

    explicit TamponSource(const char *chaine);

    explicit TamponSource(kuri::chaine chaine) noexcept;

    TamponSource(TamponSource const &autre);

    TamponSource &operator=(TamponSource const &autre);

    TamponSource(TamponSource &&autre);
    TamponSource &operator=(TamponSource &&autre);

    /**
     * Retourne un pointeur vers le début du tampon.
     */
    const char *debut() const noexcept;

    /**
     * Retourne un pointeur vers la fin du tampon.
     */
    const char *fin() const noexcept;

    kuri::chaine_statique operator[](int64_t i) const noexcept;

    int64_t nombre_lignes() const noexcept;

    int64_t taille_donnees() const noexcept;

    TamponSource sous_tampon(size_t debut, size_t fin) const;

    kuri::chaine const &chaine() const;

  private:
    /**
     * Construit le vecteur contenant les données de chaque ligne du tampon.
     */
    void construis_lignes();
    void construis_lignes_lent();
    void construis_lignes_avx();
};
