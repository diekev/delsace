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
#include <numero7/math/vec2.h>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"
#include "bibliotheques/texture/texture.h"

#include "coeur/maillage.h"

/* ************************************************************************** */

TamponRendu *cree_tampon_arrete()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::VERTEX_SHADER,
				numero7::ego::util::str_from_file("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				numero7::ego::FRAGMENT_SHADER,
				numero7::ego::util::str_from_file("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->enable();
	programme->uniform("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->disable();

	return tampon;
}

TamponRendu *genere_tampon_arrete(Maillage *maillage)
{
	const auto nombre_arretes = maillage->nombre_arretes();
	const auto nombre_elements = nombre_arretes * 2;
	auto tampon = cree_tampon_arrete();

	std::vector<numero7::math::vec3f> sommets;
	sommets.reserve(nombre_elements);

	/* OpenGL ne travaille qu'avec des floats. */
	for (size_t	i = 0; i < nombre_arretes; ++i) {
		const auto arrete = maillage->arrete(i);

		sommets.push_back(arrete->s[0]->pos);
		sommets.push_back(arrete->s[1]->pos);
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

	tampon->remplie_tampon(parametres_tampon);

	numero7::ego::util::GPU_check_errors("Erreur lors de la création du tampon de sommets");

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

/* ************************************************************************** */

RenduMaillage::RenduMaillage(Maillage *maillage)
	: m_maillage(maillage)
{}

RenduMaillage::~RenduMaillage()
{
	delete m_tampon_arrete;
}

void RenduMaillage::initialise()
{
	m_tampon_arrete = genere_tampon_arrete(m_maillage);
}

void RenduMaillage::dessine(const ContexteRendu &contexte)
{
	m_tampon_arrete->dessine(contexte);
}
