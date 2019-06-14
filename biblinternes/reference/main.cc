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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <iostream>
#include <tuple>
#include <vector>

#include "../chrono/chronometre_de_portee.hh"

template <typename... Args>
struct ArgumentsFonction {
	std::tuple<Args...> m_arguments;

	ArgumentsFonction(Args &&... args)
		: m_arguments(std::make_tuple(args...))
	{}
};

template <typename Fonction, typename... Args>
class TestReference {
	Fonction m_fonction;


	std::vector<std::tuple<Args...>> m_parametres{};

public:
	explicit TestReference(Fonction &&fonction, Args... args)
		: m_fonction(fonction)
	{
		ajoute_parametres(args...);
	}

	void ajoute_parametres(Args... args)
	{
		m_parametres.push_back(std::make_tuple(std::forward<Args...>(args)...));
	}

	void lance_tests()
	{
		for (const auto &parametre : m_parametres) {
			std::apply(m_fonction, parametre);
		}
	}
};

template <typename Fonction1, typename Fonction2>
class TestReference3 {
	Fonction1 m_fonction;

	std::vector<Fonction2> m_fonctions_genratrices{};

public:
	explicit TestReference3(Fonction1 &&fonction)
		: m_fonction(fonction)
	{}

	void ajoute_parametres(Fonction2 &&fonction)
	{
		m_fonctions_genratrices.push_back(std::forward<Fonction2>(fonction));
	}

	void lance_tests()
	{
		std::ostream &os = std::cout;
		for (const auto &fonction : m_fonctions_genratrices) {
			CHRONOMETRE_PORTEE("XXXX", os);
			auto parametres = fonction();

			std::apply(m_fonction, parametres);
		}
	}
};

template <typename Fonction, typename... Args>
auto cree_test_reference(Fonction &&fonction, Args... args)
{
	return TestReference<Fonction, Args...>(std::forward<Fonction>(fonction), args...);
}

template <typename Fonction1, typename Fonction2>
auto cree_test_reference2(Fonction1 &&fonction)
{
	return TestReference3<Fonction1, Fonction2>(std::forward<Fonction1>(fonction));
}

int main()
{
	typedef void(*type_fonction1)(int);
	typedef std::tuple<int>(*type_fonction2)();

	auto test = cree_test_reference2<type_fonction1, type_fonction2>([](const int i)
	{
		std::cout << "Test" << i << "\n";
	});

	test.ajoute_parametres([](){ return std::make_tuple(0); });
	test.ajoute_parametres([](){ return std::make_tuple(1); });
	test.ajoute_parametres([](){ return std::make_tuple(2); });
	test.ajoute_parametres([](){ return std::make_tuple(3); });

	test.lance_tests();
}
