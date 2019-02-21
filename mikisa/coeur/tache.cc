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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tache.h"

#include "entreface/fenetre_principale.h"

TaskNotifier::TaskNotifier(FenetrePrincipale *window)
{
	if (!window) {
		return;
	}

	connect(this, SIGNAL(image_traitee()), window, SLOT(image_traitee()));
	connect(this, SIGNAL(signal_proces(int)), window, SLOT(signale_proces(int)));
}

void TaskNotifier::signalImageProcessed()
{
	Q_EMIT(image_traitee());
}

void TaskNotifier::signalise_proces(int quoi)
{
	Q_EMIT(signal_proces(quoi));
}
