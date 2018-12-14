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

#include "rendu_particules.h"

#include <ego/outils.h>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"

#include "coeur/fluide.h"

/* ************************************************************************** */

static TamponRendu *cree_tampon()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				numero7::ego::util::str_from_file("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_POINTS);
	parametres_dessin.taille_point(2.0f);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.1f, 0.2f, 0.7f, 1.0f);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

RenduParticules::RenduParticules(Fluide *fluide)
	: m_fluide(fluide)
{
}

RenduParticules::~RenduParticules()
{
	delete m_tampon;
}

void RenduParticules::initialise()
{
	m_tampon = cree_tampon();

	std::vector<dls::math::vec3f> points;
	points.reserve(m_fluide->particules.size());

	for (auto const &particule : m_fluide->particules) {
		points.push_back(particule.pos);
	}

	ParametresTampon parametres;
	parametres.attribut = "sommets";
	parametres.dimension_attribut = 3;
	parametres.pointeur_sommets = points.data();
	parametres.elements = points.size();
	parametres.taille_octet_sommets = points.size() * sizeof(dls::math::vec3f);

	m_tampon->remplie_tampon(parametres);
}

void RenduParticules::dessine(const ContexteRendu &contexte)
{
	m_tampon->dessine(contexte);
}
