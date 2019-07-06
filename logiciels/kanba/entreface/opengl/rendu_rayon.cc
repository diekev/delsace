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

#include "rendu_rayon.h"

#include <numeric>
#include <numero7/math/conversion_point_vecteur.h>

#include "coeur/kyanba.h"

#include "tampon_rendu.h"

static const char *source_vertex =
		"#version 330 core\n"
		"layout(location = 0) in vec3 sommets;\n"
		"uniform mat4 MVP;"
		"void main()\n"
		"{\n"
		"	gl_Position = MVP * vec4(sommets, 1.0);\n"
		"}\n";

static const char *source_fragment =
		"#version 330 core\n"
		"layout (location = 0) out vec4 couleur_fragment;\n"
		" void main()\n"
		"{\n"
		"	couleur_fragment = vec4(1.0);\n"
		"}\n";

static TamponRendu *cree_tampon()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::VERTEX_SHADER,
				source_vertex);

	tampon->charge_source_programme(
				numero7::ego::FRAGMENT_SHADER,
				source_fragment);

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("matrice");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametre_dessin;
	parametre_dessin.type_dessin(GL_LINES);

	tampon->parametres_dessin(parametre_dessin);

	return tampon;
}

RenduRayon::RenduRayon(Kyanba *kyanba)
	: m_kyanba(kyanba)
	, m_tampon(nullptr)
	, m_nombre_rayons(-1)
{}

RenduRayon::~RenduRayon()
{
	delete m_tampon;
}

void RenduRayon::ajourne()
{
	if (!m_tampon) {
		m_tampon = cree_tampon();
	}

#ifdef DEBOGAGE_RAYON
	if (m_nombre_rayons == m_kyanba->rayons.size()) {
		return;
	}

	m_nombre_rayons = m_kyanba->rayons.size();

	std::vector<numero7::math::vec3f> sommets;
	sommets.reserve(m_nombre_rayons * 2);

	for (const auto &rayon : m_kyanba->rayons) {
		const auto &origine = numero7::math::vecteur_depuis_point(rayon.origine);

		sommets.push_back(origine);
		sommets.push_back(origine + float(rayon.distance_max) * rayon.direction);
	}

	std::vector<unsigned int> index(sommets.size());
	std::iota(index.begin(), index.end(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.elements = index.size();
	parametres_tampon.pointeur_index = index.data();
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(numero7::math::vec3f);
	parametres_tampon.taille_octet_index = index.size() * sizeof(unsigned int);
	parametres_tampon.dimension_attribut = 3;

	m_tampon->remplie_tampon(parametres_tampon);
#endif
}

void RenduRayon::dessine(const ContexteRendu &contexte)
{
	ajourne();

	m_tampon->dessine(contexte);
}
