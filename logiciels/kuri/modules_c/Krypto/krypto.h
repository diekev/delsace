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

typedef enum TypeHacheuse {
    HACHEUSE_CRC32,
    HACHEUSE_KECCAK_224,
    HACHEUSE_KECCAK_256,
    HACHEUSE_KECCAK_384,
    HACHEUSE_KECCAK_512,
    HACHEUSE_MD5,
    HACHEUSE_SHA1,
    HACHEUSE_SHA256,
    HACHEUSE_SHA3_224,
    HACHEUSE_SHA3_256,
    HACHEUSE_SHA3_384,
    HACHEUSE_SHA3_512,
} TypeHacheuse;

struct InfoTypeHacheuse {
    const char *nom;
    TypeHacheuse type;
    int taille_bloc;
    int taille_condensant;
};

struct HACHEUSE;

struct InfoTypeHacheuse *KRYPTO_donne_info_type_hacheuse_pour(enum TypeHacheuse type);

struct HACHEUSE *KRYPTO_cree_hacheuse_pour_type(enum TypeHacheuse type);

struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha1();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha256();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha3_224();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha3_256();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha3_384();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_sha3_512();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_keccak_224();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_keccak_256();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_keccak_384();
struct HACHEUSE *KRYPTO_HACHEUSE_cree_keccak_512();
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
