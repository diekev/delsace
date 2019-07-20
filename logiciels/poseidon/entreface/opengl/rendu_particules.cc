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

#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/fluide.h"

/* ************************************************************************** */

static TamponRendu *cree_tampon()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::contenu_fichier("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::contenu_fichier("nuanceurs/simple.frag"));

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

	dls::tableau<dls::math::vec3f> points;
	points.reserve(m_fluide->particules.taille());

	for (auto const &particule : m_fluide->particules) {
		points.pousse(particule.pos);
	}

	ParametresTampon parametres;
	parametres.attribut = "sommets";
	parametres.dimension_attribut = 3;
	parametres.pointeur_sommets = points.donnees();
	parametres.elements = static_cast<size_t>(points.taille());
	parametres.taille_octet_sommets = static_cast<size_t>(points.taille()) * sizeof(dls::math::vec3f);

	m_tampon->remplie_tampon(parametres);
}

void RenduParticules::dessine(ContexteRendu const &contexte)
{
	m_tampon->dessine(contexte);
}
