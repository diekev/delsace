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

#include "biblinternes/structures/chaine.hh"

#include "morceaux.hh"

struct DonneesModule;

class decoupeuse_texte {
    DonneesModule *m_module;
    const char *m_debut_mot = nullptr;
    const char *m_debut = nullptr;
    const char *m_fin = nullptr;

    long m_position_ligne = 0;
    long m_compte_ligne = 0;
    long m_pos_mot = 0;
    long m_taille_mot_courant = 0;

  public:
    explicit decoupeuse_texte(DonneesModule *module);

    void genere_morceaux();

    /**
     * Retourne la taille en octets de la mémoire utilisée par les morceaux.
     */
    size_t memoire_morceaux() const;

    void imprime_morceaux(std::ostream &os);

  private:
    bool fini() const;

    void avance(int n = 1);

    char caractere_courant() const;

    char caractere_voisin(int n = 1) const;

    dls::vue_chaine mot_courant() const;

    [[noreturn]] void lance_erreur(const dls::chaine &quoi) const;

    void analyse_caractere_simple();

    void pousse_caractere();

    void pousse_mot(id_morceau identifiant);

    void enregistre_pos_mot();
};
