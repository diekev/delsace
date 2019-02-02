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

#include "tache.h"

#include "koudou.h"

#include "entreface/fenetre_principale.h"

NotaireTache::NotaireTache(FenetrePrincipale *fenetre)
{
	if (!fenetre) {
		return;
	}

	connect(this, &NotaireTache::rendu_fini, fenetre, &FenetrePrincipale::rendu_fini);
	connect(this, &NotaireTache::progres_avance, fenetre, &FenetrePrincipale::progres_avance);
	connect(this, &NotaireTache::tache_fini, fenetre, &FenetrePrincipale::tache_fini);
	connect(this, &NotaireTache::tache_commence, fenetre, &FenetrePrincipale::tache_commence);
	connect(this, &NotaireTache::progres_temps, fenetre, &FenetrePrincipale::progres_temps);
}

void NotaireTache::signale_rendu_fini()
{
	Q_EMIT(rendu_fini());
}

void NotaireTache::signale_tache_commence()
{
	Q_EMIT(tache_commence());
}

void NotaireTache::signale_tache_fini()
{
	Q_EMIT(tache_fini());
}

void NotaireTache::signale_progres_avance(float progres)
{
	Q_EMIT(progres_avance(progres));
}

void NotaireTache::signale_progres_temps(unsigned int echantillon, double temps_echantillon, double temps_ecoule, double temps_restant)
{
	Q_EMIT(progres_temps(echantillon, temps_echantillon, temps_ecoule, temps_restant));
}

Tache::Tache(Koudou const &koudou)
	: m_notaire(new NotaireTache(koudou.fenetre_principale))
	, m_koudou(koudou)
{}

tbb::task *Tache::execute()
{
	m_notaire->signale_tache_commence();

	this->commence(this->m_koudou);

	m_notaire->signale_tache_fini();

	return nullptr;
}
