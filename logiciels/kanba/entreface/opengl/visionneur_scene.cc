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

#include "visionneur_scene.h"

#include <GL/glew.h>
#include <sstream>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/opengl/rendu_grille.h"
#include "biblinternes/opengl/rendu_texte.h"
#include "biblinternes/vision/camera.h"

#include "coeur/brosse.h"
#include "coeur/kanba.h"

#include "rendu_brosse.h"
#include "rendu_maillage.h"
#include "rendu_rayon.h"

template <typename T>
auto converti_matrice_glm(dls::math::mat4x4<T> const &matrice)
{
	dls::math::mat4x4<float> resultat;

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			resultat[i][j] = static_cast<float>(matrice[i][j]);
		}
	}

	return resultat;
}

VisionneurScene::VisionneurScene(VueCanevas *parent, Kanba *kanba)
	: m_parent(parent)
	, m_kanba(kanba)
	, m_camera(kanba->camera)
	, m_rendu_brosse(nullptr)
	, m_rendu_grille(nullptr)
	, m_rendu_texte(nullptr)
	, m_rendu_maillage(nullptr)
	, m_pos_x(0)
	, m_pos_y(0)
	, m_debut(0)
{}

VisionneurScene::~VisionneurScene()
{
	delete m_rendu_maillage;
	delete m_rendu_texte;
	delete m_rendu_grille;
	delete m_rendu_brosse;
}

void VisionneurScene::initialise()
{
	glClearColor(0.5, 0.5, 0.5, 1.0);

	glEnable(GL_DEPTH_TEST);

	m_rendu_grille = new RenduGrille(20, 20);
	m_rendu_texte = new RenduTexte();
	m_rendu_brosse = new RenduBrosse;
	m_rendu_brosse->initialise();

	m_camera->ajourne();

	m_debut = dls::chrono::maintenant();
}

void VisionneurScene::peint_opengl()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_camera->ajourne();

	/* Met en place le contexte. */
	auto const &MV = m_camera->MV();
	auto const &P = m_camera->P();
	auto const &MVP = P * MV;

	m_contexte.vue(m_camera->dir());
	m_contexte.modele_vue(MV);
	m_contexte.projection(P);
	m_contexte.MVP(MVP);
	m_contexte.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
	m_contexte.matrice_objet(converti_matrice_glm(m_stack.sommet()));
	m_contexte.pour_surlignage(false);

	/* Peint la scene. */
	m_rendu_grille->dessine(m_contexte);

	if (m_kanba->maillage && !m_rendu_maillage) {
		m_rendu_maillage = new RenduMaillage(m_kanba->maillage);
		m_rendu_maillage->initialise();
	}
	else if (m_rendu_maillage && m_rendu_maillage->maillage() != m_kanba->maillage) {
		delete m_rendu_maillage;
		m_rendu_maillage = new RenduMaillage(m_kanba->maillage);
		m_rendu_maillage->initialise();
	}

	if (m_rendu_maillage) {
		m_stack.pousse(m_rendu_maillage->matrice());
		m_contexte.matrice_objet(converti_matrice_glm(m_stack.sommet()));

		m_rendu_maillage->dessine(m_contexte);

		m_stack.enleve_sommet();
	}

	auto const &diametre = static_cast<float>(m_kanba->brosse->rayon) * 2.0f;

	m_rendu_brosse->dessine(m_contexte,
							diametre / static_cast<float>(m_camera->largeur()),
							diametre / static_cast<float>(m_camera->hauteur()),
							m_pos_x,
							m_pos_y);

	auto const fin = dls::chrono::maintenant();

	auto const temps = fin - m_debut;
	auto const fps = static_cast<int>(1.0 / temps);

	std::stringstream ss;
	ss << fps << " IPS";

	glEnable(GL_BLEND);

	m_rendu_texte->reinitialise();
	m_rendu_texte->dessine(m_contexte, ss.str());

	glDisable(GL_BLEND);

	m_debut = dls::chrono::maintenant();
}

void VisionneurScene::redimensionne(int largeur, int hauteur)
{
	m_rendu_texte->etablie_dimension_fenetre(largeur, hauteur);
	m_camera->redimensionne(largeur, hauteur);
}

void VisionneurScene::position_souris(int x, int y)
{
	m_pos_x = static_cast<float>(x) / static_cast<float>(m_camera->largeur()) * 2.0f - 1.0f;
	m_pos_y = static_cast<float>(m_camera->hauteur() - y) / static_cast<float>(m_camera->hauteur()) * 2.0f - 1.0f;
}
