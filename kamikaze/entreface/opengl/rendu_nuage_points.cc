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

#include "rendu_nuage_points.h"

#include <ego/outils.h>
#include <numeric>

#include "../bibliotheques/opengl/tampon_rendu.h"

#include "coeur/sdk/prim_points.h"

/* ************************************************************************** */

static TamponRendu *cree_tampon_points()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/flat_shader.vert"));

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				numero7::ego::util::str_from_file("nuanceurs/flat_shader.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametres_programme;
	parametres_programme.ajoute_attribut("vertex");
	parametres_programme.ajoute_attribut("vertex_color");
	parametres_programme.ajoute_uniforme("matrice");
	parametres_programme.ajoute_uniforme("MVP");
	parametres_programme.ajoute_uniforme("for_outline");
	parametres_programme.ajoute_uniforme("color");
	parametres_programme.ajoute_uniforme("has_vcolors");

	tampon->parametres_programme(parametres_programme);

	auto programme = tampon->programme();
	programme->uniforme("color", 0.0f, 0.0f, 0.0f, 1.0f);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_POINTS);
	parametres_dessin.taille_point(2.0f);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

/* ************************************************************************** */

RenduNuagePoints::RenduNuagePoints(PrimPoints *nuage_points)
	: m_nuage_points(nuage_points)
	, m_tampon_points(nullptr)
{}

RenduNuagePoints::~RenduNuagePoints()
{
	delete m_tampon_points;
}

void RenduNuagePoints::initialise()
{
	m_tampon_points = cree_tampon_points();

	auto points = m_nuage_points->points();

	ParametresTampon parametres;
	parametres.attribut = "vertex";
	parametres.dimension_attribut = 3;
	parametres.pointeur_sommets = const_cast<void *>(points->data());
	parametres.taille_octet_sommets = points->byte_size();
	parametres.pointeur_index = nullptr;
	parametres.taille_octet_index = 0ul;
	parametres.elements = points->size();

	m_tampon_points->remplie_tampon(parametres);

	auto colors = m_nuage_points->attribute("color", ATTR_TYPE_VEC3);

	if (colors != nullptr) {
		parametres.attribut = "vertex_color";
		parametres.dimension_attribut = 3;
		parametres.pointeur_donnees_extra = const_cast<void *>(colors->data());
		parametres.taille_octet_donnees_extra = colors->byte_size();

		m_tampon_points->remplie_tampon_extra(parametres);
	}
}

void RenduNuagePoints::dessine(const ContexteRendu &contexte)
{
	m_tampon_points->dessine(contexte);
}
