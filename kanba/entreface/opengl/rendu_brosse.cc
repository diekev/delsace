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

#include "rendu_brosse.h"

#include <numero7/ego/outils.h>
#include <delsace/math/vecteur.hh>

#include "bibliotheques/opengl/tampon_rendu.h"
#include "bibliotheques/outils/constantes.h"

/* ************************************************************************** */

static const char *source_vertex =
		"#version 330 core\n"
		"layout(location = 0) in vec3 sommets;\n"
		"uniform float taille_x;\n"
		"uniform float taille_y;\n"
		"uniform float pos_x;\n"
		"uniform float pos_y;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(sommets.x * taille_x + pos_x, sommets.y * taille_y + pos_y, -1, 1.0);\n"
		"}\n";

static const char *source_fragment =
		"#version 330 core\n"
		"layout (location = 0) out vec4 couleur_fragment;\n"
		"uniform vec4 couleur;\n"
		" void main()\n"
		"{\n"
		"	couleur_fragment = couleur;\n"
		"}\n";

static TamponRendu *creer_tampon()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				source_vertex);

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				source_fragment);

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("couleur");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("taille_x");
	parametre_programme.ajoute_uniforme("taille_y");
	parametre_programme.ajoute_uniforme("pos_x");
	parametre_programme.ajoute_uniforme("pos_y");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.8f, 0.1f, 0.1f, 1.0f);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

RenduBrosse::~RenduBrosse()
{
	delete m_tampon_contour;
}

void RenduBrosse::initialise()
{
	if (m_tampon_contour != nullptr) {
		return;
	}

	m_tampon_contour = creer_tampon();

	auto const &points = 64ul;

	std::vector<dls::math::vec3f> sommets(points + 1);

	for (auto i = 0ul; i <= points; i++){
		auto const angle = static_cast<float>(TAU) * static_cast<float>(i) / static_cast<float>(points);
		auto const x = std::cos(angle);
		auto const y = std::sin(angle);
		sommets[i] = dls::math::vec3f(x, y, 0.0f);
	}

	std::vector<unsigned int> index;
	index.reserve(points * 2);

	for (unsigned i = 0; i < 64; ++i) {
		index.push_back(i);
		index.push_back(i + 1);
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = index.data();
	parametres_tampon.taille_octet_index = index.size() * sizeof(unsigned int);
	parametres_tampon.elements = index.size();

	m_tampon_contour->remplie_tampon(parametres_tampon);
}

void RenduBrosse::dessine(
		const ContexteRendu &contexte,
		const float taille_x,
		const float taille_y,
		const float pos_x,
		const float pos_y)
{
	auto programme = m_tampon_contour->programme();
	programme->active();
	programme->uniforme("taille_x", taille_x);
	programme->uniforme("taille_y", taille_y);
	programme->uniforme("pos_x", pos_x);
	programme->uniforme("pos_y", pos_y);
	programme->desactive();

	m_tampon_contour->dessine(contexte);
}
