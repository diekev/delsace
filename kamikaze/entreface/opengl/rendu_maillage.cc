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

#include "rendu_maillage.h"

#include <ego/utils.h>
#include <numeric>

#include "../bibliotheques/opengl/tampon_rendu.h"

#include "coeur/sdk/outils/géométrie.h"
#include "coeur/sdk/mesh.h"

/* ************************************************************************** */

static TamponRendu *cree_tampon_surface()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::VERTEX_SHADER,
				numero7::ego::util::str_from_file("nuanceurs/object.vert"));

	tampon->charge_source_programme(
				numero7::ego::FRAGMENT_SHADER,
				numero7::ego::util::str_from_file("nuanceurs/object.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametres;
	parametres.ajoute_attribut("vertex");
	parametres.ajoute_attribut("normal");
	parametres.ajoute_attribut("vertex_color");
	parametres.ajoute_uniforme("matrice");
	parametres.ajoute_uniforme("MVP");
	parametres.ajoute_uniforme("N");
	parametres.ajoute_uniforme("pour_surlignage");
	parametres.ajoute_uniforme("color");
	parametres.ajoute_uniforme("has_vcolors");

	auto program = tampon->programme();
	program->uniform("color", 1.0f, 1.0f, 1.0f);

	tampon->parametres_programme(parametres);

	return tampon;
}

/* ************************************************************************** */

RenduMaillage::RenduMaillage(Mesh *maillage)
	: m_maillage(maillage)
	, m_tampon_surface(nullptr)
{}

RenduMaillage::~RenduMaillage()
{
	delete m_tampon_surface;
}

void RenduMaillage::initialise()
{
	if (m_tampon_surface != nullptr) {
		return;
	}

	m_tampon_surface = cree_tampon_surface();

	auto points = m_maillage->points();
	auto polys = m_maillage->polys();

	std::vector<unsigned int> indices;
	indices.reserve(polys->size());

	for (auto i = 0ul, ie = polys->size(); i < ie; ++i) {
		const auto &quad = (*polys)[i];

		indices.push_back(quad[0]);
		indices.push_back(quad[1]);
		indices.push_back(quad[2]);

		if (quad[3] != INVALID_INDEX) {
			indices.push_back(quad[0]);
			indices.push_back(quad[2]);
			indices.push_back(quad[3]);
		}
	}

	m_tampon_surface->peut_surligner(true);

	ParametresTampon parametres;
	parametres.attribut = "vertex";
	parametres.dimension_attribut = 3;
	parametres.pointeur_sommets = const_cast<void *>(points->data());
	parametres.taille_octet_sommets = points->byte_size();
	parametres.pointeur_index = indices.data();
	parametres.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres.elements = indices.size();

	m_tampon_surface->remplie_tampon(parametres);

	auto normals = m_maillage->attribute("normal", ATTR_TYPE_VEC3);

	if (normals != nullptr) {
		if (normals->size() != points->size()) {
			normals->resize(points->size());

			calcule_normales(*points, *polys, *normals, false);
		}

		parametres.attribut = "normal";
		parametres.dimension_attribut = 3;
		parametres.pointeur_donnees_extra = const_cast<void *>(normals->data());
		parametres.taille_octet_donnees_extra = normals->byte_size();

		m_tampon_surface->remplie_tampon_extra(parametres);
	}

	auto colors = m_maillage->attribute("color", ATTR_TYPE_VEC3);

	if (colors != nullptr) {
		parametres.attribut = "vertex_color";
		parametres.dimension_attribut = 3;
		parametres.pointeur_donnees_extra = const_cast<void *>(colors->data());
		parametres.taille_octet_donnees_extra = colors->byte_size();

		m_tampon_surface->remplie_tampon_extra(parametres);
	}
}

void RenduMaillage::dessine(const ContexteRendu &contexte)
{
	/* Dessine sommets. */
	{
		ParametresDessin parametres_dessin;
		parametres_dessin.type_dessin(GL_POINTS);
		parametres_dessin.taille_point(2.0f);

		m_tampon_surface->parametres_dessin(parametres_dessin);

		auto programme = m_tampon_surface->programme();
		programme->enable();
		programme->uniform("color", 0.0f, 0.0f, 0.0f);
		programme->disable();

		m_tampon_surface->dessine(contexte);
	}

	/* Dessine surface. */
	{
		ParametresDessin parametres_dessin;
		parametres_dessin.type_dessin(GL_TRIANGLES);

		m_tampon_surface->parametres_dessin(parametres_dessin);

		auto programme = m_tampon_surface->programme();
		programme->enable();
		programme->uniform("color", 1.0f, 1.0f, 1.0f);
		programme->disable();

		m_tampon_surface->dessine(contexte);
	}
}
