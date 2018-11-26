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

#include "contexte_rendu.h"

const dls::math::mat4x4f &ContexteRendu::projection() const
{
	return m_projection;
}

void ContexteRendu::projection(const dls::math::mat4x4f &matrice)
{
	m_projection = matrice;
}

const dls::math::vec3f &ContexteRendu::vue() const
{
	return m_vue;
}

void ContexteRendu::vue(const dls::math::vec3f &matrice)
{
	m_vue = matrice;
}

const dls::math::mat3x3f &ContexteRendu::normal() const
{
	return m_normal;
}

void ContexteRendu::normal(const dls::math::mat3x3f &matrice)
{
	m_normal = matrice;
}

const dls::math::mat4x4f &ContexteRendu::MVP() const
{
	return m_modele_vue_projection;
}

void ContexteRendu::MVP(const dls::math::mat4x4f &matrice)
{
	m_modele_vue_projection = matrice;
}

const dls::math::mat4x4f &ContexteRendu::matrice_objet() const
{
	return m_matrice_objet;
}

void ContexteRendu::matrice_objet(const dls::math::mat4x4f &matrice)
{
	m_matrice_objet = matrice;
}

bool ContexteRendu::pour_surlignage() const
{
	return m_pour_surlignage;
}

void ContexteRendu::pour_surlignage(bool ouinon)
{
	m_pour_surlignage = ouinon;
}

bool ContexteRendu::dessine_arretes() const
{
	return m_dessine_arretes;
}

void ContexteRendu::dessine_arretes(bool ouinon)
{
	m_dessine_arretes = ouinon;
}

bool ContexteRendu::dessine_normaux() const
{
	return m_dessine_normaux;
}

void ContexteRendu::dessine_normaux(bool ouinon)
{
	m_dessine_normaux = ouinon;
}

const dls::math::mat4x4f &ContexteRendu::modele_vue() const
{
	return m_modele_vue;
}

void ContexteRendu::modele_vue(const dls::math::mat4x4f &matrice)
{
	m_modele_vue = matrice;
}
