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

#include "rendu_grille.h"

#include <algorithm>
#include <ego/utils.h>
#include <GL/glew.h>
#include <numeric>

#include "bibliotheques/opengl/tampon_rendu.h"

static TamponRendu *cree_tampon(const glm::vec4 &couleur, float taille_ligne)
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
	parametres_dessin.taille_ligne(taille_ligne);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->enable();
	programme->uniform("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->disable();

	return tampon;
}

RenduGrille::RenduGrille(int largeur, int hauteur)
	: m_tampon_grille(nullptr)
	, m_tampon_axe_x(nullptr)
	, m_tampon_axe_z(nullptr)
{
	/* Création tampon grille. */

	m_tampon_grille = cree_tampon(glm::vec4(1.0f), 1.0f);

	const auto nombre_sommets = ((largeur + 1) + (hauteur + 1)) * 2;
	std::vector<glm::vec3> sommets(nombre_sommets);

	const auto moitie_largeur = (largeur / 2);
	const auto moitie_hauteur = (hauteur / 2);
	auto compte = 0;

	for (int i = -moitie_hauteur; i <= moitie_hauteur; ++i) {
		sommets[compte++] = glm::vec3(i, 0.0f, -moitie_hauteur);
		sommets[compte++] = glm::vec3(i, 0.0f,  moitie_hauteur);
		sommets[compte++] = glm::vec3(-moitie_largeur, 0.0f, i);
		sommets[compte++] = glm::vec3( moitie_largeur, 0.0f, i);
	}

	std::vector<unsigned int> index(largeur * hauteur);
	std::iota(index.begin(), index.end(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(glm::vec3);
	parametres_tampon.pointeur_index = index.data();
	parametres_tampon.taille_octet_index = index.size() * sizeof(unsigned int);
	parametres_tampon.elements = index.size();

	m_tampon_grille->remplie_tampon(parametres_tampon);

	/* Création tampons lignes. */

	std::vector<unsigned int> index_ligne(2);
	std::iota(index_ligne.begin(), index_ligne.end(), 0);

	/* Création tampon ligne X. */

	m_tampon_axe_x = cree_tampon(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);

	std::vector<glm::vec3> sommets_x(2);
	sommets_x[0] = glm::vec3(-moitie_largeur, 0.0f, 0.0f);
	sommets_x[1] = glm::vec3( moitie_largeur, 0.0f, 0.0f);

	parametres_tampon.pointeur_sommets = sommets_x.data();
	parametres_tampon.taille_octet_sommets = sommets_x.size() * sizeof(glm::vec3);
	parametres_tampon.pointeur_index = index_ligne.data();
	parametres_tampon.taille_octet_index = index_ligne.size() * sizeof(unsigned int);
	parametres_tampon.elements = index_ligne.size();

	m_tampon_axe_x->remplie_tampon(parametres_tampon);

	/* Création tampon ligne Z. */

	m_tampon_axe_z = cree_tampon(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 2.0f);

	std::vector<glm::vec3> sommets_z(2);
	sommets_z[0] = glm::vec3(0.0f, 0.0f, -moitie_hauteur);
	sommets_z[1] = glm::vec3(0.0f, 0.0f,  moitie_hauteur);

	parametres_tampon.pointeur_sommets = sommets_z.data();
	parametres_tampon.taille_octet_sommets = sommets_z.size() * sizeof(glm::vec3);

	m_tampon_axe_z->remplie_tampon(parametres_tampon);
}

RenduGrille::~RenduGrille()
{
	delete m_tampon_grille;
	delete m_tampon_axe_x;
	delete m_tampon_axe_z;
}

void RenduGrille::dessine(const ContexteRendu &contexte)
{
	m_tampon_grille->dessine(contexte);
	m_tampon_axe_x->dessine(contexte);
	m_tampon_axe_z->dessine(contexte);
}
