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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "divers.h"
#include "string_utils.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#include "../chrono/chronometre_de_portee.hh"
#include "../outils/iterateurs.h"

InvalidNumberExcetion::InvalidNumberExcetion(int n)
    : m_what("Invalid number: ")
{
	m_what += std::to_string(n);
}

const char *InvalidNumberExcetion::what() const noexcept
{
	return m_what.c_str();
}

namespace french {

std::pair<std::string, std::string> dict[] = {
    { "deux dix", "vingt" },
    { "trois dix", "trente" },
    { "quatre dix", "quarante" },
    { "cinq dix", "cinquante" },
    { "six dix", "soixante" },
    { "sept dix", "soixante-dix" },
    { "huit dix", "quatre-vingt" },
    { "neuf dix", "quatre-vingt dix" },
    { "dix un", "onze" },
    { "dix deux", "douze" },
    { "dix trois", "treize" },
    { "dix quatre", "quatorze" },
    { "dix cinq", "quinze" },
    { "dix six", "seize" },
    { "dix sept", "dix-sept" },
    { "dix huit", "dix-huit" },
    { "dix neuf", "dix-neuf" },
    { "vingt un", "vingt-et-un" },
    { "trente un", "trente-et-un" },
};

auto unit_to_string(const int n)
{
	switch (n) {
		case 1: return "un";
		case 2: return "deux";
		case 3: return "trois";
		case 4: return "quatre";
		case 5: return "cinq";
		case 6: return "six";
		case 7: return "sept";
		case 8: return "huit";
		case 9: return "neuf";
	}

	throw InvalidNumberExcetion(n);
}

auto big_unit_to_string(const int n)
{
	switch (n) {
		case 0: return "";
		case 3: return "mille";
		case 6: return "million";
		case 9: return "milliard";
	}

	switch (n % 3) {
		case 1: return "dix";
		case 2: return "cent";
	}

	throw InvalidNumberExcetion(n);
}

}  /* namespace french */

namespace german {

std::pair<std::string, std::string> dict[] = {
    { "zwei zehn", "zwanzig" },
    { "drei zehn", "dreizig" },
    { "vier zehn", "vierzig" },
    { "funf zehn", "funfzig" },
    { "sechs zehn", "sechszig" },
    { "sieben zehn", "siebzig" },
    { "acht zehn", "achtzig" },
    { "neun zehn", "neunzig" },
    { "zehn ein", "elf" },
    { "zehn zwei", "zwelf" },
    { "zehn drei", "dreizehn" },
    { "zehn vier", "vierzehn" },
    { "zehn funf", "funfzehn" },
    { "zehn sechs", "sechszehn" },
    { "zehn sieben", "sibzehn" },
    { "zehn acht", "achtzehn" },
    { "zehn neun", "neunzehn" },
};

auto unit_to_string(const int n)
{
	switch (n) {
		case 1: return "ein";
		case 2: return "zwei";
		case 3: return "drei";
		case 4: return "vier";
		case 5: return "funf";
		case 6: return "sechs";
		case 7: return "sieben";
		case 8: return "acht";
		case 9: return "neun";
	}

	throw InvalidNumberExcetion(n);
}

auto big_unit_to_string(const int n)
{
	switch (n) {
		case 0: return "";
		case 3: return "tausend";
		case 6: return "million";
		case 9: return "milliarde";
	}

	switch (n % 3) {
		case 1: return "zehn";
		case 2: return "hundert";
	}

	throw InvalidNumberExcetion(n);
}

}  /* namespace german */

namespace japanese {

std::pair<std::string, std::string> dict[] = {
    { "san hyaku", "sanbyaku" },
    { "roku hyaku", "roppyaku" },
    { "hachi hyaku", "happyaku" },
    { "san sen", "sanzen" },
    { "hachi sen", "hassen" },
};

auto unit_to_string(const int n)
{
	switch (n) {
		case  1: return "ichi";
		case  2: return "ni";
		case  3: return "san";
		case  4: return "yon";
		case  5: return "go";
		case  6: return "roku";
		case  7: return "nana";
		case  8: return "hachi";
		case  9: return "kyu";
	}

	throw InvalidNumberExcetion(n);
}

auto big_unit_to_string(const int n)
{
	switch (n) {
		case 8: return "oku";    // 1 0000 0000
	}

	switch (n % 5) {
		case 0: return "";
		case 1: return "jyu";
		case 2: return "hyaku";  //         100
		case 3: return "sen";    //        1000
		case 4: return "man";    //      1 0000
	}

	throw InvalidNumberExcetion(n);
}

}  /* namespace japanese */

namespace english {

std::pair<std::string, std::string> dict[] = {
    { "two ten", "twenty" },
    { "three ten", "thirty" },
    { "four ten", "forty" },
    { "five ten", "fifty" },
    { "six ten", "sixty" },
    { "seven ten", "seventy" },
    { "eight ten", "eighty" },
    { "nine ten", "ninety" },
    { "ten one", "eleven" },
    { "ten two", "twelve" },
    { "ten three", "thirteen" },
    { "ten four", "fourteen" },
    { "ten five", "fifteen" },
    { "ten six", "sixteen" },
    { "ten seven", "seventeen" },
    { "ten eight", "eightteen" },
    { "ten nine", "nineteen" },
};

auto unit_to_string(const int n)
{
	switch (n) {
		case  1: return "one";
		case  2: return "two";
		case  3: return "three";
		case  4: return "four";
		case  5: return "five";
		case  6: return "six";
		case  7: return "seven";
		case  8: return "eight";
		case  9: return "nine";
	}

	throw InvalidNumberExcetion(n);
}

auto big_unit_to_string(const int n)
{
	switch (n) {
		case 0: return "";
		case 3: return "thousand";
		case 6: return "million";
		case 9: return "billion";
	}

	switch (n % 3) {
		case 1: return "ten";
		case 2: return "hundred";
	}

	throw InvalidNumberExcetion(n);
}

}  /* namespace english */

namespace language = japanese;

auto to_words(int number) -> std::string
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);

	auto num_str = std::to_string(number);
	std::vector<std::string> vec;

	auto size = 0ul;
	auto j = 0;
	for (const auto &num : dls::outils::inverse_iterateur(num_str)) {
		if (num == '0') {
			++j;
			continue;
		}

		auto tmp = language::big_unit_to_string(j++);
		auto tmp_size = std::strlen(tmp);

		if (tmp_size != 0) {
			vec.emplace_back(tmp);
			size += tmp_size;
		}

		if ((j > 1 && num == '1')) {
			continue;
		}

		tmp = language::unit_to_string(num - '0');
		vec.emplace_back(tmp);
		size += std::strlen(tmp);
	}

	std::string str;
	str.reserve(size);
	for (const auto &v : dls::outils::inverse_iterateur(vec)) {
		str += v;
		str += ' ';
	}

	for (const auto &it : language::dict) {
		replace_substr(str, it.first, it.second);
	}

	return str;
}
