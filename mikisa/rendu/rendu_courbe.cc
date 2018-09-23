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

#include "rendu_courbe.h"

#include <ego/utils.h>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"

#include "coeur/corps/courbes.h"

/* ************************************************************************** */

static void construit_tampon_pour_courbe();

static int nombre_de_courbe();

static TamponRendu *cree_tampon_courbes()
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

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->enable();
	programme->uniform("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->disable();

	return tampon;
}

static TamponRendu *genere_tampon_courbes(Courbes *courbes)
{
	auto tampon = cree_tampon_courbes();

	const size_t nombre_elements = courbes->nombre_courbes * (courbes->segments_par_courbe * 2);

	std::vector<numero7::math::vec3f> sommets;
	sommets.reserve(courbes->liste_points()->taille());

	std::vector<int> index;
	index.reserve(nombre_elements);

	for (Sommet *sommet : courbes->liste_points()->sommets()) {
		sommets.push_back(sommet->pos);
	}

	for (Arrete *arrete : courbes->liste_segments()->arretes()) {
		index.push_back(arrete->s[0]->index);
		index.push_back(arrete->s[1]->index);
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(numero7::math::vec3f);
	parametres_tampon.pointeur_index = index.data();
	parametres_tampon.taille_octet_index = index.size() * sizeof(int);
	parametres_tampon.elements = index.size();

	tampon->remplie_tampon(parametres_tampon);

	return tampon;
}

/* ************************************************************************** */

RenduCourbe::RenduCourbe(Courbes *courbes)
	: m_courbes(courbes)
{}

RenduCourbe::~RenduCourbe()
{
	delete m_tampon;
}

void RenduCourbe::initialise()
{
	m_tampon = genere_tampon_courbes(m_courbes);
}

void RenduCourbe::dessine(const ContexteRendu &contexte)
{
	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_POINTS);
	parametres_dessin.taille_point(2.0);

	m_tampon->parametres_dessin(parametres_dessin);
	m_tampon->dessine(contexte);

	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	m_tampon->parametres_dessin(parametres_dessin);
	m_tampon->dessine(contexte);
}
