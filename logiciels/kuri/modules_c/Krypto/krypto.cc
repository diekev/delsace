/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "krypto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcrypt.h"
#include "crc32.h"
#include "hmac.h"
#include "keccak.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "sha3.h"

static void converti_hash_chaine_hex(char *sortie, unsigned char *hash_cru, int taille)
{
    static const char dec2hex[16 + 1] = "0123456789abcdef";

    for (int i = 0; i < taille; i++) {
        *sortie++ = dec2hex[(hash_cru[i] >> 4) & 15];
        *sortie++ = dec2hex[hash_cru[i] & 15];
    }
}

class BaseHacheuse {
  public:
    virtual ~BaseHacheuse() = default;
    virtual void ajourne(const void *donnees, size_t taille_donnees) = 0;
    virtual void condensat(unsigned char *sortie) = 0;
    virtual void condensat_hex(char *sortie) = 0;
    virtual int taille_bloc() const = 0;
    virtual int taille_condensat() const = 0;
    virtual const char *nom() = 0;
    virtual void réinitialise() = 0;
};

template <typename TypeHacheuse>
class HacheuseHMAC : public BaseHacheuse {
    unsigned char usedKey[TypeHacheuse::BlockSize] = {0};
    TypeHacheuse hacheuse_interne;

  public:
    HacheuseHMAC(const void *key, size_t numKeyBytes)
    {
        // adjust length of key: must contain exactly blockSize bytes
        if (numKeyBytes <= TypeHacheuse::BlockSize) {
            // copy key
            memcpy(usedKey, key, numKeyBytes);
        }
        else {
            // shorten key: usedKey = hashed(key)
            SHA256 keyHasher;
            keyHasher.add(key, numKeyBytes);
            keyHasher.getHash(usedKey);
        }

        // create initial XOR padding
        for (size_t i = 0; i < TypeHacheuse::BlockSize; i++) {
            usedKey[i] ^= 0x36;
        }

        hacheuse_interne.add(usedKey, TypeHacheuse::BlockSize);
    }

    void ajourne(const void *donnees, size_t taille_donnees) override
    {
        hacheuse_interne.add(donnees, taille_donnees);
    }

    void condensat(unsigned char *sortie) override
    {
        unsigned char inside[TypeHacheuse::HashBytes];
        hacheuse_interne.getHash(inside);

        // undo usedKey's previous 0x36 XORing and apply a XOR by 0x5C
        for (size_t i = 0; i < TypeHacheuse::BlockSize; i++)
            usedKey[i] ^= 0x5C ^ 0x36;

        // hash((usedKey ^ 0x5C) + hash((usedKey ^ 0x36) + data))
        TypeHacheuse finalHasher;
        finalHasher.add(usedKey, TypeHacheuse::BlockSize);
        finalHasher.add(inside, TypeHacheuse::HashBytes);

        finalHasher.getHash(sortie);
    }

    void condensat_hex(char *sortie) override
    {
        unsigned char rawHash[TypeHacheuse::HashBytes];
        condensat(rawHash);
        converti_hash_chaine_hex(sortie, rawHash, TypeHacheuse::HashBytes);
    }

    int taille_bloc() const override
    {
        return TypeHacheuse::BlockSize;
    }

    int taille_condensat() const override
    {
        return TypeHacheuse::HashBytes;
    }

    void réinitialise() override
    {
        /* À FAIRE : réinitialise les autres données. */
        hacheuse_interne.reset();
    }

    const char *nom() override
    {
        return "hmac";
    }
};

using HacheuseHMACMD5 = HacheuseHMAC<MD5>;
using HacheuseHMACSHA1 = HacheuseHMAC<SHA1>;
using HacheuseHMACSHA256 = HacheuseHMAC<SHA256>;

template <typename TypeHacheuse>
class Hacheuse : public BaseHacheuse {
    TypeHacheuse hacheuse_interne{};
    InfoTypeHacheuse *m_infos = nullptr;

  public:
    Hacheuse(InfoTypeHacheuse &infos) : m_infos(&infos)
    {
    }

    void ajourne(const void *donnees, size_t taille_donnees) override
    {
        hacheuse_interne.add(donnees, taille_donnees);
    }

    void condensat(unsigned char *sortie) override
    {
        hacheuse_interne.getHash(sortie);
    }

    void condensat_hex(char *sortie) override
    {
        unsigned char rawHash[TypeHacheuse::HashBytes];
        condensat(rawHash);
        converti_hash_chaine_hex(sortie, rawHash, TypeHacheuse::HashBytes);
    }

    int taille_bloc() const override
    {
        return m_infos->taille_bloc;
    }

    int taille_condensat() const override
    {
        return m_infos->taille_condensant;
    }

    void réinitialise() override
    {
        hacheuse_interne.reset();
    }

    const char *nom() override
    {
        return m_infos->nom;
    }
};

/* Définis les infos-types des hacheuses. */
#define DEFINIS_INFO_TYPE_HACHEUSE(nom_enum, nom_hacheuse, nom_classe, ident)                     \
    static InfoTypeHacheuse info_type_##nom_enum = {                                              \
        nom_hacheuse, HACHEUSE_##nom_enum, nom_classe::BlockSize, nom_classe::HashBytes};

ENUMERE_TYPE_HACHEUSE(DEFINIS_INFO_TYPE_HACHEUSE)

#undef DEFINIS_INFO_TYPE_HACHEUSE

extern "C" {

int64_t BCrypt_taille_tampon()
{
    return BCRYPT_HASHSIZE;
}

void BCrypt_genere_empreinte(char *mot_de_passe, int charge_travail, char *sortie)
{
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];
    int ret;

    ret = bcrypt_gensalt(charge_travail, salt);

    if (ret != 0) {
        return;
    }

    ret = bcrypt_hashpw(mot_de_passe, salt, hash);

    if (ret != 0) {
        return;
    }

    strncpy(sortie, hash, BCRYPT_HASHSIZE);
}

int BCrypt_compare_empreinte(char *mot_de_passe, char *empreinte)
{
    return bcrypt_checkpw(mot_de_passe, empreinte);
}

InfoTypeHacheuse *KRYPTO_donne_info_type_hacheuse_pour(TypeHacheuse type)
{
#define DEFINIS_CAS(nom_enum, nom_hacheuse, nom_classe, ident)                                    \
    case TypeHacheuse::HACHEUSE_##nom_enum:                                                       \
    {                                                                                             \
        return &info_type_##nom_enum;                                                             \
    }

    switch (type) {
        ENUMERE_TYPE_HACHEUSE(DEFINIS_CAS)
    }

    return nullptr;
#undef DEFINIS_CAS
}

HACHEUSE *KRYPTO_cree_hacheuse_pour_type(TypeHacheuse type)
{
    auto infos = KRYPTO_donne_info_type_hacheuse_pour(type);
    if (!infos) {
        return nullptr;
    }

#define DEFINIS_CAS(nom_enum, nom_hacheuse, nom_classe, ident)                                    \
    case TypeHacheuse::HACHEUSE_##nom_enum:                                                       \
    {                                                                                             \
        return reinterpret_cast<HACHEUSE *>(new Hacheuse<nom_classe>(*infos));                    \
    }

    switch (type) {
        ENUMERE_TYPE_HACHEUSE(DEFINIS_CAS)
    }

    return nullptr;
#undef DEFINIS_CAS
}

#define POIGNEE(x) reinterpret_cast<HACHEUSE *>(x)

#define DEFINIS_FONCTION_CREATION(nom_enum, nom_hacheuse, nom_classe, ident)                      \
    HACHEUSE *KRYPTO_HACHEUSE_cree_##ident()                                                      \
    {                                                                                             \
        return POIGNEE(KRYPTO_cree_hacheuse_pour_type(TypeHacheuse::HACHEUSE_##nom_enum));        \
    }                                                                                             \
    HACHEUSE *KRYPTO_HACHEUSE_HMAC_cree_##ident(                                                  \
        const void *key, uint64_t numKeyBytes, const void *data, uint64_t numDataBytes)           \
    {                                                                                             \
        auto poignée = POIGNEE(new HacheuseHMAC<nom_classe>(key, numKeyBytes));                   \
        KRYPTO_HACHEUSE_ajourne(poignée, data, numDataBytes);                                     \
        return poignée;                                                                           \
    }

ENUMERE_TYPE_HACHEUSE(DEFINIS_FONCTION_CREATION)

#undef DEFINIS_FONCTION_CREATION

HACHEUSE *KRYPTO_cree_hacheuse_hmac_pour_type(enum TypeHacheuse type,
                                              const void *key,
                                              uint64_t numKeyBytes,
                                              const void *data,
                                              uint64_t numDataBytes)
{
#define DEFINIS_CAS(nom_enum, nom_hacheuse, nom_classe, ident)                                    \
    case TypeHacheuse::HACHEUSE_##nom_enum:                                                       \
    {                                                                                             \
        return KRYPTO_HACHEUSE_HMAC_cree_##ident(key, numKeyBytes, data, numDataBytes);           \
    }

    switch (type) {
        ENUMERE_TYPE_HACHEUSE(DEFINIS_CAS)
    }

    return nullptr;
#undef DEFINIS_CAS
}

void KRYPTO_HACHEUSE_detruit(HACHEUSE *poignee)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignee);
    delete hacheuse;
}

void KRYPTO_HACHEUSE_ajourne(HACHEUSE *poignee, const void *donnees, size_t taille_donnees)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignee);
    hacheuse->ajourne(donnees, taille_donnees);
}

void KRYPTO_HACHEUSE_reinitialise(HACHEUSE *poignée)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignée);
    hacheuse->réinitialise();
}

void KRYPTO_HACHEUSE_condensat(HACHEUSE *poignee, unsigned char *sortie)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignee);
    hacheuse->condensat(sortie);
}

void KRYPTO_HACHEUSE_condensat_hex(HACHEUSE *poignee, char *sortie)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignee);
    hacheuse->condensat_hex(sortie);
}

int KRYPTO_HACHEUSE_taille_condensat(HACHEUSE *poignee)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignee);
    return hacheuse->taille_condensat();
}

int KRYPTO_HACHEUSE_taille_bloc(HACHEUSE *poignee)
{
    auto hacheuse = reinterpret_cast<BaseHacheuse *>(poignee);
    return hacheuse->taille_bloc();
}

int KRYPTO_HACHEUSE_compare_condensat(const unsigned char *a,
                                      uint64_t taille_a,
                                      const unsigned char *b,
                                      uint64_t taille_b)
{
    /* The volatile type declarations make sure that the compiler has no
     * chance to optimize and fold the code in any way that may change
     * the timing.
     */
    volatile const unsigned char *droite;
    volatile const unsigned char *gauche;
    volatile uint64_t longueur;
    volatile unsigned char résultat;

    /* loop count depends on length of b */
    longueur = taille_b;
    gauche = nullptr;
    droite = b;

    /* don't use else here to keep the amount of CPU instructions constant,
     * volatice forces reevaluation */
    if (taille_a == longueur) {
        gauche = *((volatile const unsigned char **)&a);
        résultat = 0;
    }

    if (taille_a != longueur) {
        gauche = b;
        résultat = 1;
    }

    for (uint64_t i = 0; i < longueur; ++i) {
        résultat = résultat | (*gauche++ ^ *droite++);
    }

    return (résultat == 0 ? 0 : 1);
}
}
