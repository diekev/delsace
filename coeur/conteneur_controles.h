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

namespace kangao {

/**
 * La classe ConteneurControles sert de classe de base pour faire le pont entre
 * l'interface et le programme.
 *
 * Cette classe contiendra la disposition provenant de la compilation d'un
 * script, et à chaque fois qu'un contrôle est modifié dans l'interface,
 * le Q_SLOT ajourne_manipulable est appelé pour que le programme répondent au
 * changement dans l'interface.
 */
class ConteneurControles : public QWidget {
	Q_OBJECT

public:
	explicit ConteneurControles(QWidget *parent = nullptr);

public Q_SLOTS:
	/**
	 * Cette méthode est appelée à chaque qu'un contrôle associé est modifiée
	 * dans l'interface.
	 */
	virtual void ajourne_manipulable() = 0;
};

}  /* namespace kangao */
