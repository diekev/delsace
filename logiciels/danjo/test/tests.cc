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

#include "test_decoupage.h"
#include "test_initialisation.h"

#include <QApplication>

int main(int argc, char **argv)
{
	/* Il nous faut une application pour pouvoir construire les QWidgets
	 * nécessaires à l'initialisation des propriétés. */
	QApplication app(argc, argv);

	auto controleuse = dls::test_unitaire::Controleuse{};
	controleuse.ajoute_fonction(test_decoupage);
	controleuse.ajoute_fonction(test_initialisation);

	controleuse.performe_controles();

	controleuse.imprime_resultat();
}
