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

dls::math::mat4x4f const &ContexteRendu::projection() const
{
	return m_projection;
}

void ContexteRendu::projection(dls::math::mat4x4f const &matrice)
{
	m_projection = matrice;
}

dls::math::vec3f const &ContexteRendu::vue() const
{
	return m_vue;
}

void ContexteRendu::vue(dls::math::vec3f const &matrice)
{
	m_vue = matrice;
}

dls::math::mat3x3f const &ContexteRendu::normal() const
{
	return m_normal;
}

void ContexteRendu::normal(dls::math::mat3x3f const &matrice)
{
	m_normal = matrice;
}

dls::math::mat4x4f const &ContexteRendu::MVP() const
{
	return m_modele_vue_projection;
}

void ContexteRendu::MVP(dls::math::mat4x4f const &matrice)
{
	m_modele_vue_projection = matrice;
}

dls::math::mat4x4f const &ContexteRendu::matrice_objet() const
{
	return m_matrice_objet;
}

void ContexteRendu::matrice_objet(dls::math::mat4x4f const &matrice)
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

dls::math::mat4x4f const &ContexteRendu::modele_vue() const
{
	return m_modele_vue;
}

void ContexteRendu::modele_vue(dls::math::mat4x4f const &matrice)
{
	m_modele_vue = matrice;
}
