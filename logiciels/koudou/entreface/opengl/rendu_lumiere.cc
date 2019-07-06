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

#include "biblinternes/ego/outils.h"
#include <numeric>

#include "biblinternes/opengl/tampon_rendu.h"

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
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/simple.vert"));

	m_tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/simple.frag"));

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

	std::vector<dls::math::vec3f> sommets;

	if (m_lumiere->type == type_lumiere::POINT) {
		sommets.resize(6);

		sommets[0] = dls::math::vec3f(-1.0f,  0.0f,  0.0f);
		sommets[1] = dls::math::vec3f( 1.0f,  0.0f,  0.0f);
		sommets[2] = dls::math::vec3f( 0.0f, -1.0f,  0.0f);
		sommets[3] = dls::math::vec3f( 0.0f,  1.0f,  0.0f);
		sommets[4] = dls::math::vec3f( 0.0f,  0.0f, -1.0f);
		sommets[5] = dls::math::vec3f( 0.0f,  0.0f,  1.0f);
	}
	else {
		sommets.resize(6);

		sommets[0] = dls::math::vec3f( 0.0f,  0.1f,  0.0f);
		sommets[1] = dls::math::vec3f( 0.0f,  0.1f, -1.0f);
		sommets[2] = dls::math::vec3f( 0.1f, -0.1f,  0.0f);
		sommets[3] = dls::math::vec3f( 0.1f, -0.1f, -1.0f);
		sommets[4] = dls::math::vec3f(-0.1f, -0.1f,  0.0f);
		sommets[5] = dls::math::vec3f(-0.1f, -0.1f, -1.0f);
	}

	std::vector<unsigned int> indices(sommets.size());
	std::iota(indices.begin(), indices.end(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres_tampon.elements = indices.size();

	m_tampon->remplie_tampon(parametres_tampon);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	m_tampon->parametres_dessin(parametres_dessin);
}

void RenduLumiere::dessine(ContexteRendu const &contexte)
{
	m_tampon->dessine(contexte);
}

dls::math::mat4x4d RenduLumiere::matrice() const
{
	return m_lumiere->transformation.matrice();
}
