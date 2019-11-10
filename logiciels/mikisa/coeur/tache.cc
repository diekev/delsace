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

	connect(this, &TaskNotifier::debut_tache, window, &FenetrePrincipale::tache_demarree);
	connect(this, &TaskNotifier::ajourne_progres, window, &FenetrePrincipale::ajourne_progres);
	connect(this, &TaskNotifier::fin_tache, window, &FenetrePrincipale::tache_terminee);
	connect(this, &TaskNotifier::debut_evaluation, window, &FenetrePrincipale::evaluation_debutee);
}

void TaskNotifier::signalImageProcessed()
{
	Q_EMIT(image_traitee());
}

void TaskNotifier::signalise_proces(int quoi)
{
	Q_EMIT(signal_proces(quoi));
}

void TaskNotifier::signale_debut_tache()
{
	Q_EMIT(debut_tache());
}

void TaskNotifier::signale_ajournement_progres(float progres)
{
	Q_EMIT(ajourne_progres(progres));
}

void TaskNotifier::signale_fin_tache()
{
	Q_EMIT(fin_tache());
}

void TaskNotifier::signale_debut_evaluation(const char *message, int execution, int total)
{
	Q_EMIT(debut_evaluation(message, execution, total));
}
