/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include <cstdint>

/* espaces */
static constexpr auto ESPACE_INSECABLE = 0x00A0;
static constexpr auto ESPACE_D_OGAM = 0x1680;
static constexpr auto SEPARATEUR_VOYELLES_MONGOL = 0x180E;
static constexpr auto DEMI_CADRATIN = 0x2000;
static constexpr auto CADRATIN = 0x2001;
static constexpr auto ESPACE_DEMI_CADRATIN = 0x2002;
static constexpr auto ESPACE_CADRATIN = 0x2003;
static constexpr auto TIERS_DE_CADRATIN = 0x2004;
static constexpr auto QUART_DE_CADRATIN = 0x2005;
static constexpr auto SIXIEME_DE_CADRATIN = 0x2006;
static constexpr auto ESPACE_TABULAIRE = 0x2007;
static constexpr auto ESPACE_PONCTUATION = 0x2008;
static constexpr auto ESPACE_FINE = 0x2009;
static constexpr auto ESPACE_ULTRAFINE = 0x200A;
static constexpr auto ESPACE_SANS_CHASSE = 0x200B;
static constexpr auto ESPACE_INSECABLE_ETROITE = 0x202F;
static constexpr auto ESPACE_MOYENNE_MATHEMATIQUE = 0x205F;
static constexpr auto ESPACE_IDEOGRAPHIQUE = 0x3000;
static constexpr auto ESPACE_INSECABLE_SANS_CHASSE = 0xFEFF;

/* guillemets */
static constexpr auto GUILLEMET_OUVRANT = 0x00AB; /* « */
static constexpr auto GUILLEMET_FERMANT = 0x00BB; /* » */

namespace unicode {

int nombre_octets(const char *sequence);

int convertis_utf32(const char *sequence, int n);

int point_de_code_vers_utf8(uint32_t point_de_code, uint8_t *sequence);

}  // namespace unicode
