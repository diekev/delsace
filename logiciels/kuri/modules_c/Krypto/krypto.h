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

#define ENUMERE_TYPE_HACHEUSE(O)                                                                  \
    O(CRC32, "crc32", CRC32, crc32)                                                               \
    O(KECCAK_224, "keccak-224", Keccak_224bits, keccak_224)                                       \
    O(KECCAK_256, "keccak-256", Keccak_256bits, keccak_256)                                       \
    O(KECCAK_384, "keccak-384", Keccak_384bits, keccak_384)                                       \
    O(KECCAK_512, "keccak-512", Keccak_512bits, keccak_512)                                       \
    O(MD5, "md5", MD5, md5)                                                                       \
    O(SHA1, "sha1", SHA1, sha1)                                                                   \
    O(SHA256, "sha256", SHA256, sha256)                                                           \
    O(SHA3_224, "sha3-224", SHA3_224bits, sha3_224)                                               \
    O(SHA3_256, "sha3-256", SHA3_256bits, sha3_256)                                               \
    O(SHA3_384, "sha3-384", SHA3_384bits, sha3_384)                                               \
    O(SHA3_512, "sha3-512", SHA3_512bits, sha3_512)

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
struct HACHEUSE *KRYPTO_cree_hacheuse_hmac_pour_type(enum TypeHacheuse type,
                                                     const void *key,
                                                     uint64_t numKeyBytes,
                                                     const void *data,
                                                     uint64_t numDataBytes);

#define DECLARE_FONCTION_CREATION(nom_enum, nom_hacheuse, nom_classe, ident)                      \
    struct HACHEUSE *KRYPTO_HACHEUSE_cree_##ident();                                              \
    struct HACHEUSE *KRYPTO_HACHEUSE_HMAC_cree_##ident(                                           \
        const void *key, uint64_t numKeyBytes, const void *data, uint64_t numDataBytes);

ENUMERE_TYPE_HACHEUSE(DECLARE_FONCTION_CREATION)

#undef DEFINIS_FONCTION_CREATION

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
