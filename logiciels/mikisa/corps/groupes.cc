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

#include "groupes.h"

#include <cassert>

#include "biblinternes/memoire/logeuse_memoire.hh"

/* ************************************************************************** */

static void supprime_liste(dls::tableau<size_t> *liste)
{
	memoire::deloge("groupe", liste);
}

/* ************************************************************************** */

void GroupePoint::ajoute_point(size_t index_point)
{
	detache();
	this->m_points->pousse(index_point);
}

void GroupePoint::remplace_index(long i, size_t j)
{
	detache();
	(*this->m_points)[i] = j;
}

void GroupePoint::reserve(long const nombre)
{
	assert(nombre >= 0);
	detache();
	this->m_points->reserve(nombre);
}

void GroupePoint::reinitialise()
{
	detache();
	m_points->clear();
}

long GroupePoint::taille() const
{
	if (m_points == nullptr) {
		return 0;
	}

	return m_points->taille();
}

bool GroupePoint::contiens(size_t index_point) const
{
	if (m_points == nullptr) {
		return false;
	}

	for (auto const i : (*m_points)) {
		if (i == index_point) {
			return true;
		}
	}

	return false;
}

size_t GroupePoint::index(long i) const
{
	return m_points->a(i);
}

void GroupePoint::detache()
{
	if (m_points == nullptr) {
		m_points = ptr_liste(memoire::loge<type_liste>("groupe"), supprime_liste);
		return;
	}

	if (!m_points.unique()) {
		m_points = ptr_liste(memoire::loge<type_liste>("groupe", *(m_points.get())), supprime_liste);
	}
}

/* ************************************************************************** */

void GroupePrimitive::ajoute_primitive(size_t index_poly)
{
	detache();
	this->m_primitives->pousse(index_poly);
}

void GroupePrimitive::remplace_index(long i, size_t j)
{
	(*this->m_primitives)[i] = j;
}

void GroupePrimitive::reserve(long const nombre)
{
	detache();
	assert(nombre >= 0);
	this->m_primitives->reserve(nombre);
}

void GroupePrimitive::reinitialise()
{
	detache();
	m_primitives->clear();
}

long GroupePrimitive::taille() const
{
	if (m_primitives == nullptr) {
		return 0;
	}

	return m_primitives->taille();
}

size_t GroupePrimitive::index(long i) const
{
	return m_primitives->a(i);
}

void GroupePrimitive::detache()
{
	if (m_primitives == nullptr) {
		m_primitives = ptr_liste(memoire::loge<type_liste>("groupe"), supprime_liste);
		return;
	}

	if (!m_primitives.unique()) {
		m_primitives = ptr_liste(memoire::loge<type_liste>("groupe", *(m_primitives.get())), supprime_liste);
	}
}

/* ************************************************************************** */

iteratrice_index::iteratrice::iteratrice(long nombre)
	: m_est_groupe(false)
	, m_etat_nombre(nombre)
{}

iteratrice_index::iteratrice::iteratrice(GroupePoint *groupe_point)
	: m_est_groupe(true)
	, gpnt(groupe_point)
{}

iteratrice_index::iteratrice::iteratrice(GroupePrimitive *groupe_primitive)
	: m_est_groupe(true)
	, gprm(groupe_primitive)
{}

long iteratrice_index::iteratrice::operator*()
{
	if (gpnt) {
		return static_cast<long>(gpnt->index(m_etat_nombre));
	}

	if (gprm) {
		return static_cast<long>(gprm->index(m_etat_nombre));
	}

	return m_etat_nombre;
}

iteratrice_index::iteratrice &iteratrice_index::iteratrice::operator++()
{
	++m_etat_nombre;
	return *this;
}

bool iteratrice_index::iteratrice::est_egal(iteratrice_index::iteratrice it)
{
	return this->m_etat_nombre == it.m_etat_nombre;
}

/* ************************************************************************** */

iteratrice_index::iteratrice_index(long nombre)
	: m_nombre(nombre)
	, m_courant(0)
{}

iteratrice_index::iteratrice_index(GroupePoint *groupe_point)
	: m_courant(0)
	, gpnt(groupe_point)
{}

iteratrice_index::iteratrice_index(GroupePrimitive *groupe_primitive)
	: m_courant(0)
	, gprm(groupe_primitive)
{}

iteratrice_index::iteratrice iteratrice_index::begin()
{
	if (gpnt) {
		return iteratrice(gpnt);
	}

	if (gprm) {
		return iteratrice(gprm);
	}

	return iteratrice(m_courant);
}

iteratrice_index::iteratrice iteratrice_index::end()
{
	if (gpnt) {
		return iteratrice(gpnt->taille());
	}

	if (gprm) {
		return iteratrice(gprm->taille());
	}

	return iteratrice(m_nombre);
}
