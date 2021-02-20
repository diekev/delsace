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

struct HACHEUSE;

HACHEUSE *KRYPTO_HACHEUSE_cree_sha1();
HACHEUSE *KRYPTO_HACHEUSE_cree_sha256();
HACHEUSE *KRYPTO_HACHEUSE_cree_md5();
HACHEUSE *KRYPTO_HACHEUSE_cree_crc32();

HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_md5(const void* key, unsigned long numKeyBytes, const void *data, unsigned long numDataBytes);
HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_sha1(const void* key, unsigned long numKeyBytes, const void *data, unsigned long numDataBytes);
HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_sha256(const void* key, unsigned long numKeyBytes, const void *data, unsigned long numDataBytes);

void KRYPTO_HACHEUSE_detruit(HACHEUSE *poignee);
void KRYPTO_HACHEUSE_ajourne(HACHEUSE *poignee, const void *data, unsigned long numDataBytes);
void KRYPTO_HACHEUSE_condensat(HACHEUSE *poignee, unsigned char *sortie);
void KRYPTO_HACHEUSE_condensat_hex(HACHEUSE *poignee, char *sortie);
int KRYPTO_HACHEUSE_taille_condensat(HACHEUSE *poignee);
int KRYPTO_HACHEUSE_taille_bloc(HACHEUSE *poignee);

int KRYPTO_HACHEUSE_compare_condensat(const unsigned char *a, unsigned long taille_a, const unsigned char *b, unsigned long taille_b);

#ifdef __cplusplus
}
#endif
