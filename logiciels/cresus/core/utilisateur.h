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

#include "compte.h"

class Utilisateur {
	QString m_nom{};
	QVector<Compte> m_accounts{};
	QVector<Transaction> m_transactions{};

	double m_argent_liquide{};
	double m_net_value{};
	double m_dette{};
	double m_liquid_capital{};

	auto updateNetValue() -> void
	{
		m_net_value = m_argent_liquide - m_dette;
		m_liquid_capital = m_argent_liquide;

		for (const auto &account : m_accounts) {
			auto value = account.value();
			m_net_value += value;

			if (!account.isBlocked()) {
				m_liquid_capital += value;
			}
		}
	}

public:
	explicit Utilisateur(QString name)
		: m_nom(std::move(name))
	{
		m_argent_liquide = m_net_value = m_dette = m_liquid_capital = 0.0;
		m_accounts.reserve(10);
		m_transactions.reserve(10);
	}

	/* Required for serialization */

	Utilisateur() = default;
	~Utilisateur() = default;

	Utilisateur(const Utilisateur &user)
		: m_nom(user.name())
		, m_accounts(user.accounts())
		, m_transactions(user.m_transactions)
	    , m_argent_liquide(user.m_argent_liquide)
		, m_net_value(user.netValue())
		, m_dette(user.m_dette)
		, m_liquid_capital(user.m_liquid_capital)
	{}

	/* ============================= main methods ============================ */

	auto name() const -> QString
	{
		return m_nom;
	}

	auto setName(QString &&name) noexcept -> void
	{
		m_nom = std::move(name);
	}

	auto netValue() -> double&
	{
		updateNetValue();
		return m_net_value;
	}

	auto netValue() const -> double
	{
		return m_net_value;
	}

	auto setArgentLiquide(const double valeur) -> void
	{
		m_argent_liquide = valeur;
	}

	auto getArgentLiquide() const -> double
	{
		return m_argent_liquide;
	}

	auto setDebt(const double valeur) -> void
	{
		m_dette = valeur;
	}

	auto addAccount(const Compte &account) -> void
	{
		m_accounts.push_back(account);
	}

	auto account(int index) -> Compte&
	{
		return m_accounts[index];
	}

	auto account(int index) const -> Compte
	{
		return m_accounts[index];
	}

	auto accounts() -> QVector<Compte>&
	{
		return m_accounts;
	}

	auto accounts() const -> QVector<Compte>
	{
		return m_accounts;
	}

	/* ============================== operators ============================== */

	friend QDataStream &operator<<(QDataStream &out, const Utilisateur &user);
	friend QDataStream &operator>>(QDataStream &in, Utilisateur &user);
};

Q_DECLARE_METATYPE(Utilisateur)

QDataStream &operator<<(QDataStream &out, const Utilisateur &user);
QDataStream &operator>>(QDataStream &in, Utilisateur &user);
