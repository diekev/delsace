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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "controle_propriete.h"

class QHBoxLayout;
class QLineEdit;
class QMenu;
class QPushButton;

namespace danjo {

class SelecteurListe : public ControlePropriete {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QLineEdit *m_line_edit;
	QPushButton *m_push_button;
	QMenu *m_list_widget;

	bool m_input;

public:
	explicit SelecteurListe(QWidget *parent = nullptr);

	~SelecteurListe();

	void setValue(const QString &text);

	void addField(const QString &text);

private Q_SLOTS:
	void showList();
	void handleClick();
	void updateText();

Q_SIGNALS:
	void valeur_changee(const QString &text);
};

class ControleProprieteListe final : public SelecteurListe {
	Q_OBJECT

	std::string *m_pointeur;

public:
	explicit ControleProprieteListe(QWidget *parent = nullptr);

	~ControleProprieteListe() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(const QString &valeur);
};

}  /* namespace danjo */
