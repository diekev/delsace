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

#include "rendu_manipulatrice.h"

#include <ego/outils.h>

#include "bibliotheques/opengl/tampon_rendu.h"

#include "coeur/manipulatrice.h"

/* ************************************************************************** */

enum {
	TAMPON_X,
	TAMPON_Y,
	TAMPON_Z,
	TAMPON_XY,
	TAMPON_XZ,
	TAMPON_YZ,
	TAMPON_XYZ,
	TAMPON_SELECTION,

	NOMBRE_TAMPON,
};

static constexpr auto RAYON_BRANCHE = 0.02f;
static constexpr auto RAYON_POIGNEE = 0.10f;
static constexpr auto TAILLE = 1.0f;

static const dls::math::vec4f couleurs[NOMBRE_TAMPON] = {
	dls::math::vec4f{1.0f, 0.0f, 0.0f, 1.0f}, // TAMPON_X
	dls::math::vec4f{0.0f, 1.0f, 0.0f, 1.0f}, // TAMPON_Y
	dls::math::vec4f{0.0f, 0.0f, 1.0f, 1.0f}, // TAMPON_Z
	dls::math::vec4f{1.0f, 1.0f, 0.0f, 1.0f}, // TAMPON_XY
	dls::math::vec4f{1.0f, 0.0f, 1.0f, 1.0f}, // TAMPON_XZ
	dls::math::vec4f{0.0f, 1.0f, 1.0f, 1.0f}, // TAMPON_YZ
	dls::math::vec4f{0.8f, 0.8f, 0.8f, 1.0f}, // TAMPON_XYZ
	dls::math::vec4f{1.0f, 1.0f, 0.1f, 1.0f}, // TAMPON_SELECTION
};

static TamponRendu *cree_tampon_base(const dls::math::vec4f &couleur)
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

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	return tampon;
}

static void ajoute_baton_axe(
		const dls::math::vec3f &min,
		const dls::math::vec3f &max,
		std::vector<dls::math::vec3f> &sommets,
		std::vector<unsigned int> &indices)
{
	const auto decalage = sommets.size();

	const dls::math::vec3f coins[8] = {
		dls::math::vec3f(min[0], min[1], min[2]),
		dls::math::vec3f(max[0], min[1], min[2]),
		dls::math::vec3f(max[0], max[1], min[2]),
		dls::math::vec3f(min[0], max[1], min[2]),
		dls::math::vec3f(min[0], min[1], max[2]),
		dls::math::vec3f(max[0], min[1], max[2]),
		dls::math::vec3f(max[0], max[1], max[2]),
		dls::math::vec3f(min[0], max[1], max[2])
	};

	sommets.reserve(sommets.size() + 8);

	for (const auto &coin : coins) {
		sommets.push_back(coin);
	}

	const unsigned int polygones[6][4] = {
		{0, 4, 7, 3}, // min x
		{5, 1, 2, 6}, // max x
		{1, 5, 4, 0}, // min y
		{3, 7, 6, 2}, // max y
		{1, 0, 3, 2}, // min z
		{4, 5, 6, 7}, // max z
	};

	for (size_t	 i = 0; i < 6; ++i) {
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][0]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][1]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][2]));

		indices.push_back(static_cast<unsigned>(decalage + polygones[i][0]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][2]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][3]));
	}
}

static void ajoute_poignee_axe(
		const dls::math::vec3f &min,
		const dls::math::vec3f &max,
		std::vector<dls::math::vec3f> &sommets,
		std::vector<unsigned int> &indices)
{
	const auto decalage = sommets.size();

	const dls::math::vec3f coins[8] = {
		dls::math::vec3f(min[0], min[1], min[2]),
		dls::math::vec3f(max[0], min[1], min[2]),
		dls::math::vec3f(max[0], max[1], min[2]),
		dls::math::vec3f(min[0], max[1], min[2]),
		dls::math::vec3f(min[0], min[1], max[2]),
		dls::math::vec3f(max[0], min[1], max[2]),
		dls::math::vec3f(max[0], max[1], max[2]),
		dls::math::vec3f(min[0], max[1], max[2])
	};

	sommets.reserve(sommets.size() + 8);

	for (const auto &coin : coins) {
		sommets.push_back(coin);
	}

	const unsigned int polygones[6][4] = {
		{0, 4, 7, 3}, // min x
		{5, 1, 2, 6}, // max x
		{1, 5, 4, 0}, // min y
		{3, 7, 6, 2}, // max y
		{1, 0, 3, 2}, // min z
		{4, 5, 6, 7}, // max z
	};

	for (size_t	 i = 0; i < 6; ++i) {
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][0]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][1]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][2]));

		indices.push_back(static_cast<unsigned>(decalage + polygones[i][0]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][2]));
		indices.push_back(static_cast<unsigned>(decalage + polygones[i][3]));
	}
}

static TamponRendu *cree_tampon_axe(int axe, float taille)
{
	auto tampon = cree_tampon_base(couleurs[axe]);

	std::vector<dls::math::vec3f> sommets;
	std::vector<unsigned int> indices;

	switch (axe) {
		case TAMPON_X:
		{
			auto min = dls::math::vec3f(RAYON_POIGNEE * 2, -RAYON_BRANCHE, -RAYON_BRANCHE);
			auto max = dls::math::vec3f(taille,  RAYON_BRANCHE,  RAYON_BRANCHE);
			ajoute_baton_axe(min, max, sommets, indices);

			min = dls::math::vec3f(taille - RAYON_POIGNEE, -RAYON_POIGNEE, -RAYON_POIGNEE);
			max = dls::math::vec3f(taille + RAYON_POIGNEE,  RAYON_POIGNEE,  RAYON_POIGNEE);
			ajoute_poignee_axe(min, max, sommets, indices);
			break;
		}
		case TAMPON_Y:
		{
			auto min = dls::math::vec3f(-RAYON_BRANCHE, RAYON_POIGNEE * 2, -RAYON_BRANCHE);
			auto max = dls::math::vec3f( RAYON_BRANCHE, taille,  RAYON_BRANCHE);
			ajoute_baton_axe(min, max, sommets, indices);

			min = dls::math::vec3f(-RAYON_POIGNEE, taille - RAYON_POIGNEE, -RAYON_POIGNEE);
			max = dls::math::vec3f( RAYON_POIGNEE, taille + RAYON_POIGNEE,  RAYON_POIGNEE);
			ajoute_poignee_axe(min, max, sommets, indices);
			break;
		}
		case TAMPON_Z:
		{
			auto min = dls::math::vec3f(-RAYON_BRANCHE, -RAYON_BRANCHE, RAYON_POIGNEE * 2);
			auto max = dls::math::vec3f( RAYON_BRANCHE,  RAYON_BRANCHE, taille);
			ajoute_baton_axe(min, max, sommets, indices);

			min = dls::math::vec3f(-RAYON_POIGNEE, -RAYON_POIGNEE, taille - RAYON_POIGNEE);
			max = dls::math::vec3f( RAYON_POIGNEE,  RAYON_POIGNEE, taille + RAYON_POIGNEE);
			ajoute_poignee_axe(min, max, sommets, indices);
			break;
		}
		case TAMPON_XYZ:
		{
			auto min = dls::math::vec3f(-taille, -taille, -taille);
			auto max = dls::math::vec3f( taille,  taille, taille);

			ajoute_poignee_axe(min, max, sommets, indices);
			break;
		}
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres_tampon.elements = indices.size();

	tampon->remplie_tampon(parametres_tampon);

	return tampon;
}

/* ************************************************************************** */

RenduManipulatricePosition::RenduManipulatricePosition()
{
	m_tampon_axe_x = cree_tampon_axe(TAMPON_X, TAILLE);
	m_tampon_axe_y = cree_tampon_axe(TAMPON_Y, TAILLE);
	m_tampon_axe_z = cree_tampon_axe(TAMPON_Z, TAILLE);
	m_tampon_poignee_xyz = cree_tampon_axe(TAMPON_XYZ, RAYON_POIGNEE);
}

RenduManipulatricePosition::~RenduManipulatricePosition()
{
	delete m_tampon_axe_x;
	delete m_tampon_axe_y;
	delete m_tampon_axe_z;
	delete m_tampon_poignee_xyz;
}

void RenduManipulatricePosition::manipulatrice(Manipulatrice3D *pointeur)
{
	m_pointeur = pointeur;

	if (m_pointeur == nullptr) {
		return;
	}

	auto etat = m_pointeur->etat();
	dls::math::vec4f couleur;

	if (etat == ETAT_INTERSECTION_X || etat == ETAT_SELECTION_X || etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_X];
	}

	auto programme = m_tampon_axe_x->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_Y || etat == ETAT_SELECTION_Y || etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_Y];
	}

	programme = m_tampon_axe_y->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_Z || etat == ETAT_SELECTION_Z || etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_Z];
	}

	programme = m_tampon_axe_z->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_XYZ];
	}

	programme = m_tampon_poignee_xyz->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();
}

void RenduManipulatricePosition::dessine(const ContexteRendu &contexte)
{
	if (m_pointeur == nullptr) {
		return;
	}

	m_tampon_poignee_xyz->dessine(contexte);
	m_tampon_axe_x->dessine(contexte);
	m_tampon_axe_y->dessine(contexte);
	m_tampon_axe_z->dessine(contexte);
}

/* ************************************************************************** */

RenduManipulatriceEchelle::RenduManipulatriceEchelle()
{
	m_tampon_axe_x = cree_tampon_axe(TAMPON_X, TAILLE);
	m_tampon_axe_y = cree_tampon_axe(TAMPON_Y, TAILLE);
	m_tampon_axe_z = cree_tampon_axe(TAMPON_Z, TAILLE);
	m_tampon_poignee_xyz = cree_tampon_axe(TAMPON_XYZ, RAYON_POIGNEE);
}

RenduManipulatriceEchelle::~RenduManipulatriceEchelle()
{
	delete m_tampon_axe_x;
	delete m_tampon_axe_y;
	delete m_tampon_axe_z;
	delete m_tampon_poignee_xyz;
}

void RenduManipulatriceEchelle::manipulatrice(Manipulatrice3D *pointeur)
{
	m_pointeur = pointeur;

	if (m_pointeur == nullptr) {
		return;
	}

	/* Ajourne la taille de la manipulatrice. */
//	if (m_tampon_axe_x) {
//		delete m_tampon_axe_x;
//		delete m_tampon_axe_y;
//		delete m_tampon_axe_z;
//		m_tampon_axe_x = cree_tampon_axe(0, pointeur->taille().x);
//		m_tampon_axe_y = cree_tampon_axe(1, pointeur->taille().y);
//		m_tampon_axe_z = cree_tampon_axe(2, pointeur->taille().z);
//	}

	auto etat = m_pointeur->etat();
	dls::math::vec4f couleur;

	if (etat == ETAT_INTERSECTION_X || etat == ETAT_SELECTION_X || etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_X];
	}

	auto programme = m_tampon_axe_x->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_Y || etat == ETAT_SELECTION_Y || etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_Y];
	}

	programme = m_tampon_axe_y->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_Z || etat == ETAT_SELECTION_Z || etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_Z];
	}

	programme = m_tampon_axe_z->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_XYZ) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_XYZ];
	}

	programme = m_tampon_poignee_xyz->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();
}

void RenduManipulatriceEchelle::dessine(const ContexteRendu &contexte)
{
	if (m_pointeur == nullptr) {
		return;
	}

	m_tampon_poignee_xyz->dessine(contexte);
	m_tampon_axe_x->dessine(contexte);
	m_tampon_axe_y->dessine(contexte);
	m_tampon_axe_z->dessine(contexte);
}

/* ************************************************************************** */

static TamponRendu *cree_tampon_cercle_axe(int axe)
{
	auto tampon = cree_tampon_base(couleurs[axe]);

	const auto segments = 64;
	std::vector<dls::math::vec3f> sommets;
	std::vector<unsigned int> indices;

	sommets.reserve(segments + 1);
	indices.reserve(segments * 2 + 1);

	switch (axe) {
		case 0:
		{
			const auto phid = 2.0f * static_cast<float>(M_PI) / segments;
			auto phi = 0.0f;

			dls::math::vec3f point;

			for (int a = 0; a < segments; ++a, phi += phid) {
				point[0] = 0.0f;
				point[1] = std::sin(phi);
				point[2] = std::cos(phi);

				sommets.push_back(point);
			}
			break;
		}
		case 1:
		{
			const auto phid = 2.0f * static_cast<float>(M_PI) / segments;
			auto phi = 0.0f;

			dls::math::vec3f point;

			for (int a = 0; a < segments; ++a, phi += phid) {
				point[0] = std::sin(phi);
				point[1] = 0.0f;
				point[2] = std::cos(phi);

				sommets.push_back(point);
			}
			break;
		}
		case 2:
		{
			const auto phid = 2.0f * static_cast<float>(M_PI) / segments;
			auto phi = 0.0f;

			dls::math::vec3f point;

			for (int a = 0; a < segments; ++a, phi += phid) {
				point[0] = std::sin(phi);
				point[1] = std::cos(phi);
				point[2] = 0.0f;

				sommets.push_back(point);
			}
			break;
		}
	}

	for (auto i = 0u; i < segments; ++i) {
		indices.push_back(i);
		indices.push_back((i + 1) % segments);
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres_tampon.elements = indices.size();

	tampon->remplie_tampon(parametres_tampon);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(2);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

RenduManipulatriceRotation::RenduManipulatriceRotation()
{
	m_tampon_axe_x = cree_tampon_cercle_axe(TAMPON_X);
	m_tampon_axe_y = cree_tampon_cercle_axe(TAMPON_Y);
	m_tampon_axe_z = cree_tampon_cercle_axe(TAMPON_Z);
}

RenduManipulatriceRotation::~RenduManipulatriceRotation()
{
	delete m_tampon_axe_x;
	delete m_tampon_axe_y;
	delete m_tampon_axe_z;
}

void RenduManipulatriceRotation::manipulatrice(Manipulatrice3D *pointeur)
{
	m_pointeur = pointeur;

	if (m_pointeur == nullptr) {
		return;
	}

	auto etat = m_pointeur->etat();

	dls::math::vec4f couleur;

	if (etat == ETAT_INTERSECTION_X || etat == ETAT_SELECTION_X) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_X];
	}

	auto programme = m_tampon_axe_x->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_Y || etat == ETAT_SELECTION_Y) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_Y];
	}

	programme = m_tampon_axe_y->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();

	if (etat == ETAT_INTERSECTION_Z || etat == ETAT_SELECTION_Z) {
		couleur = couleurs[TAMPON_SELECTION];
	}
	else {
		couleur = couleurs[TAMPON_Z];
	}

	programme = m_tampon_axe_z->programme();
	programme->active();
	programme->uniforme("couleur", couleur.r, couleur.g, couleur.b, couleur.a);
	programme->desactive();
}

void RenduManipulatriceRotation::dessine(const ContexteRendu &contexte)
{
	if (m_pointeur == nullptr) {
		return;
	}

	m_tampon_axe_x->dessine(contexte);
	m_tampon_axe_y->dessine(contexte);
	m_tampon_axe_z->dessine(contexte);
}
