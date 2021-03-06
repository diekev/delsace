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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "enchaineuse.hh"

#include <string>

#include "biblinternes/memoire/logeuse_memoire.hh"

Enchaineuse::Enchaineuse() : tampon_courant(&m_tampon_base)
{
}

Enchaineuse::~Enchaineuse()
{
    auto tampon = m_tampon_base.suivant;

    while (tampon != nullptr) {
        auto tmp = tampon;
        tampon = tampon->suivant;

        memoire::deloge("Tampon", tmp);
    }
}

void Enchaineuse::ajoute(const kuri::chaine_statique &chn)
{
    ajoute(chn.pointeur(), chn.taille());
}

void Enchaineuse::ajoute_inverse(const kuri::chaine_statique &chn)
{
    for (auto i = chn.taille() - 1; i >= 0; --i) {
        pousse_caractere(chn.pointeur()[i]);
    }
}

void Enchaineuse::ajoute(const char *c_str, long N)
{
    auto tampon = tampon_courant;

    if (tampon->occupe + N < TAILLE_TAMPON) {
        memcpy(&tampon->donnees[tampon->occupe], c_str, static_cast<size_t>(N));
        tampon->occupe += static_cast<int>(N);
    }
    else {
        auto taille_a_ecrire = N;
        auto taille_max = TAILLE_TAMPON - tampon->occupe;

        if (taille_max != 0) {
            memcpy(&tampon->donnees[tampon->occupe], c_str, static_cast<size_t>(taille_max));
            tampon->occupe += taille_max;
            taille_a_ecrire -= taille_max;
        }

        auto decalage = taille_max;
        while (taille_a_ecrire > 0) {
            ajoute_tampon();
            tampon = tampon_courant;

            auto taille_ecrite = std::min(taille_a_ecrire, static_cast<long>(TAILLE_TAMPON));

            memcpy(&tampon->donnees[0], c_str + decalage, static_cast<size_t>(taille_ecrite));
            tampon->occupe += static_cast<int>(taille_ecrite);

            taille_a_ecrire -= taille_ecrite;
            decalage += static_cast<int>(taille_ecrite);
        }
    }
}

void Enchaineuse::pousse_caractere(char c)
{
    auto tampon = tampon_courant;

    if (tampon->occupe == TAILLE_TAMPON) {
        ajoute_tampon();
        tampon = tampon_courant;
    }

    tampon->donnees[tampon->occupe++] = c;
}

void Enchaineuse::imprime_dans_flux(std::ostream &flux)
{
    auto tampon = &m_tampon_base;

    while (tampon != nullptr) {
        flux.write(&tampon->donnees[0], tampon->occupe);
        tampon = tampon->suivant;
    }
}

void Enchaineuse::ajoute_tampon()
{
    auto tampon = memoire::loge<Tampon>("Tampon");
    tampon_courant->suivant = tampon;
    tampon_courant = tampon;
}

int Enchaineuse::nombre_tampons() const
{
    auto compte = 1;
    auto tampon = m_tampon_base.suivant;

    while (tampon != nullptr) {
        compte += 1;
        tampon = tampon->suivant;
    }

    return compte;
}

int Enchaineuse::nombre_tampons_alloues() const
{
    return nombre_tampons() - 1;
}

long Enchaineuse::taille_chaine() const
{
    auto taille = 0l;
    auto tampon = &m_tampon_base;

    while (tampon) {
        taille += tampon->occupe;
        tampon = tampon->suivant;
    }

    return taille;
}

kuri::chaine Enchaineuse::chaine() const
{
    auto taille = taille_chaine();

    if (taille == 0) {
        return "";
    }

    auto resultat = kuri::chaine();
    resultat.redimensionne(taille);

    auto tampon = &m_tampon_base;
    auto decalage = 0;

    while (tampon) {
        memcpy(&resultat[decalage], &tampon->donnees[0], static_cast<size_t>(tampon->occupe));
        decalage += tampon->occupe;
        tampon = tampon->suivant;
    }

    return resultat;
}

void Enchaineuse::permute(Enchaineuse &autre)
{
    if (tampon_courant != &m_tampon_base && autre.tampon_courant != &autre.m_tampon_base) {
        std::swap(tampon_courant, autre.tampon_courant);
    }

    for (auto i = 0; i < TAILLE_TAMPON; ++i) {
        std::swap(m_tampon_base.donnees[i], autre.m_tampon_base.donnees[i]);
    }

    std::swap(m_tampon_base.occupe, autre.m_tampon_base.occupe);
    std::swap(m_tampon_base.suivant, autre.m_tampon_base.suivant);
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const kuri::chaine_statique &chn)
{
    enchaineuse.ajoute(chn.pointeur(), chn.taille());
    return enchaineuse;
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const kuri::chaine &chn)
{
    enchaineuse.ajoute(chn.pointeur(), chn.taille());
    return enchaineuse;
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const char *chn)
{
    auto ptr = chn;

    while (*chn != '\0') {
        ++chn;
    }

    enchaineuse.ajoute(ptr, chn - ptr);

    return enchaineuse;
}
