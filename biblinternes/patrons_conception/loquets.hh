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

#include <atomic>
#include <thread>

namespace dls {

/* ************************************************************************** */

struct loquet_gyrant {
private:
	enum class etat : int {
		deverrouille = 0,
		verrouille = 1,
	};

	std::atomic<etat> m_verrou;

public:
	loquet_gyrant();

	void verrouille();

	void deverrouille();
};

/* ************************************************************************** */

struct loquet_gyrant_affamant {
private:
	enum class etat : int {
		deverrouille = 0,
		verrouille = 1,
	};

	std::atomic<etat> m_verrou;

public:
	loquet_gyrant_affamant();

	void verrouille();

	void deverrouille();
};

/* ************************************************************************** */

/* sans famine (au moins sur x86) */
class loquet_ticket {
	std::atomic<long> m_ticket;
	std::atomic<long> m_grant;

public:
	loquet_ticket();

	void verrouille();

	void deverrouille();
};

}  /* namespace dls */
