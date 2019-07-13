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

#pragma once

#include "transaction.h"

#include <QVector>

enum {
	COMPTE_COURANT = 0,
	COMPTE_EPARGNE = 1,
};

class Compte {
	QString m_nom;
	float m_balance;
	int m_type;
	bool m_is_blocked;
	QVector<Transaction> m_transactions;

public:
	/* Required for serialization */

	Compte() = default;
	~Compte() = default;

	Compte(const Compte &compte)
		: m_nom(compte.m_nom)
		, m_balance(compte.m_balance)
		, m_type(compte.m_type)
		, m_is_blocked(compte.m_is_blocked)
		, m_transactions(compte.m_transactions)
	{}

	/* ============================= main methods ============================ */

	auto setName(const QString &name) -> void
	{
		this->m_nom = name;
	}

	auto name() const -> QString
	{
		return this->m_nom;
	}

	auto value() const -> float
	{
		return this->m_balance;
	}

	auto setValue(const float valeur) -> void
	{
		this->m_balance = valeur;
	}

	auto type() const -> int
	{
		return this->m_type;
	}

	auto setType(const int type) -> void
	{
		this->m_type = type;
	}

	auto setDeposit(const float montant) -> void
	{
		this->m_balance += montant;
	}

	auto setWithdrawal(const float montant) -> void
	{
		this->m_balance -= montant;
	}

	auto addTransaction(const Transaction &transac) -> void
	{
		this->m_transactions.pousse(transac);
	}

	auto addTransaction(const float value, const bool income) -> void
	{
		this->m_balance += (income) ? value : -value;
	}

	auto isBlocked() const -> bool
	{
		return this->m_is_blocked;
	}

	auto setBlocked(const bool b) -> void
	{
		this->m_is_blocked = b;
	}

	/* ============================== operators ============================== */

	friend QDataStream &operator<<(QDataStream &out, const Compte &compte);
	friend QDataStream &operator>>(QDataStream &in, Compte &compte);
};

Q_DECLARE_METATYPE(Compte)

QDataStream &operator<<(QDataStream &out, const Compte &compte);
QDataStream &operator>>(QDataStream &in, Compte &compte);
