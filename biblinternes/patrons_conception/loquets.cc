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

#include "loquets.hh"

namespace dls {

/* ************************************************************************** */

loquet_gyrant::loquet_gyrant()
	: m_verrou(etat::deverrouille)
{}

void loquet_gyrant::verrouille()
{
	while (m_verrou.exchange(etat::verrouille) == etat::verrouille) {}
}

void loquet_gyrant::deverrouille()
{
	m_verrou.exchange(etat::deverrouille);
}

/* ************************************************************************** */

loquet_gyrant_affamant::loquet_gyrant_affamant()
	: m_verrou(etat::deverrouille)
{}

void loquet_gyrant_affamant::verrouille()
{
	auto tmp = etat::deverrouille;

	while (!m_verrou.compare_exchange_strong(tmp, etat::verrouille)) {
		std::this_thread::yield();
		tmp = etat::deverrouille;
	}
}

void loquet_gyrant_affamant::deverrouille()
{
	m_verrou.store(etat::deverrouille, std::memory_order_release);
}

/* ************************************************************************** */

loquet_ticket::loquet_ticket()
	: m_ticket(0)
	, m_grant(0)
{}

void loquet_ticket::verrouille()
{
	auto ticket = m_ticket.fetch_add(1);

	while (ticket != m_grant.load()) {
		std::this_thread::yield();
	}
}

void loquet_ticket::deverrouille()
{
	auto grant = m_grant.load(std::memory_order_relaxed);
	m_grant.store(grant + 1, std::memory_order_release);
}

}  /* namespace dls */
