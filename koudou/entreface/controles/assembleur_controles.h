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

#pragma once

#include <QObject>
#include <unordered_map>

class QGridLayout;

using widget_pair = std::pair<QWidget *, QWidget *>;

class AssembleurControles {
	QGridLayout *m_agencement;
	QWidget *m_dernier_controle;
	int m_compte_items;

	std::vector<QWidget *> m_controles;
	std::unordered_map<std::string, widget_pair> m_tableau_controle;

public:
	explicit AssembleurControles(QGridLayout *layout);
	~AssembleurControles();

	void addWarning(const QString &warning);
	void addWidget(QWidget *widget, const QString &name);
	void setTooltip(const QString &tooltip);

	void clear();

	void setVisible(bool yesno);
	void setVisible(const QString &name, bool yesno);

	template <typename SlotType>
	void setContext(QObject *context, SlotType slot)
	{
		for (auto &controle : m_controles) {
			QObject::connect(controle, SIGNAL(controle_change()), context, slot);
		}
	}
};
