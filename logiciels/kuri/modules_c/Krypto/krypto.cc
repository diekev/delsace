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

#include "krypto.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcrypt.h"
#include "keccak.h"
#include "hmac.h"
#include "crc32.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "sha3.h"

static void converti_hash_chaine_hex(char *sortie, unsigned char *hash_cru, int taille)
{
	static const char dec2hex[16+1] = "0123456789abcdef";

	for (int i = 0; i < taille; i++) {
		*sortie++ = dec2hex[(hash_cru[i] >> 4) & 15];
		*sortie++ = dec2hex[ hash_cru[i]       & 15];
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
};

template <typename TypeHacheuse>
class HacheuseHMAC : public BaseHacheuse {
	unsigned char usedKey[TypeHacheuse::BlockSize] = {0};
	TypeHacheuse hacheuse_interne;

public:
	HacheuseHMAC(const void* key, size_t numKeyBytes, const void *data, size_t numDataBytes)
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

		if (data) {
			ajourne(data, numDataBytes);
		}
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
		finalHasher.add(inside,  TypeHacheuse::HashBytes);

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
	TypeHacheuse hacheuse_interne;

public:
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
		return TypeHacheuse::BlockSize;
	}

	int taille_condensat() const override
	{
		return TypeHacheuse::HashBytes;
	}

	const char *nom() override
	{
		return "hmac";
	}
};

using HacheuseCRC32 = Hacheuse<CRC32>;
using HacheuseMD5 = Hacheuse<MD5>;
using HacheuseSHA1 = Hacheuse<SHA1>;
using HacheuseSHA256 = Hacheuse<SHA256>;

extern "C" {

long BCrypt_taille_tampon()
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

	if(ret != 0){
		return;
	}

	strncpy(sortie, hash, BCRYPT_HASHSIZE);
}

int BCrypt_compare_empreinte(char *mot_de_passe, char *empreinte)
{
	return bcrypt_checkpw(mot_de_passe, empreinte);
}

long CRC32_taille_tampon()
{
	return 2 * CRC32::HashBytes;
}

void CRC32_genere_empreinte(char *entree, long taille, char *sortie)
{
	auto sha256 = CRC32();
	sha256.add(entree, static_cast<unsigned long>(taille));

	unsigned char rawHash[CRC32::HashBytes];
	sha256.getHash(rawHash);

	converti_hash_chaine_hex(sortie, rawHash, CRC32::HashBytes);
}

long MD5_taille_tampon()
{
	return 2 * MD5::HashBytes;
}

void MD5_genere_empreinte(char *entree, long taille, char *sortie)
{
	auto sha256 = MD5();
	sha256.add(entree, static_cast<unsigned long>(taille));

	unsigned char rawHash[MD5::HashBytes];
	sha256.getHash(rawHash);

	converti_hash_chaine_hex(sortie, rawHash, MD5::HashBytes);
}

long SHA1_taille_tampon()
{
	return 2 * SHA1::HashBytes;
}

void SHA1_genere_empreinte(char *entree, long taille, char *sortie)
{
	auto sha1 = SHA1();
	sha1.add(entree, static_cast<unsigned long>(taille));

	unsigned char rawHash[SHA1::HashBytes];
	sha1.getHash(rawHash);

	converti_hash_chaine_hex(sortie, rawHash, SHA1::HashBytes);
}

long SHA256_taille_tampon()
{
	return 2 * SHA256::HashBytes;
}

void SHA256_genere_empreinte(char *entree, long taille, char *sortie)
{
	auto sha256 = SHA256();
	sha256.add(entree, static_cast<unsigned long>(taille));

	unsigned char rawHash[SHA256::HashBytes];
	sha256.getHash(rawHash);

	converti_hash_chaine_hex(sortie, rawHash, SHA256::HashBytes);
}

long SHA384_taille_tampon()
{
	return SHA3::Bits384 / 4;
}

void SHA384_genere_empreinte(char *entree, long taille, char *sortie)
{
	auto sha3 = SHA3(SHA3::Bits384);
	sha3.add(entree, static_cast<unsigned long>(taille));
	sha3.getHash(sortie);
}

long SHA512_taille_tampon()
{
	return SHA3::Bits512 / 4;
}

void SHA512_genere_empreinte(char *entree, long taille, char *sortie)
{
	auto sha3 = SHA3(SHA3::Bits512);
	sha3.add(entree, static_cast<unsigned long>(taille));
	sha3.getHash(sortie);
}

long HMAC_taille_tampon()
{
	return 2 * SHA256::HashBytes;
}

void HMAC_genere_empreinte(unsigned char *cle, long taille_cle, unsigned char *message, long taille_message, char *sortie)
{
	unsigned char rawHash[SHA256::HashBytes];
	hmac<SHA256>(cle, static_cast<unsigned long>(taille_cle), message, static_cast<unsigned long>(taille_message), rawHash);
	converti_hash_chaine_hex(sortie, rawHash, SHA256::HashBytes);
}

#define POIGNEE(x) reinterpret_cast<HACHEUSE *>(x)

HACHEUSE *KRYPTO_HACHEUSE_cree_sha1()
{
	return POIGNEE(new HacheuseSHA1());
}

HACHEUSE *KRYPTO_HACHEUSE_cree_sha256()
{
	return POIGNEE(new HacheuseSHA256());
}

HACHEUSE *KRYPTO_HACHEUSE_cree_md5()
{
	return POIGNEE(new HacheuseMD5());
}

HACHEUSE *KRYPTO_HACHEUSE_cree_crc32()
{
	return POIGNEE(new HacheuseCRC32());
}

HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_md5(const void* key, unsigned long numKeyBytes, const void *data, unsigned long numDataBytes)
{
	return POIGNEE(new HacheuseHMACMD5(key, numKeyBytes, data, numDataBytes));
}

HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_sha1(const void* key, unsigned long numKeyBytes, const void *data, unsigned long numDataBytes)
{
	return POIGNEE(new HacheuseHMACSHA1(key, numKeyBytes, data, numDataBytes));
}

HACHEUSE *KRYPTO_HACHEUSE_cree_hmac_sha256(const void* key, unsigned long numKeyBytes, const void *data, unsigned long numDataBytes)
{
	return POIGNEE(new HacheuseHMACSHA256(key, numKeyBytes, data, numDataBytes));
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

int KRYPTO_HACHEUSE_compare_condensat(const unsigned char *a, unsigned long taille_a, const unsigned char *b, unsigned long taille_b)
{
	/* The volatile type declarations make sure that the compiler has no
	 * chance to optimize and fold the code in any way that may change
	 * the timing.
	 */
	volatile const unsigned char *droite;
	volatile const unsigned char *gauche;
	volatile unsigned long longueur;
	volatile unsigned char resultat;

	/* loop count depends on length of b */
	longueur = taille_b;
	gauche = nullptr;
	droite = b;

	/* don't use else here to keep the amount of CPU instructions constant,
	 * volatice forces reevaluation */
	if (taille_a == longueur) {
		gauche = *((volatile const unsigned char **)&a);
		resultat = 0;
	}

	if (taille_a != longueur) {
		gauche = b;
		resultat = 1;
	}

	for (unsigned long i = 0; i < longueur; ++i) {
		resultat |= *gauche++ ^ *droite++;
	}

	return (resultat == 0 ? 0 : 1);
}

}
