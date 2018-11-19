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

#include "rendu_lumiere.h"

#include <ego/outils.h>
#include <numeric>

#include "bibliotheques/opengl/tampon_rendu.h"

#include "coeur/lumiere.h"

RenduLumiere::RenduLumiere(Lumiere *lumiere)
	: m_lumiere(lumiere)
{}

RenduLumiere::~RenduLumiere()
{
	delete m_tampon;
}

void RenduLumiere::initialise()
{
	if (m_tampon != nullptr) {
		return;
	}

	m_tampon = new TamponRendu;

	m_tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/simple.vert"));

	m_tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				numero7::ego::util::str_from_file("nuanceurs/simple.frag"));

	m_tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("normal");
	parametre_programme.ajoute_uniforme("N");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	m_tampon->parametres_programme(parametre_programme);

	auto programme = m_tampon->programme();
	programme->active();
	programme->uniforme("couleur", 1.0f, 0.0f, 0.0f, 1.0f);
	programme->desactive();

	std::vector<numero7::math::vec3f> sommets;

	if (m_lumiere->type == LUMIERE_POINT) {
		sommets.resize(6);

		sommets[0] = numero7::math::vec3f(-1.0,  0.0,  0.0);
		sommets[1] = numero7::math::vec3f( 1.0,  0.0,  0.0);
		sommets[2] = numero7::math::vec3f( 0.0, -1.0,  0.0);
		sommets[3] = numero7::math::vec3f( 0.0,  1.0,  0.0);
		sommets[4] = numero7::math::vec3f( 0.0,  0.0, -1.0);
		sommets[5] = numero7::math::vec3f( 0.0,  0.0,  1.0);
	}
	else {
		sommets.resize(6);

		sommets[0] = numero7::math::vec3f( 0.0,  0.1,  0.0);
		sommets[1] = numero7::math::vec3f( 0.0,  0.1, -1.0);
		sommets[2] = numero7::math::vec3f( 0.1, -0.1,  0.0);
		sommets[3] = numero7::math::vec3f( 0.1, -0.1, -1.0);
		sommets[4] = numero7::math::vec3f(-0.1, -0.1,  0.0);
		sommets[5] = numero7::math::vec3f(-0.1, -0.1, -1.0);
	}

	std::vector<unsigned int> indices(sommets.size());
	std::iota(indices.begin(), indices.end(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(numero7::math::vec3f);
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres_tampon.elements = indices.size();

	m_tampon->remplie_tampon(parametres_tampon);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	m_tampon->parametres_dessin(parametres_dessin);
}

void RenduLumiere::dessine(const ContexteRendu &contexte)
{
	m_tampon->dessine(contexte);
}

numero7::math::mat4d RenduLumiere::matrice() const
{
	return m_lumiere->transformation.matrice();
}
