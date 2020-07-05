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

static void converti_hash_chaine_hex(char *sortie, unsigned char *hash_cru, int taille)
{
	static const char dec2hex[16+1] = "0123456789abcdef";

	for (int i = 0; i < taille; i++) {
		*sortie++ = dec2hex[(hash_cru[i] >> 4) & 15];
		*sortie++ = dec2hex[ hash_cru[i]       & 15];
	}
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

}
