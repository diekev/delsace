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

#include "chaine.hh"

#include <cstring>
#include <utility>

#include "tableau.hh"

namespace kuri {

bool operator ==(const chaine &chn1, const chaine &chn2)
{
	if (chn1.taille() != chn2.taille()) {
		return false;
	}

	if (chn1.taille() == 0) {
		return true;
	}

	for (auto i = 0; i < chn1.taille(); ++i) {
		if (chn1[i] != chn2[i]) {
			return false;
		}
	}

	return true;
}

bool operator == (chaine const &chn1, chaine_statique const &chn2)
{
	return kuri::chaine_statique(chn1) == chn2;
}

bool operator == (chaine_statique const &chn1, chaine const &chn2)
{
	return kuri::chaine_statique(chn2) == chn1;
}

bool operator == (chaine const &chn1, const char *chn2)
{
	return kuri::chaine_statique(chn2) == chn1;
}

bool operator == (const char *chn1, chaine const &chn2)
{
	return kuri::chaine_statique(chn1) == chn2;
}

bool operator !=(const chaine &chn1, const chaine &chn2)
{
	return !(chn1 == chn2);
}

bool operator != (chaine const &chn1, chaine_statique const &chn2)
{
	return !(chn1 == chn2);
}

bool operator != (chaine_statique const &chn1, chaine const &chn2)
{
	return !(chn1 == chn2);
}

bool operator != (chaine const &chn1, const char *chn2)
{
	return !(chn1 == chn2);
}

bool operator != (const char *chn1, chaine const &chn2)
{
	return !(chn1 == chn2);
}

std::ostream &operator<<(std::ostream &os, const chaine &chn)
{
	POUR (chn) {
		os << it;
	}

	return os;
}

long distance_levenshtein(chaine_statique const &chn1, chaine_statique const &chn2)
{
	auto const m = chn1.taille();
	auto const n = chn2.taille();

	if (m == 0) {
		return n;
	}

	if (n == 0) {
		return m;
	}

	auto couts = tableau<long>(n + 1);

	for (auto k = 0; k <= n; k++) {
		couts[k] = k;
	}

	for (auto i = 0l; i < chn1.taille(); ++i) {
		couts[0] = i + 1;
		auto coin = i;

		for (auto j = 0; j < chn2.taille(); ++j) {
			auto enhaut = couts[j + 1];

			if (chn1.pointeur()[i] == chn2.pointeur()[j]) {
				couts[j + 1] = coin;
			}
			else {
				auto t = enhaut < coin ? enhaut : coin;
				couts[j + 1] = (couts[j] < t ? couts[j] : t) + 1;
			}

			coin = enhaut;
		}
	}

	return couts[n];
}

/*
chaine::chaine(Allocatrice &alloc)
	: m_allocatrice(&alloc)
{}

chaine::chaine(const chaine &autre)
{
	copie_donnees(autre);
}

chaine::chaine(chaine &&autre)
{
	permute(autre);
}

chaine &chaine::operator=(const chaine &autre)
{
	copie_donnees(autre);
	return *this;
}

chaine &chaine::operator=(chaine &&autre)
{
	permute(autre);
	return *this;
}

chaine::~chaine()
{
	m_allocatrice->deloge_tableau(m_pointeur, m_capacite);
}

void chaine::ajoute(char c)
{
	garantie_capacite(taille() + 1);
	m_pointeur[m_taille] = c;
	m_taille += 1;
}

void chaine::reserve(long taille)
{
	garantie_capacite(taille);
}

void chaine::redimensionne(long taille)
{
	garantie_capacite(taille);
	m_taille = taille;
}

long chaine::taille() const
{
	return m_taille;
}

long chaine::capacite() const
{
	return m_capacite;
}

const char *chaine::pointeur() const
{
	return m_pointeur;
}

chaine::operator bool()
{
	return !m_pointeur || m_taille == 0;
}

void chaine::permute(chaine &autre)
{
	using std::swap;
	swap(m_pointeur, autre.m_pointeur);
	swap(m_taille, autre.m_taille);
	swap(m_capacite, autre.m_capacite);
	swap(m_allocatrice, autre.m_allocatrice);
}

void chaine::copie_donnees(const chaine &autre)
{
	if (this == &autre) {
		return;
	}

	m_allocatrice->reloge_tableau("", m_pointeur, m_capacite, autre.m_taille);
	m_taille = autre.m_taille;
	m_capacite = autre.m_taille;
	memcpy(m_pointeur, autre.m_pointeur, static_cast<size_t>(autre.m_taille));
}

void chaine::garantie_capacite(long taille)
{
	if (taille <= m_capacite) {
		return;
	}

	m_allocatrice->reloge_tableau("", m_pointeur, m_capacite, taille);
	m_capacite = taille;
}

chaine_statique::chaine_statique(const char *pointeur_)
	: tableau_statique(pointeur_, static_cast<long>(strlen(pointeur_)))
{}

chaine_statique::chaine_statique(const chaine &chn)
	: tableau_statique(chn.pointeur(), chn.taille())
{}
*/

}
