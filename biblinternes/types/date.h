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

#pragma once

#include <ostream>

namespace dls {
namespace types {

struct ExceptionJourInvalide {};
struct ExceptionDateInvalide {};
struct ExceptionMoisInvalide {};

struct Jour {
	int valeur;

	explicit Jour(int jour);
};

struct Mois {
	unsigned int valeur;

	explicit Mois(unsigned int mois);

	enum {
		JANUARY   = 1,
		FEBRUARY  = 2,
		MARCH     = 3,
		APRIL     = 4,
		MAY       = 5,
		JUNE      = 6,
		JULY      = 7,
		AUGUST    = 8,
		SEPTEMBER = 9,
		OCTOBER   = 10,
		NOVEMBER  = 11,
		DECEMBER  = 12,
	};
};

struct Annee {
	int valeur;

	explicit Annee(int yy);
};

class Date {
	int m_jour;
	unsigned int m_mois;
	int m_annee;
	const unsigned int MOIS_PAR_ANNEE = 12;

	unsigned int m_jours_par_mois[12] = {
	    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	const char *m_noms_des_jours[7] = {
	    "Sunday",
	    "Monday",
	    "Tuesday",
	    "Wednesday",
	    "Thursday",
	    "Friday",
	    "Saturday"
	};

	const char *m_noms_des_mois[12] = {
	    "January",
	    "February",
	    "Mars",
	    "April",
	    "May",
	    "June",
	    "July",
	    "August",
	    "Septempber",
	    "October",
	    "November",
	    "December"
	};

public:
	Date() = delete;
	~Date() = default;

	/**
	 * Construit une date à partir du format européen : jour/mois/année.
	 */
	Date(const Jour &j, const Mois &m, const Annee &a);

	/**
	 * Construit une date à partir du format asiatique : année/mois/jour.
	 */
	Date(const Annee &a, const Mois &m, const Jour &j);

	/**
	 * Construit une date à partir du format américain : mois/jour/année.
	 */
	Date(const Mois &m, const Jour &j, const Annee &a);

	/**
	 * Construit une date à partir d'une autre date. La date construite sera une
	 * copie de l'autre, et après l'opération l'assertion '*this == autre' sera
	 * vraie.
	 */
	Date(const Date &autre);

	/* ============================= main methods ============================ */

	/**
	 * Retourne le nom du jour courant.
	 */
	const char *nom_jour() const;

	/**
	 * Retourne le nom du mois courant.
	 */
	const char *nom_mois() const;

	/**
	 * Retourne si oui ou non la date est sur une année bissextile.
	 */
	bool est_annee_bissextile() const;

	/**
	 * Retourne le nombre de jours écoulés entre l'an 0 et cette date.
	 */
	size_t jours() const;

	/**
	 * Retourne le nombre jours entre cette date et l'autre date spécifiée.
	 * L'autre date doit être antérieure à celle-ci.
	 */
	size_t jours_jusque(const Date &autre) const;

	/**
	 * Retourne le nombre jours entre cette date et l'autre date spécifiée.
	 * L'autre date doit être postérieure à celle-ci.
	 */
	size_t jours_depuis(const Date &autre) const;

	/**
	 * Retourne le nombre de mois écoulés entre l'an 0 et cette date.
	 */
	size_t mois() const;

	/**
	 * Retourne le nombre d'années écoulés entre l'an 0 et cette date.
	 */
	size_t annees() const;

	/* ============================== operators ============================== */

	Date &operator=(const Date &other);

	const Date &operator+=(const Date &other);

	const Date &operator+=(const Jour &dd);

	const Date &operator+=(const Mois &mm);

	const Date &operator+=(const Annee &yy);

	const Date &operator++();

	const Date &operator-=(const Date &other);

	const Date &operator-=(const Jour &dd);

	const Date &operator-=(const Mois &mm);

	const Date &operator-=(const Annee &yy);

	const Date &operator--();

	friend std::ostream &operator<<(std::ostream &os, const Date &d);

private:
	void verifie_date();

	bool est_valide() const;

	void ajoute_jour(int dd);

	void soustrait_jour(int dd);

	void ajoute_mois(unsigned int mm);

	void soustrait_mois(unsigned int mm);

	void ajoute_annee(int yy);

	void soustrait_annee(int yy);

	inline void verifie_annee_bissextile();

	size_t jours_entre(const Date &d1, const Date &d2) const;
};

inline Date operator++(Date &d, int)
{
	Date tmp(d);
	++d;
	return tmp;
}

inline Date operator--(Date &d, int)
{
	Date tmp(d);
	--d;
	return tmp;
}

inline Date operator+(const Date &d1, const Date &d2)
{
	Date result(d1);
	result += d2;
	return result;
}

inline Date operator+(const Date &d1, const Jour &dd)
{
	Date result(d1);
	result += dd;
	return result;
}

inline Date operator+(const Date &d1, const Mois &mm)
{
	Date result(d1);
	result += mm;
	return result;
}

inline Date operator+(const Date &d1, const Annee &yy)
{
	Date result(d1);
	result += yy;
	return result;
}

inline Date operator-(const Date &d1, const Date &d2)
{
	Date result(d1);
	result -= d2;
	return result;
}

inline Date operator-(const Date &d1, const Jour &dd)
{
	Date result(d1);
	result -= dd;
	return result;
}

inline Date operator-(const Date &d1, const Mois &mm)
{
	Date result(d1);
	result -= mm;
	return result;
}

inline Date operator-(const Date &d1, const Annee &yy)
{
	Date result(d1);
	result -= yy;
	return result;
}

inline bool operator==(const Date &d1, const Date &d2)
{
	return     (d1.jours() == d2.jours())
			&& (d1.mois() == d2.mois())
			&& (d1.annees() == d2.annees());
}

inline bool operator!=(const Date &d1, const Date &d2)
{
	return !(d1 == d2);
}

inline bool operator<(const Date &d1, const Date &d2)
{
	return     (d1.jours() < d2.jours())
			|| (d1.mois() < d2.mois())
			|| (d1.annees() < d2.annees());
}

inline bool operator<=(const Date &d1, const Date &d2)
{
	return (d1 < d2) || (d1 == d2);
}

inline bool operator>(const Date &d1, const Date &d2)
{
	return     (d1.jours() > d2.jours())
			|| (d1.mois() > d2.mois())
			|| (d1.annees() > d2.annees());
}

inline bool operator>=(const Date &d1, const Date &d2)
{
	return (d1 > d2) || (d1 == d2);
}

inline std::ostream &operator<<(std::ostream &os, const Date &date)
{
	os << date.m_jour << " " << date.m_mois << " " << date.m_annee;
	return os;
}

}  /* namespace types */
}  /* namespace dls */
