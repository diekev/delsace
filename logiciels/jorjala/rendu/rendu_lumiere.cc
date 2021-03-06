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

#include <numeric>

#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/objet.h"

RenduLumiere::RenduLumiere(Lumiere const *lumiere)
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
				dls::contenu_fichier("nuanceurs/simple.vert"));

	m_tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::contenu_fichier("nuanceurs/simple.frag"));

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
	programme->uniforme("couleur", m_lumiere->spectre.r, m_lumiere->spectre.v, m_lumiere->spectre.b, m_lumiere->spectre.a);
	programme->desactive();

	dls::tableau<dls::math::vec3f> sommets;

	if (m_lumiere->type == LUMIERE_POINT) {
		sommets.redimensionne(6);

		sommets[0] = dls::math::vec3f(-1.0f,  0.0f,  0.0f);
		sommets[1] = dls::math::vec3f( 1.0f,  0.0f,  0.0f);
		sommets[2] = dls::math::vec3f( 0.0f, -1.0f,  0.0f);
		sommets[3] = dls::math::vec3f( 0.0f,  1.0f,  0.0f);
		sommets[4] = dls::math::vec3f( 0.0f,  0.0f, -1.0f);
		sommets[5] = dls::math::vec3f( 0.0f,  0.0f,  1.0f);
	}
	else {
		sommets.redimensionne(6);

		sommets[0] = dls::math::vec3f( 0.0f,  0.1f,  0.0f);
		sommets[1] = dls::math::vec3f( 0.0f,  0.1f, -1.0f);
		sommets[2] = dls::math::vec3f( 0.1f, -0.1f,  0.0f);
		sommets[3] = dls::math::vec3f( 0.1f, -0.1f, -1.0f);
		sommets[4] = dls::math::vec3f(-0.1f, -0.1f,  0.0f);
		sommets[5] = dls::math::vec3f(-0.1f, -0.1f, -1.0f);
	}

	dls::tableau<unsigned int> indices(sommets.taille());
	std::iota(indices.debut(), indices.fin(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.donnees();
	parametres_tampon.taille_octet_sommets = static_cast<size_t>(sommets.taille()) * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.donnees();
	parametres_tampon.taille_octet_index = static_cast<size_t>(indices.taille()) * sizeof(unsigned int);
	parametres_tampon.elements = static_cast<size_t>(indices.taille());

	m_tampon->remplie_tampon(parametres_tampon);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	m_tampon->parametres_dessin(parametres_dessin);
}

void RenduLumiere::dessine(ContexteRendu const &contexte)
{
	m_tampon->dessine(contexte);
}
