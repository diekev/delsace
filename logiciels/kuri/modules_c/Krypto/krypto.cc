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
#include "sha256.hh"

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

long SHA256_taille_tampon()
{
	return 2 * sha256::SHA256::DIGEST_SIZE;
}

void SHA256_genere_empreinte(char *entree, long taille, char *sortie)
{
	using namespace sha256;

	uint8 digest[SHA256::DIGEST_SIZE];
	memset(digest, 0, SHA256::DIGEST_SIZE);

	auto ctx = SHA256{};
	ctx.init();
	ctx.update(reinterpret_cast<const unsigned char*>(entree), static_cast<unsigned>(taille));
	ctx.final(digest);

	char buf[2 * SHA256::DIGEST_SIZE + 1];
	buf[2 * SHA256::DIGEST_SIZE] = 0;

	for (auto i = 0u; i < SHA256::DIGEST_SIZE; i++) {
		sprintf(buf+i*2, "%02x", digest[i]);
	}

	strncpy(sortie, buf, 2 * SHA256::DIGEST_SIZE);
}

}

void HMAC_genere_empreinte(unsigned char *cle, long taille_cle, unsigned char *message, long taille_message)
{
	// initialize key with zeros
	unsigned char usedKey[2 * sha256::SHA256::DIGEST_SIZE] = {0};

	// adjust length of key: must contain exactly blockSize bytes
	if (taille_cle <= 2 * sha256::SHA256::DIGEST_SIZE) {
		// copy key
		memcpy(usedKey, cle, static_cast<size_t>(taille_cle));
	}
	else {
		// shorten key: usedKey = hashed(key)
		using namespace sha256;
		auto ctx = SHA256{};
		ctx.init();
		ctx.update(cle, static_cast<unsigned>(taille_cle));
		ctx.final(usedKey);
		//SHA256_genere_empreinte(reinterpret_cast<char *>(cle), taille_cle, reinterpret_cast<char *>(usedKey));
	}

	// create initial XOR padding
	for (size_t i = 0; i < 2 * sha256::SHA256::DIGEST_SIZE; i++)
		usedKey[i] ^= 0x36;

	printf("--------------------------\n");
	printf("résultat IPAD : \n");

	for (auto i = 0u; i < 2 * sha256::SHA256::DIGEST_SIZE; ++i) {
		printf("%c", usedKey[i]);
	}

	printf("\n--------------------------\n");

	// inside = hash((usedKey ^ 0x36) + data)
	unsigned char inside[2 * sha256::SHA256::DIGEST_SIZE];
	{
		using namespace sha256;
		auto ctx = SHA256{};
		ctx.init();
		ctx.update(usedKey, 2 * sha256::SHA256::DIGEST_SIZE);
		ctx.update(message, static_cast<unsigned>(taille_message));
		ctx.final(inside);

//		char buf[2 * SHA256::DIGEST_SIZE + 1];
//		buf[2 * SHA256::DIGEST_SIZE] = 0;

//		for (auto i = 0u; i < SHA256::DIGEST_SIZE; i++) {
//			sprintf(buf+i*2, "%02x", inside[i]);
//		}

//		memcpy(inside, buf, 2 * SHA256::DIGEST_SIZE);
	}

	// undo usedKey's previous 0x36 XORing and apply a XOR by 0x5C
	for (size_t i = 0; i < 2 * sha256::SHA256::DIGEST_SIZE; i++)
		usedKey[i] ^= 0x5C ^ 0x36;

	printf("--------------------------\n");
	printf("résultat OPAD : \n");

	for (auto i = 0u; i < 2 * sha256::SHA256::DIGEST_SIZE; ++i) {
		printf("%c", usedKey[i]);
	}

	printf("\n--------------------------\n");

	// hash((usedKey ^ 0x5C) + hash((usedKey ^ 0x36) + data))
	unsigned char resultat[2 * sha256::SHA256::DIGEST_SIZE];
	{
		using namespace sha256;
		auto ctx = SHA256{};
		ctx.init();
		ctx.update(usedKey, 2 * sha256::SHA256::DIGEST_SIZE);
		ctx.update(inside, 2 * sha256::SHA256::DIGEST_SIZE);
		ctx.final(resultat);

//		char buf[2 * SHA256::DIGEST_SIZE + 1];
//		buf[2 * SHA256::DIGEST_SIZE] = 0;

//		for (auto i = 0u; i < SHA256::DIGEST_SIZE; i++) {
//			sprintf(buf+i*2, "%02x", resultat[i]);
//		}

//		memcpy(resultat, buf, 2 * SHA256::DIGEST_SIZE);
	}

	printf("--------------------------\n");
	printf("résultat HMAC C : \n");

	for (auto i = 0u; i < 2 * sha256::SHA256::DIGEST_SIZE; ++i) {
		printf("%c", resultat[i]);
	}

	printf("\n--------------------------\n");
}
