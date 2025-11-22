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

#include "tampon_source.hh"

#include <immintrin.h>

#if !(defined(_MSC_VER) || defined(__SCE__)) || defined(__AVX__)
#    define SUPPORTE_AVX
#endif

/* ************************************************************************** */

/**
 * Retourne la position de la fin de la prochaine ligne (caractère '\\n') dans
 * la chaîne délimitée par 'début' et 'fin'.
 */
static int64_t trouve_fin_ligne(const char *début, const char *fin)
{
    int64_t pos = 0;

    while (début != fin) {
        ++pos;

        if (*début == '\n') {
            break;
        }

        ++début;
    }

    return pos;
}

/* ************************************************************************** */

TamponSource::TamponSource(const char *chaine) : m_tampon(chaine)
{
    construis_lignes();
}

TamponSource::TamponSource(kuri::chaine chaine) noexcept : m_tampon(std::move(chaine))
{
    construis_lignes();
}

TamponSource::TamponSource(const TamponSource &autre)
{
    m_tampon = autre.m_tampon;
    construis_lignes();
}

TamponSource &TamponSource::operator=(const TamponSource &autre)
{
    m_tampon = autre.m_tampon;
    construis_lignes();

    return *this;
}

TamponSource::TamponSource(TamponSource &&autre)
{
    m_tampon.permute(autre.m_tampon);
    m_lignes.permute(autre.m_lignes);
}

TamponSource &TamponSource::operator=(TamponSource &&autre)
{
    m_tampon.permute(autre.m_tampon);
    m_lignes.permute(autre.m_lignes);
    return *this;
}

const char *TamponSource::début() const noexcept
{
    return m_tampon.pointeur();
}

const char *TamponSource::fin() const noexcept
{
    return début() + m_tampon.taille();
}

kuri::chaine_statique TamponSource::operator[](int64_t i) const noexcept
{
    return m_lignes[i];
}

int64_t TamponSource::nombre_lignes() const noexcept
{
    return m_lignes.taille();
}

int64_t TamponSource::taille_données() const noexcept
{
    return m_tampon.taille() * taille_de(char) +
           nombre_lignes() * taille_de(kuri::chaine_statique);
}

TamponSource TamponSource::sous_tampon(size_t début, size_t fin) const
{
    auto pos = m_lignes[static_cast<int64_t>(début)].begin();
    auto taille = int64_t(0);

    for (auto i = début; i < fin; ++i) {
        taille += m_lignes[static_cast<int64_t>(i)].taille();
    }

    return TamponSource(kuri::chaine(pos, taille));
}

const kuri::chaine &TamponSource::chaine() const
{
    return m_tampon;
}

void TamponSource::construis_lignes()
{
    if (m_tampon.taille() == 0) {
        return;
    }

#ifdef SUPPORTE_AVX
    construis_lignes_avx();
#else
    construis_lignes_lent();
#endif
}

void TamponSource::construis_lignes_lent()
{
    /* Compte le nombre de lignes. */

    auto taille_modulo_8 = m_tampon.taille() % 8;
    auto taille_sure = m_tampon.taille() - taille_modulo_8;

    auto nombre_de_lignes = 0;
    for (auto i = int64_t(0); i < taille_sure; i += 8) {
        nombre_de_lignes += m_tampon[i] == '\n';
        nombre_de_lignes += m_tampon[i + 1] == '\n';
        nombre_de_lignes += m_tampon[i + 2] == '\n';
        nombre_de_lignes += m_tampon[i + 3] == '\n';
        nombre_de_lignes += m_tampon[i + 4] == '\n';
        nombre_de_lignes += m_tampon[i + 5] == '\n';
        nombre_de_lignes += m_tampon[i + 6] == '\n';
        nombre_de_lignes += m_tampon[i + 7] == '\n';
    }

    for (auto i = taille_sure; i < m_tampon.taille(); ++i) {
        nombre_de_lignes += m_tampon[i] == '\n';
    }

    nombre_de_lignes += (m_tampon[m_tampon.taille() - 1] != '\n');

    /* Construit le tampon. */

    m_lignes.réserve(nombre_de_lignes);

    for (auto i = int64_t(0); i < m_tampon.taille();) {
        auto pos = &m_tampon[i];
        auto taille = trouve_fin_ligne(pos, this->fin());

        m_lignes.ajoute(kuri::chaine_statique{pos, taille});

        i += taille;
    }
}

void TamponSource::construis_lignes_avx()
{
#ifdef SUPPORTE_AVX
    /* Compte le nombre de lignes. */

    auto taille_modulo_32 = m_tampon.taille() % 32;
    auto taille_sure = m_tampon.taille() - taille_modulo_32;
    const __m256i __cmp_mask = _mm256_set1_epi8('\n');

    auto nombre_de_lignes = 0;
    for (auto i = int64_t(0); i < taille_sure; i += 32) {
        const auto __32 = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&m_tampon[i]));
        const auto __cmp_result = _mm256_cmpeq_epi8(__32, __cmp_mask);
        nombre_de_lignes += __builtin_popcount(uint32_t(_mm256_movemask_epi8(__cmp_result)));
    }

    for (auto i = taille_sure; i < m_tampon.taille(); ++i) {
        nombre_de_lignes += m_tampon[i] == '\n';
    }

    nombre_de_lignes += (m_tampon[m_tampon.taille() - 1] != '\n');

    /* Construit le tampon. */

    m_lignes.réserve(nombre_de_lignes);
    auto pos_début_ligne = &m_tampon[0];
    auto pos_courante = pos_début_ligne;

    for (auto i = int64_t(0); i < taille_sure; i += 32) {
        const auto __32 = _mm256_loadu_si256(reinterpret_cast<__m256i *>(pos_courante));
        const auto __cmp_result = _mm256_cmpeq_epi8(__32, __cmp_mask);
        auto n = __builtin_popcount(uint32_t(_mm256_movemask_epi8(__cmp_result)));
        if (n == 0) {
            pos_courante += 32;
            continue;
        }

        auto j = 32;
        while (n != 0) {
            if (*pos_courante++ == '\n') {
                m_lignes.ajoute(
                    kuri::chaine_statique{pos_début_ligne, pos_courante - pos_début_ligne});
                pos_début_ligne = pos_courante;
                n -= 1;
            }
            j -= 1;
        }
        pos_courante += j;
    }

    while (pos_début_ligne < this->fin()) {
        auto taille = trouve_fin_ligne(pos_début_ligne, this->fin());
        m_lignes.ajoute(kuri::chaine_statique{pos_début_ligne, taille});
        pos_début_ligne += taille;
    }
#endif
}
