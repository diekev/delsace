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

/* ************************************************************************** */

/* espaces */
static constexpr auto ESPACE_INSECABLE             = 0x00A0;
static constexpr auto ESPACE_D_OGAM                = 0x1680;
static constexpr auto SEPARATEUR_VOYELLES_MONGOL   = 0x180E;
static constexpr auto DEMI_CADRATIN                = 0x2000;
static constexpr auto CADRATIN                     = 0x2001;
static constexpr auto ESPACE_DEMI_CADRATIN         = 0x2002;
static constexpr auto ESPACE_CADRATIN              = 0x2003;
static constexpr auto TIERS_DE_CADRATIN            = 0x2004;
static constexpr auto QUART_DE_CADRATIN            = 0x2005;
static constexpr auto SIXIEME_DE_CADRATIN          = 0x2006;
static constexpr auto ESPACE_TABULAIRE             = 0x2007;
static constexpr auto ESPACE_PONCTUATION           = 0x2008;
static constexpr auto ESPACE_FINE                  = 0x2009;
static constexpr auto ESPACE_ULTRAFINE             = 0x200A;
static constexpr auto ESPACE_SANS_CHASSE           = 0x200B;
static constexpr auto ESPACE_INSECABLE_ETROITE     = 0x202F;
static constexpr auto ESPACE_MOYENNE_MATHEMATIQUE  = 0x205F;
static constexpr auto ESPACE_IDEOGRAPHIQUE         = 0x3000;
static constexpr auto ESPACE_INSECABLE_SANS_CHASSE = 0xFEFF;

/* guillemets */
static constexpr auto GUILLEMET_OUVRANT = 0x00AB;  /* « */
static constexpr auto GUILLEMET_FERMANT = 0x00BB;  /* » */

/* ************************************************************************** */

namespace lng {

/**
 * Retourne le nombre d'octets Unicode (entre 1 et 4) qui composent le début la
 * séquence précisée. Retourne 0 si la sequence d'octets n'est pas valide en
 * Unicode (UTF-8).
 */
int nombre_octets(const char *sequence);

/**
 * Trouve le décalage pour le caractère de la chaine à la position i. La
 * fonction prend en compte la possibilité qu'un caractère soit invalide et le
 * saute au cas où.
 */
long decalage_pour_caractere(dls::vue_chaine const &chaine, long i);

/**
 * Retourne une chaine correspondant à la chaine spécifiée dénuée d'accents.
 */
dls::chaine supprime_accents(dls::chaine const &chaine);

int converti_utf32(const char *sequence, int n);

int point_de_code_vers_utf8(unsigned int point_de_code, unsigned char *sequence);

}  /* namespace lng */
