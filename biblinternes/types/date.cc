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

#include "date.h"

namespace dls {
namespace types {

Jour::Jour(int jour)
	: valeur(jour)
{
	if (jour < 1) {
		throw ExceptionJourInvalide();
	}
}

Mois::Mois(int mois)
	: valeur(mois)
{
	if (mois < 1) {
		throw ExceptionMoisInvalide();
	}
}

Annee::Annee(int yy)
	: valeur(yy)
{}

Date::Date(const Jour &j, const Mois &m, const Annee &a)
	: m_jour(j.valeur)
	, m_mois(m.valeur)
	, m_annee(a.valeur)
{
	verifie_date();
}

Date::Date(const Annee &a, const Mois &m, const Jour &j)
	: Date(j, m, a)
{}

Date::Date(const Mois &m, const Jour &j, const Annee &a)
	: Date(j, m, a)
{}

Date::Date(const Date &autre)
	: m_jour(autre.m_jour)
	, m_mois(autre.m_mois)
	, m_annee(autre.m_annee)
{
	verifie_date();
}

const char *Date::nom_jour() const
{
	const int t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	const auto jj = this->m_jour;
	const auto mm = this->m_mois;
	const auto aa = this->m_annee - int(mm < 3);
	const auto &idx = (aa + aa / 4 - aa / 100 + aa / 400 + t[mm - 1] + jj);

	return m_noms_des_jours[idx % 7];
}

const char *Date::nom_mois() const
{
	return m_noms_des_mois[this->m_mois - 1];
}

bool Date::est_annee_bissextile() const
{
	if ((this->m_annee % 400) == 0) {
		return true;
	}

	return !bool(this->m_annee % 4);
}

size_t Date::jours() const
{
	auto jours_ecoules_annee_courante = 0l;

	/* don't take into account the current month */
	for (int i(0); i < this->m_mois - 1; ++i) {
		jours_ecoules_annee_courante += this->m_jours_par_mois[i];
	}

	jours_ecoules_annee_courante += this->m_jour;

	const auto annees = std::max(0, this->m_annee - 1);

	return static_cast<size_t>(static_cast<long>(static_cast<float>(annees) * 365.25f) + jours_ecoules_annee_courante);
}

size_t Date::jours_jusque(const Date &autre) const
{
	return jours_entre(*this, autre);
}

size_t Date::jours_depuis(const Date &autre) const
{
	return jours_entre(autre, *this);
}

size_t Date::mois() const
{
	return static_cast<size_t>(this->m_mois + (this->m_annee * this->MOIS_PAR_ANNEE));
}

size_t Date::annees() const
{
	return static_cast<size_t>(this->m_annee);
}

Date &Date::operator=(const Date &other)
{
	this->m_jour = other.m_jour;
	this->m_mois = other.m_mois;
	this->m_annee = other.m_annee;
	return *this;
}

const Date &Date::operator+=(const Date &other)
{
	ajoute_jour(other.m_jour);
	ajoute_mois(other.m_mois);
	ajoute_annee(other.m_annee);
	return *this;
}

const Date &Date::operator+=(const Jour &dd)
{
	ajoute_jour(dd.valeur);
	return *this;
}

const Date &Date::operator+=(const Mois &mm)
{
	ajoute_mois(mm.valeur);
	return *this;
}

const Date &Date::operator+=(const Annee &yy)
{
	ajoute_annee(yy.valeur);
	return *this;
}

const Date &Date::operator++()
{
	ajoute_jour(1);
	return *this;
}

const Date &Date::operator-=(const Date &other)
{
	soustrait_jour(other.m_jour);
	soustrait_mois(other.m_mois);
	soustrait_annee(other.m_annee);
	return *this;
}

const Date &Date::operator-=(const Jour &dd)
{
	soustrait_jour(dd.valeur);
	return *this;
}

const Date &Date::operator-=(const Mois &mm)
{
	soustrait_mois(mm.valeur);
	return *this;
}

const Date &Date::operator-=(const Annee &yy)
{
	soustrait_annee(yy.valeur);
	return *this;
}

const Date &Date::operator--()
{
	soustrait_jour(1);
	return *this;
}

void Date::verifie_date()
{
	verifie_annee_bissextile();

	if (!est_valide()) {
		throw ExceptionDateInvalide();
	}
}

bool Date::est_valide() const
{
	return     (this->m_mois <= MOIS_PAR_ANNEE)
			&& (this->m_jour <= m_jours_par_mois[this->m_mois - 1]);
}

void Date::ajoute_jour(int dd)
{
	this->m_jour += dd;

	while (this->m_jour > m_jours_par_mois[this->m_mois - 1]) {
		this->m_jour -= m_jours_par_mois[this->m_mois - 1];
		ajoute_mois(1);
	}
}

void Date::soustrait_jour(int dd)
{
	this->m_jour -= dd;

	while (this->m_jour < 1) {
		soustrait_mois(1);
		this->m_jour += m_jours_par_mois[this->m_mois - 1];
	}
}

void Date::ajoute_mois(int mm)
{
	this->m_mois += mm;

	while (this->m_mois > MOIS_PAR_ANNEE) {
		this->m_mois -= MOIS_PAR_ANNEE;
		ajoute_annee(1);
	}
}

void Date::soustrait_mois(int mm)
{
	this->m_mois -= mm;

	while (this->m_mois < 1) {
		this->m_mois += MOIS_PAR_ANNEE;
		soustrait_annee(1);
	}
}

void Date::ajoute_annee(int yy)
{
	this->m_annee += yy;
	verifie_annee_bissextile();
}

void Date::soustrait_annee(int yy)
{
	ajoute_annee(-yy);
}

void Date::verifie_annee_bissextile()
{
	this->m_jours_par_mois[1] = (est_annee_bissextile() ? 29 : 28);
}

size_t Date::jours_entre(const Date &d1, const Date &d2) const
{
	auto jours_entre_mois = 0;

	for (int i(d1.m_mois); i < d2.m_mois - 1; ++i) {
		jours_entre_mois += m_jours_par_mois[i];
	}

	const auto jours_restant_mois = m_jours_par_mois[d1.m_mois - 1] - d1.m_jour;
	const auto jours_dans_annee = static_cast<int>(
									  static_cast<float>(d2.m_annee - d1.m_annee) * 365.25f);

	return static_cast<size_t>(jours_restant_mois + jours_entre_mois + jours_dans_annee + d2.m_jour);
}

}  /* namespace types */
}  /* namespace dls */
