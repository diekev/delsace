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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <iostream>
#include <mysql/mysql.h>

class iterateur_ligne {
	MYSQL_RES *m_resultat = nullptr;
	MYSQL_ROW m_ligne = nullptr;

public:
	iterateur_ligne()
		: m_resultat(nullptr)
		, m_ligne(nullptr)
	{}

	iterateur_ligne(MYSQL_RES *res)
		: m_resultat(res)
		, m_ligne(mysql_fetch_row(m_resultat))
	{}

	void operator++()
	{
		m_ligne = mysql_fetch_row(m_resultat);
	}

	MYSQL_ROW ligne() const
	{
		return m_ligne;
	}

	MYSQL_RES *resultat() const
	{
		return m_resultat;
	}
};

class ligne_resultat {
	MYSQL_ROW m_ligne = nullptr;
	unsigned long *m_longueurs = nullptr;

	ligne_resultat(MYSQL_ROW ligne, MYSQL_RES *resultat)
		: m_ligne(ligne)
		, m_longueurs(mysql_fetch_lengths(resultat))
	{}

public:
	ligne_resultat(iterateur_ligne iter_ligne)
		: ligne_resultat(iter_ligne.ligne(), iter_ligne.resultat())
	{}

	std::string_view operator[](size_t i)
	{
		return std::string_view(m_ligne[i], m_longueurs[i]);
	}
};

struct connexion_mysql {
	MYSQL myqsl;
};

#include <vector>

struct Profil {
	std::string nom;
	std::string prenom;
	std::string age;
};

int main()
{
	MYSQL mysql;
	mysql_init(&mysql);

	mysql_options(&mysql,MYSQL_READ_DEFAULT_GROUP,"option");

	auto connexion = mysql_real_connect(&mysql, "localhost", "root", "u1z-dotl50", "test", 0, nullptr, 0);

	if (connexion == nullptr) {
		printf("Une erreur s'est produite lors de la connexion à la BDD!");
	}

	mysql_query(&mysql, "SELECT * FROM profil;");

	auto resultat = mysql_store_result(&mysql);
	auto nombre_resultat = mysql_num_rows(resultat);

	std::cout << "Il y a " << nombre_resultat << " résultats\n";

	auto nombre_champs = mysql_num_fields(resultat);

	auto iter_ligne = iterateur_ligne(resultat);

#if 0
	std::vector<Profil> m_profils;

	for (const auto &ligne : iter_ligne) {
		auto profil = Profil{};
		profil.nom = ligne[0];
		profil.prenom = ligne[1];
		profil.age = ligne[2];

		m_profils.push_back(profil);
	}
#else
	while (iter_ligne.ligne() != nullptr) {
		auto ligne = ligne_resultat(iter_ligne);

		for (auto i = 0u; i < nombre_champs; ++i) {
			std::cout << "[" << ligne[i] << "] ";
		}

		++iter_ligne;

		std::cout << '\n';
	}
#endif

	mysql_free_result(resultat);
	mysql_close(&mysql);

	return 0;
}
