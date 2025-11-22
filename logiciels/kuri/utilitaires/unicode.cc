/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#include "unicode.hh"

namespace unicode {

inline bool entre(uint8_t c, uint8_t a, uint8_t b)
{
    return c >= a && c <= b;
}

int nombre_octets(const char *sequence)
{
    const auto s0 = static_cast<uint8_t>(sequence[0]);

    if (entre(s0, 0x00, 0x7F)) {
        return 1;
    }

    const auto s1 = static_cast<uint8_t>(sequence[1]);

    if (entre(s0, 0xC2, 0xDF)) {
        if (!entre(s1, 0x80, 0xBF)) {
            return 0;
        }

        return 2;
    }

    if (entre(s0, 0xE0, 0xEF)) {
        if (s0 == 0xE0 && !entre(s1, 0xA0, 0xBF)) {
            return 0;
        }

        if (s0 == 0xED && !entre(s1, 0x80, 0x9F)) {
            return 0;
        }

        if (!entre(s1, 0x80, 0xBF)) {
            return 0;
        }

        const auto s2 = static_cast<uint8_t>(sequence[2]);

        if (!entre(s2, 0x80, 0xBF)) {
            return 0;
        }

        return 3;
    }

    if (entre(s0, 0xF0, 0xF4)) {
        if (s0 == 0xF0 && !entre(s1, 0x90, 0xBF)) {
            return 0;
        }

        if (s0 == 0xF4 && !entre(s1, 0x80, 0x8F)) {
            return 0;
        }

        if (!entre(s1, 0x80, 0xBF)) {
            return 0;
        }

        const auto s2 = static_cast<uint8_t>(sequence[2]);

        if (!entre(s2, 0x80, 0xBF)) {
            return 0;
        }

        const auto s3 = static_cast<uint8_t>(sequence[3]);

        if (!entre(s3, 0x80, 0xBF)) {
            return 0;
        }

        return 4;
    }

    return 0;
}

int convertis_utf32(const char *sequence, int n)
{
    auto const s0 = static_cast<uint8_t>(sequence[0]);

    if (n == 1) {
        return static_cast<int>(s0) & 0b01111111;
    }

    auto const s1 = static_cast<uint8_t>(sequence[1]);

    if (n == 2) {
        auto valeur = (s0 & 0b00011111) << 6 | (s1 & 0b00111111);
        return valeur;
    }

    auto const s2 = static_cast<uint8_t>(sequence[2]);

    if (n == 3) {
        auto valeur = (s0 & 0b00001111) << 12 | (s1 & 0b00111111) << 6 | (s2 & 0b00111111);
        return valeur;
    }

    auto const s3 = static_cast<uint8_t>(sequence[3]);

    if (n == 4) {
        auto valeur = (s0 & 0b00000111) << 18 | (s1 & 0b00111111) << 12 | (s2 & 0b00111111) << 6 |
                      (s3 & 0b00111111);
        return valeur;
    }

    return 0;
}

/* Donné un point_de_code, on écris dans sequence le code UTF-8, retournant la
 * taille de la séquence en octets, ou zéro si point_de_code est invalide. Nous
 * pourrions accélérer ceci avec clz, pdep, et une table de correspondance.
 *
 * Note: nous présumons que les substituts sont traités séparement.
 */
int point_de_code_vers_utf8(uint32_t point_de_code, uint8_t *sequence)
{
    /* caractère ASCII */
    if (point_de_code <= 0x7F) {
        sequence[0] = static_cast<uint8_t>(point_de_code);
        return 1;
    }

    /* Plan universel. */
    if (point_de_code <= 0x7FF) {
        sequence[0] = static_cast<uint8_t>((point_de_code >> 6) + 192);
        sequence[1] = static_cast<uint8_t>((point_de_code & 63) + 128);
        return 2;
    }

    /* Substituts, nous pourrions avoir une assertion ici. */
    if (0xd800 <= point_de_code && point_de_code <= 0xdfff) {
        return 0;
    }

    if (point_de_code <= 0xFFFF) {
        sequence[0] = static_cast<uint8_t>((point_de_code >> 12) + 224);
        sequence[1] = static_cast<uint8_t>(((point_de_code >> 6) & 63) + 128);
        sequence[2] = static_cast<uint8_t>((point_de_code & 63) + 128);
        return 3;
    }

    /* ceci n'est pas nécessaire si nous savons que le point de code est valide */
    if (point_de_code <= 0x10FFFF) {
        sequence[0] = static_cast<uint8_t>((point_de_code >> 18) + 240);
        sequence[1] = static_cast<uint8_t>(((point_de_code >> 12) & 63) + 128);
        sequence[2] = static_cast<uint8_t>(((point_de_code >> 6) & 63) + 128);
        sequence[3] = static_cast<uint8_t>((point_de_code & 63) + 128);
        return 4;
    }

    /* Le point de code est trop large. */
    return 0;
}

}  // namespace unicode
