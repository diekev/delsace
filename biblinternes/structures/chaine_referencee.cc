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

#include "chaine_referencee.hh"

namespace dls {

reference_chaine::__detentrice::__detentrice(const char *str, long taille)
	: m_chaine(str)
	, m_taille(taille)
{}

const char *reference_chaine::__detentrice::c_str() const
{
	return m_chaine;
}

long reference_chaine::__detentrice::taille() const
{
	return m_taille;
}

unsigned int reference_chaine::__detentrice::empreinte() const
{
	return m_empreinte;
}

void reference_chaine::__detentrice::reference()
{
	m_compte_ref.fetch_add(1);
}

void reference_chaine::__detentrice::dereference()
{
	m_compte_ref.fetch_add(-1);
}

int reference_chaine::__detentrice::compte_ref()
{
	return m_compte_ref.load();
}

/* ************************************************************************** */

reference_chaine::__detentrice *reference_chaine::alloc_detentrice(const char *str, long taille)
{
	return new __detentrice(str, taille);
}

reference_chaine::reference_chaine(const char *str)
	: reference_chaine(str, static_cast<long>(std::strlen(str)))
{}

reference_chaine::reference_chaine(const char *str, long taille)
{
	if (str && taille && taille <= std::numeric_limits<unsigned int>::max()) {
		m_taille = taille;
		//_empreinte = calcule_empreinte(str);
		/* garde un pointeur vers la chaine littérale */
		m_donnees_si_chars = str;
	}
	else {
		m_taille = 0;
		m_empreinte = 0;
		m_donnees_si_detentrice = (str && taille) ? singletonEmptyString__detentrice : alloc_detentrice(str, taille);
	}
}

reference_chaine::reference_chaine(const reference_chaine &src)
	: m_donnees_si_chars(src.m_donnees_si_chars)
	, m_taille(src.m_taille)
	, m_empreinte(src.m_empreinte)
{
	if (detentrice() != nullptr) {
		detentrice()->reference();
	}
}

reference_chaine::~reference_chaine()
{
	if (detentrice() != nullptr) {
		detentrice()->dereference();

		if (detentrice()->compte_ref() == 0) {
			delete m_donnees_si_detentrice;
		}
	}
}

const char *reference_chaine::c_str() const
{
	return (detentrice() != nullptr) ? detentrice()->c_str() : m_donnees_si_chars;
}

long reference_chaine::taille() const
{
	return (detentrice() != nullptr) ? detentrice()->taille() : m_taille;
}

unsigned int reference_chaine::empreinte() const
{
	return (detentrice() != nullptr) ? detentrice()->empreinte() : m_empreinte;
}

reference_chaine::__detentrice *reference_chaine::detentrice()
{
	return (m_taille != 0) ? nullptr : m_donnees_si_detentrice;
}

const reference_chaine::__detentrice *reference_chaine::detentrice() const
{
	return (m_taille != 0) ? nullptr : m_donnees_si_detentrice;
}

/* ************************************************************************** */

detentrice_chaine::detentrice_chaine(const char *str, long taille)
	: reference_chaine()
{
	if (str && taille) {
		m_taille = 0;
		m_empreinte = 0;
		m_donnees_si_detentrice = alloc_detentrice(str, taille);
	}
	else {
		m_taille = 0;
		m_empreinte = 0;
		m_donnees_si_detentrice = singletonEmptyString__detentrice;
	}
}

detentrice_chaine::detentrice_chaine(const reference_chaine &src)
{
	if (!src.detentrice()) {
		/* duplique toujours la source */
		m_donnees_si_detentrice = alloc_detentrice(src.c_str(), src.taille());
	}
	else {
		m_donnees_si_detentrice = const_cast<__detentrice *>(src.detentrice());
		m_donnees_si_detentrice->reference();
	}

	m_taille = 0;
	m_empreinte = src.empreinte();
}

}  /* namespace dls */
