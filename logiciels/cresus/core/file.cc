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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "utilisateur.h"

#include <QDataStream>
#include <QVector>

QDataStream &operator<<(QDataStream &out, const Compte &compte)
{
	out << compte.m_nom
		<< compte.m_balance
		<< compte.m_type
		<< compte.m_is_blocked
		<< compte.m_transactions;

	return out;
}

QDataStream &operator>>(QDataStream &in, Compte &compte)
{
	compte = Compte();

	in >> compte.m_nom;
	in >> compte.m_balance;
	in >> compte.m_type;
	in >> compte.m_is_blocked;
	in >> compte.m_transactions;

	return in;
}

QDataStream &operator<<(QDataStream &out, const Transaction &transac)
{
	out << transac.m_categorie
		<< transac.m_valeur;

	return out;
}

QDataStream &operator>>(QDataStream &in, Transaction &transac)
{
	transac = Transaction();

	in >> transac.m_categorie;
	in >> transac.m_valeur;

	return in;
}

QDataStream &operator<<(QDataStream &out, const Utilisateur &user)
{
	out << user.m_nom
		<< user.m_accounts
		<< user.m_transactions
		<< user.m_argent_liquide
		<< user.m_net_value
		<< user.m_dette
		<< user.m_liquid_capital;

	return out;
}

QDataStream &operator>>(QDataStream &in, Utilisateur &user)
{
	user = Utilisateur();

	in >> user.m_nom;
	in >> user.m_accounts;
	in >> user.m_transactions;
	in >> user.m_argent_liquide;
	in >> user.m_net_value;
	in >> user.m_dette;
	in >> user.m_liquid_capital;

	return in;
}
