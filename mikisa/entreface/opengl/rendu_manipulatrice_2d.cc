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

#include "rendu_manipulatrice_2d.h"

#include "bibliotheques/opengl/tampon_rendu.h"

static const char *source_vertex_bordure =
		"#version 330 core\n"
		"layout(location = 0) in vec2 sommets;\n"
		"uniform mat4 MVP;\n"
		"uniform mat4 matrice;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = MVP * matrice * vec4(sommets, 0.0, 1.0);\n"
		"}\n";

static const char *source_fragment_bordure =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		" void main()\n"
		"{\n"
		"	fragment_color = vec4(1.0);\n"
		"}\n";

RenduManipulatrice2D::RenduManipulatrice2D()
	: m_tampon(new TamponRendu())
{
	m_tampon->charge_source_programme(numero7::ego::Nuanceur::VERTEX, source_vertex_bordure);
	m_tampon->charge_source_programme(numero7::ego::Nuanceur::FRAGMENT, source_fragment_bordure);
	m_tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");

	m_tampon->parametres_programme(parametre_programme);

	float sommets[8] = {
		-0.1f, 0.0f,
		0.1f, 0.0f,
		0.0f, -0.1f,
		0.0f, 0.1f
	};

	unsigned int index[8] = { 0, 1, 2, 3 };

	auto parametres_tampon = ParametresTampon();
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 2;
	parametres_tampon.pointeur_sommets = sommets;
	parametres_tampon.taille_octet_sommets = sizeof(float) * 8;
	parametres_tampon.pointeur_index = index;
	parametres_tampon.taille_octet_index = sizeof(unsigned int) * 4;
	parametres_tampon.elements = 4;

	m_tampon->remplie_tampon(parametres_tampon);

	auto parametres_dessin = ParametresDessin();
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	m_tampon->parametres_dessin(parametres_dessin);
}

RenduManipulatrice2D::~RenduManipulatrice2D()
{
	delete m_tampon;
}

void RenduManipulatrice2D::dessine(ContexteRendu const &contexte)
{
	m_tampon->dessine(contexte);
}
