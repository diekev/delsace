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
#include <chronometrage/utilitaires.h>
#include <sstream>

#include "bibliotheques/opengl/rendu_camera.h"
#include "bibliotheques/opengl/rendu_grille.h"
#include "bibliotheques/opengl/rendu_texte.h"
#include "bibliotheques/texture/texture.h"
#include "bibliotheques/vision/camera.h"

#include "coeur/corps/corps.h"

#include "coeur/composite.h"
#include "coeur/manipulatrice.h"
#include "coeur/mikisa.h"
#include "coeur/objet.h"
#include "coeur/scene.h"

#include "rendu/rendu_corps.h"

#include "rendu_manipulatrice.h"

template <typename T>
static auto converti_matrice_glm(const dls::math::mat4x4<T> &matrice)
{
	dls::math::mat4x4<float> resultat;

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			resultat[i][j] = static_cast<float>(matrice[i][j]);
		}
	}

	/* La taille et la position sont transposées entre OpenGL et Kanba. */
	std::swap(resultat[0][3], resultat[3][0]);
	std::swap(resultat[1][3], resultat[3][1]);
	std::swap(resultat[2][3], resultat[3][2]);

	return resultat;
}

#if 0
class MoteurRendu {
public:
	void ajoute_camera(Camera *camera, math::transformation matrice);

	void ajoute_maillage(Maillage *maillage, math::transformation matrice);
};
#endif

VisionneurScene::VisionneurScene(VueCanevas3D *parent, Mikisa &mikisa)
	: m_parent(parent)
	, m_mikisa(mikisa)
	, m_camera(mikisa.camera_3d)
	, m_rendu_grille(nullptr)
	, m_rendu_texte(nullptr)
	, m_rendu_manipulatrice_pos(nullptr)
	, m_rendu_manipulatrice_rot(nullptr)
	, m_rendu_manipulatrice_ech(nullptr)
	, m_pos_x(0)
	, m_pos_y(0)
	, m_debut(0)
{}

VisionneurScene::~VisionneurScene()
{
	delete m_rendu_grille;
	delete m_rendu_texte;
	delete m_rendu_manipulatrice_pos;
	delete m_rendu_manipulatrice_rot;
	delete m_rendu_manipulatrice_ech;
}

void VisionneurScene::initialise()
{
	glClearColor(0.5, 0.5, 0.5, 1.0);

	glEnable(GL_DEPTH_TEST);

	m_rendu_grille = new RenduGrille(20, 20);
	m_rendu_texte = new RenduTexte();
	m_rendu_manipulatrice_pos = new RenduManipulatricePosition();
	m_rendu_manipulatrice_rot = new RenduManipulatriceRotation();
	m_rendu_manipulatrice_ech = new RenduManipulatriceEchelle();

	m_camera->ajourne();

	m_debut = numero7::chronometrage::maintenant();
}

void VisionneurScene::peint_opengl()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_camera->ajourne();

	/* Met en place le contexte. */
	const auto &MV = m_camera->MV();
	const auto &P = m_camera->P();
	const auto &MVP = P * MV;

	m_contexte.vue(m_camera->dir());
	m_contexte.modele_vue(MV);
	m_contexte.projection(P);
	m_contexte.MVP(MVP);
	m_contexte.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
	m_contexte.matrice_objet(converti_matrice_glm(m_stack.sommet()));
	m_contexte.pour_surlignage(false);

	/* Peint la grille. */
	m_rendu_grille->dessine(m_contexte);

	/* Peint le noeud 3D courant. */
	auto noeud = m_mikisa.derniere_scene_selectionnee;

	if (noeud != nullptr) {
		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
		auto scene = operatrice->scene();
		auto camera = scene->camera();

		if (camera) {
			/* la rotation de la caméra est appliquée aux points dans
			 * RenduCamera, donc on recrée une matrice sans rotation, et dont
			 * la taille dans la scène est de 1.0 (en mettant à l'échelle
			 * avec un facteur de 1.0 / distance éloignée. */
			auto matrice = dls::math::mat4x4d(1.0);
			matrice = dls::math::translation(matrice, dls::math::vec3d(camera->pos()));
			matrice = dls::math::dimension(matrice, dls::math::vec3d(static_cast<double>(1.0f / camera->eloigne())));
			m_stack.pousse(matrice);
			m_contexte.matrice_objet(converti_matrice_glm(m_stack.sommet()));

			RenduCamera rendu_camera(camera);
			rendu_camera.initialise();
			rendu_camera.dessine(m_contexte);

			m_stack.enleve_sommet();
		}

		for (auto objet : scene->objets()) {
			m_stack.pousse(objet->transformation.matrice());

			if (objet->corps != nullptr) {
				m_stack.pousse(objet->corps->transformation.matrice());
				m_contexte.matrice_objet(converti_matrice_glm(m_stack.sommet()));

				RenduCorps rendu_corps(objet->corps);
				rendu_corps.initialise();
				rendu_corps.dessine(m_contexte);

				m_stack.enleve_sommet();
			}

			m_stack.enleve_sommet();
		}
	}

	glDisable(GL_DEPTH_TEST);

	if (m_mikisa.manipulation_3d_activee && m_mikisa.manipulatrice_3d) {
		auto pos = m_mikisa.manipulatrice_3d->pos();
		auto matrice = dls::math::mat4x4d(1.0);
		matrice = dls::math::translation(matrice, dls::math::vec3d(pos.x, pos.y, pos.z));
		m_stack.pousse(matrice);
		m_contexte.matrice_objet(converti_matrice_glm(m_stack.sommet()));

		if (m_mikisa.type_manipulation_3d == MANIPULATION_ROTATION) {
			m_rendu_manipulatrice_rot->manipulatrice(m_mikisa.manipulatrice_3d);
			m_rendu_manipulatrice_rot->dessine(m_contexte);
		}
		else if (m_mikisa.type_manipulation_3d == MANIPULATION_ECHELLE) {
			m_rendu_manipulatrice_ech->manipulatrice(m_mikisa.manipulatrice_3d);
			m_rendu_manipulatrice_ech->dessine(m_contexte);
		}
		else if (m_mikisa.type_manipulation_3d == MANIPULATION_POSITION) {
			m_rendu_manipulatrice_pos->manipulatrice(m_mikisa.manipulatrice_3d);
			m_rendu_manipulatrice_pos->dessine(m_contexte);
		}

		m_stack.enleve_sommet();
	}

	glEnable(GL_DEPTH_TEST);

	const auto fin = numero7::chronometrage::maintenant();

	const auto temps = fin - m_debut;
	const auto fps = static_cast<int>(1.0 / temps);

	std::stringstream ss;
	ss << fps << " IPS";

	glEnable(GL_BLEND);

	m_rendu_texte->reinitialise();
	m_rendu_texte->dessine(m_contexte, ss.str());

	if (noeud != nullptr) {
		ss.str("");
		ss << "Scène : " << noeud->nom();

		m_rendu_texte->dessine(m_contexte, ss.str());
	}

#if 0
	noeud = m_mikisa.graphe->noeud_actif;

	if (noeud != nullptr) {
		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

		if (operatrice->type() == OPERATRICE_OBJET && operatrice->objet()) {
			auto objet = operatrice->objet();
			auto maillage = objet->donnees;

			ss.str("");
			ss << "Maillage : " << maillage->nom;
			m_rendu_texte->dessine(m_contexte, ss.str());
			ss.str("");
			ss << "Nombre sommets   : " << maillage->nombre_sommets();
			m_rendu_texte->dessine(m_contexte, ss.str());
			ss.str("");
			ss << "Nombre polygones : " << maillage->nombre_polygones();
			m_rendu_texte->dessine(m_contexte, ss.str());
			ss.str("");
			ss << "Nombre arrêtes   : " << maillage->nombre_arretes();
			m_rendu_texte->dessine(m_contexte, ss.str());
			ss.str("");
			ss << "Nombre uvs       : " << maillage->nombre_uvs();
			m_rendu_texte->dessine(m_contexte, ss.str());
			ss.str("");
			ss << "Nombre normaux   : " << maillage->nombre_normaux();
			m_rendu_texte->dessine(m_contexte, ss.str());
		}
	}
#endif

	glDisable(GL_BLEND);

	m_debut = numero7::chronometrage::maintenant();
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
