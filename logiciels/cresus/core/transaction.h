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

#include <QString>
#include <QVariant>

class Transaction {
	QString m_categorie{};
	double m_valeur{};

	enum type {
		REVENUE,
		DEPENSE
	};

public:
	explicit Transaction(const double valeur)
	    : m_categorie("")
	    , m_valeur(valeur)
	{}

	/* Required for serialization */

	Transaction() = default;
	~Transaction() = default;

	Transaction(const Transaction &transac)
	    : m_categorie(transac.m_categorie)
	    , m_valeur(transac.m_valeur)
	{}

	/* ============================== operators ============================== */

	friend QDataStream &operator<<(QDataStream &out, const Transaction &transac);
	friend QDataStream &operator>>(QDataStream &in, Transaction &transac);
};

Q_DECLARE_METATYPE(Transaction)

QDataStream &operator<<(QDataStream &out, const Transaction &transac);
QDataStream &operator>>(QDataStream &in, Transaction &transac);
