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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "chaine.hh"

namespace dls {

chaine::chaine(char c)
	: chaine(1, c)
{}

chaine::chaine(const char *__c_str)
	: m_chaine(__c_str)
{}

chaine::chaine(const char *__c_str, int64_t taille)
	: m_chaine(__c_str, static_cast<size_t>(taille))
{}

chaine::chaine(const vue_chaine &vue)
	: m_chaine(&vue[0], static_cast<size_t>(vue.taille()))
{}

chaine::chaine(const vue_chaine_compacte &vue)
	: m_chaine(&vue[0], static_cast<size_t>(vue.taille()))
{}

chaine::chaine(int64_t nombre, char c)
	: m_chaine(static_cast<size_t>(nombre), c)
{}

void chaine::efface()
{
	m_chaine.clear();
}

void chaine::efface(chaine::iteratrice iter)
{
	m_chaine.erase(iter);
}

void chaine::efface(chaine::iteratrice iter1, chaine::iteratrice iter2)
{
	m_chaine.erase(iter1, iter2);
}

void chaine::efface(int64_t pos, int64_t n)
{
	m_chaine.erase(static_cast<size_t>(pos), static_cast<size_t>(n));
}

void chaine::reserve(int64_t combien)
{
	m_chaine.reserve(static_cast<size_t>(combien));
}

void chaine::redimensionne(int64_t combien)
{
	m_chaine.resize(static_cast<size_t>(combien));
}

void chaine::redimensionne(int64_t combien, char c)
{
	m_chaine.resize(static_cast<size_t>(combien), c);
}

chaine &chaine::append(const chaine &c)
{
	m_chaine.append(c.m_chaine);
	return *this;
}

bool chaine::est_vide() const
{
	return m_chaine.empty();
}

int64_t chaine::capacite() const
{
	return static_cast<int64_t>(m_chaine.capacity());
}

chaine chaine::sous_chaine(int64_t pos, int64_t combien) const
{
	return m_chaine.substr(static_cast<size_t>(pos), static_cast<size_t>(combien));
}

int64_t chaine::trouve(char c, int64_t pos) const
{
	return static_cast<int64_t>(m_chaine.find(c, static_cast<size_t>(pos)));
}

int64_t chaine::trouve(const chaine &motif, int64_t pos) const
{
	return static_cast<int64_t>(m_chaine.find(motif.c_str(), static_cast<size_t>(pos)));
}

int64_t chaine::trouve_premier_de(char c) const
{
	return static_cast<int64_t>(m_chaine.find_first_of(c));
}

int64_t chaine::trouve_premier_de(const chaine &c, int64_t pos) const
{
	return static_cast<int64_t>(m_chaine.find_first_of(c.c_str(), static_cast<size_t>(pos)));
}

int64_t chaine::trouve_premier_non_de(char c, int64_t pos) const
{
	return static_cast<int64_t>(m_chaine.find_first_not_of(c, static_cast<size_t>(pos)));
}

int64_t chaine::trouve_premier_non_de(chaine const &c, int64_t pos) const
{
	return static_cast<int64_t>(m_chaine.find_first_not_of(c.c_str(), static_cast<size_t>(pos)));
}

int64_t chaine::trouve_dernier_de(char c) const
{
	return static_cast<int64_t>(m_chaine.find_last_of(c));
}

int64_t chaine::trouve_dernier_non_de(char c) const
{
	return static_cast<int64_t>(m_chaine.find_last_not_of(c));
}

int64_t chaine::trouve_dernier_non_de(chaine const &c) const
{
	return static_cast<int64_t>(m_chaine.find_last_not_of(c.c_str()));
}

void chaine::insere(int64_t pos, int64_t combien, char c)
{
	m_chaine.insert(static_cast<size_t>(pos), static_cast<size_t>(combien), c);
}

void chaine::remplace(int64_t pos, int64_t combien, const chaine &motif)
{
	m_chaine.replace(static_cast<size_t>(pos), static_cast<size_t>(combien), motif.m_chaine);
}

const chaine::type_chaine &chaine::std_str() const
{
	return m_chaine;
}

chaine::iteratrice chaine::debut()
{
	return m_chaine.begin();
}

chaine::iteratrice chaine::fin()
{
	return m_chaine.end();
}

chaine::const_iteratrice chaine::debut() const
{
	return m_chaine.cbegin();
}

chaine::const_iteratrice chaine::fin() const
{
	return m_chaine.cend();
}

chaine::iteratrice_inverse chaine::debut_inverse()
{
	return m_chaine.rbegin();
}

chaine::iteratrice_inverse chaine::fin_inverse()
{
	return m_chaine.rend();
}

chaine::const_iteratrice_inverse chaine::debut_inverse() const
{
	return m_chaine.rbegin();
}

chaine::const_iteratrice_inverse chaine::fin_inverse() const
{
	return m_chaine.rend();
}

chaine &chaine::operator+=(char c)
{
	this->ajoute(c);
	return *this;
}

chaine &chaine::operator+=(const chaine &autre)
{
	this->append(autre);
	return *this;
}

void chaine::permute(chaine &autre)
{
	m_chaine.swap(autre.m_chaine);
}

/* ************************************************************************** */

std::ostream &operator<<(std::ostream &os, const chaine &c)
{
	os << c.c_str();
	return os;
}

std::istream &operator>>(std::istream &is, chaine &c)
{
	std::string s;
	is >> s;
	c = s;
	return is;
}

bool operator==(const chaine &c1, const chaine &c2)
{
	return std::strcmp(c1.c_str(), c2.c_str()) == 0;
}

bool operator==(chaine const &c1, vue_chaine const &c2)
{
	return vue_chaine(c1) == c2;
}

bool operator==(vue_chaine const &c1, chaine const &c2)
{
	return vue_chaine(c2) == c1;
}

bool operator==(chaine const &c1, vue_chaine_compacte const &c2)
{
	return vue_chaine_compacte(c1) == c2;
}

bool operator==(vue_chaine_compacte const &c1, chaine const &c2)
{
	return vue_chaine_compacte(c2) == c1;
}

bool operator==(chaine const &c1, char const *c2)
{
	return std::strcmp(c1.c_str(), c2) == 0;
}

bool operator==(char const *c1, chaine const &c2)
{
	return std::strcmp(c1, c2.c_str()) == 0;
}

bool operator!=(const chaine &c1, const chaine &c2)
{
	return !(c1 == c2);
}

bool operator!=(chaine const &c1, vue_chaine const &c2)
{
	return !(c1 == c2);
}

bool operator!=(vue_chaine const &c1, chaine const &c2)
{
	return !(c1 == c2);
}

bool operator!=(chaine const &c1, vue_chaine_compacte const &c2)
{
	return !(c1 == c2);
}

bool operator!=(vue_chaine_compacte const &c1, chaine const &c2)
{
	return !(c1 == c2);
}

bool operator!=(chaine const &c1, char const *c2)
{
	return !(c1 == c2);
}

bool operator!=(char const *c1, chaine const &c2)
{
	return !(c1 == c2);
}

bool operator<(const chaine &c1, const chaine &c2)
{
	return std::strcmp(c1.c_str(), c2.c_str()) < 0;
}

chaine operator+(const chaine &c1, const chaine &c2)
{
	auto tmp = chaine(c1);
	tmp.append(c2);
	return tmp;
}

chaine operator+(const char *c1, const chaine &c2)
{
	auto tmp = chaine(c1);
	tmp.append(c2);
	return tmp;
}

chaine operator+(const chaine &c1, const char *c2)
{
	auto tmp = c1;
	tmp.append(c2);
	return tmp;
}

}  /* namespace dls */
