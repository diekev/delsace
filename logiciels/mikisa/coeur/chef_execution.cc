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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "chef_execution.hh"

#include "mikisa.h"
#include "tache.h"

ChefExecution::ChefExecution(Mikisa &mikisa)
	: m_mikisa(mikisa)
{}

bool ChefExecution::interrompu() const
{
	return m_mikisa.interrompu;
}

void ChefExecution::indique_progression(float progression)
{
	m_mikisa.notifiant_thread->signale_ajournement_progres(progression);
}

void ChefExecution::indique_progression_parallele(float delta)
{
	m_mutex_progression.lock();
	m_progression_parallele += delta;
	indique_progression(m_progression_parallele);
	m_mutex_progression.unlock();
}

void ChefExecution::demarre_evaluation(const char *message)
{
	m_progression_parallele = 0.0f;
	m_mikisa.notifiant_thread->signale_debut_evaluation(message);
}
