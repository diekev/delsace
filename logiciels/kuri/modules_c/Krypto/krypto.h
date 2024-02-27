/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BCrypt_taille_tampon();

void BCrypt_genere_empreinte(char *mot_de_passe, int charge_travail, char *sortie);

int BCrypt_compare_empreinte(char *mot_de_passe, char *empreinte);

struct HACHEUSE;

struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha1();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha256();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_md5();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_crc32();

struct HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_md5(const void *key,
                                               uint64_t numKeyBytes,
                                               const void *data,
                                               uint64_t numDataBytes);
struct HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_sha1(const void *key,
                                                uint64_t numKeyBytes,
                                                const void *data,
                                                uint64_t numDataBytes);
struct HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_sha256(const void *key,
                                                  uint64_t numKeyBytes,
                                                  const void *data,
                                                  uint64_t numDataBytes);

void KRYPTO_HACHEUSE_detruit(struct HACHEUSE *poignee);
void KRYPTO_HACHEUSE_ajourne(struct HACHEUSE *poignee, const void *data, uint64_t numDataBytes);
void KRYPTO_HACHEUSE_reinitialise(struct HACHEUSE *poignee);
void KRYPTO_HACHEUSE_condensat(struct HACHEUSE *poignee, unsigned char *sortie);
void KRYPTO_HACHEUSE_condensat_hex(struct HACHEUSE *poignee, char *sortie);
int KRYPTO_HACHEUSE_taille_condensat(struct HACHEUSE *poignee);
int KRYPTO_HACHEUSE_taille_bloc(struct HACHEUSE *poignee);

int KRYPTO_HACHEUSE_compare_condensat(const unsigned char *a,
                                      uint64_t taille_a,
                                      const unsigned char *b,
                                      uint64_t taille_b);

#ifdef __cplusplus
}
#endif
