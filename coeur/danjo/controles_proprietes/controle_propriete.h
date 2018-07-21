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

#include <QWidget>

namespace danjo {

struct DonneesControle;
struct Propriete;

/**
 * La classe Controle donne l'interface nécessaire pour les contrôles à afficher
 * dans l'interface utilisateur. Dès que le contrôle est changé le signal
 * Controle::controle_change() est émis.
 */
class ControlePropriete : public QWidget {
	Q_OBJECT

protected:
	Propriete *m_propriete = nullptr;
	int m_temps = 0;
	bool m_animation = false;

public:
	explicit ControlePropriete(QWidget *parent = nullptr);

	/**
	 * Finalise le contrôle. Cette fonction est appelée à la fin de la création
	 * du contrôle par l'assembleur de contrôle.
	 */
	virtual void finalise(const DonneesControle &donnees) = 0;

	void proriete(Propriete *p);

	void temps(int t);

Q_SIGNALS:
	/**
	 * Signal émis quand la valeur du contrôle est changée dans l'interface.
	 */
	void controle_change();
};

}  /* namespace danjo */
