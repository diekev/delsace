#pragma once

#include "bcrypt.h"
#include <string>
#include <stdexcept>

namespace bcrypt {
    static std::string genere_empreinte(const std::string &mot_de_passe, int workload = 12)
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

		return std::string{hash};
	}

	static bool compare_empreinte(const std::string & password, const std::string & hash)
	{
		return (bcrypt_checkpw(password.c_str(), hash.c_str()) == 0);
	}
}
