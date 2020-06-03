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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

long BCrypt_taille_tampon();

void BCrypt_genere_empreinte(char *mot_de_passe, int charge_travail, char *sortie);

int BCrypt_compare_empreinte(char *mot_de_passe, char *empreinte);

long SHA256_taille_tampon();

void SHA256_genere_empreinte(char *entree, long taille, char *sortie);

void HMAC_genere_empreinte(unsigned char *cle, long taille_cle, unsigned char *message, long taille_message);

#ifdef __cplusplus
}
#endif
