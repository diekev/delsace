#pragma once

#include "bcrypt.h"
#include "biblinternes/structures/chaine.hh"
#include <stdexcept>

namespace bcrypt {
    static dls::chaine genere_empreinte(const dls::chaine &mot_de_passe, int workload = 12)
	{
		char salt[BCRYPT_HASHSIZE];
		char hash[BCRYPT_HASHSIZE];
		int ret;

		ret = bcrypt_gensalt(workload, salt);

		if(ret != 0)throw std::runtime_error{"bcrypt: can not generate salt"};

		ret = bcrypt_hashpw(mot_de_passe.c_str(), salt, hash);

		if(ret != 0){
			throw std::runtime_error{"bcrypt: can not generate hash"};
		}

		return dls::chaine{hash};
	}

	static bool compare_empreinte(const dls::chaine & password, const dls::chaine & hash)
	{
		return (bcrypt_checkpw(password.c_str(), hash.c_str()) == 0);
	}
}
