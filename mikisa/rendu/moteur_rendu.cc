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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "moteur_rendu.hh"

#include <ego/outils.h>
#include <GL/glew.h>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/rendu_camera.h"
#include "bibliotheques/opengl/rendu_grille.h"
#include "bibliotheques/opengl/pile_matrice.h"
#include "bibliotheques/vision/camera.h"

#include "coeur/objet.h"
#include "coeur/scene.h"

#include "rendu_corps.h"

/* ************************************************************************** */

template <typename T>
static auto converti_matrice_glm(dls::math::mat4x4<T> const &matrice)
{
	dls::math::mat4x4<float> resultat;

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			resultat[i][j] = static_cast<float>(matrice[i][j]);
		}
	}

	return resultat;
}

/* ************************************************************************** */

/* Concernant ce déléguée_scène :
 * La finalité du MoteurRendu est d'abstraire différents moteurs de rendus
 * (traçage de rayon, ratissage, OpenGL, etc.) dans un système où il y a
 * plusieurs moteurs de rendu, et plusieurs représentation scénique différentes,
 * opérants en même temps. La Déléguée de scène servira de pont entre les
 * différentes représentations scéniques et les différents moteurs de rendus.
 * L'idée est similaire à celle présente dans Hydra de Pixar.
 */
struct deleguee_scene {
	Scene *scene = nullptr;

	long nombre_objets() const
	{
		return scene->objets().taille();
	}

	Objet *objet(long idx) const
	{
		return scene->objets()[idx];
	}
};

/* ************************************************************************** */

MoteurRendu::MoteurRendu()
	: m_delegue(memoire::loge<deleguee_scene>("Délégué Scène"))
{}

MoteurRendu::~MoteurRendu()
{
	memoire::deloge("RenduGrille", m_rendu_grille);
	memoire::deloge("Délégué Scène", m_delegue);
}

void MoteurRendu::camera(vision::Camera3D *camera)
{
	m_camera = camera;
}

void MoteurRendu::scene(Scene *scene)
{
	m_delegue->scene = scene;
}

void MoteurRendu::calcule_rendu(
		float *tampon,
		int hauteur,
		int largeur,
		bool rendu_final)
{
	/* initialise données pour le rendu */

	GLuint fbo, render_buf, tampon_prof;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	glGenRenderbuffers(1 ,&render_buf);
	glBindRenderbuffer(GL_RENDERBUFFER, render_buf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, largeur, hauteur);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buf);

	glGenRenderbuffers(1 ,&tampon_prof);
	glBindRenderbuffer(GL_RENDERBUFFER, tampon_prof);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, largeur, hauteur);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tampon_prof);

	/* ****************************************************************** */

	int taille_original[4];
	glGetIntegerv(GL_VIEWPORT, taille_original);

	glViewport(0, 0, largeur, hauteur);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* ****************************************************************** */

	/* Met en place le contexte. */
	auto contexte = ContexteRendu{};
	auto pile = PileMatrice{};

	m_camera->ajourne();

	auto const &MV = m_camera->MV();
	auto const &P = m_camera->P();
	auto const &MVP = P * MV;

	contexte.vue(m_camera->dir());
	contexte.modele_vue(MV);
	contexte.projection(P);
	contexte.MVP(MVP);
	contexte.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
	contexte.matrice_objet(converti_matrice_glm(pile.sommet()));
	contexte.pour_surlignage(false);

	/* Peint la grille. */
	if (!rendu_final) {
		if (m_rendu_grille == nullptr) {
			m_rendu_grille = memoire::loge<RenduGrille>("RenduGrille", 20, 20);
		}

		m_rendu_grille->dessine(contexte);
	}

	if (m_delegue->scene != nullptr) {
		auto camera_scene = m_delegue->scene->camera();

		if (!rendu_final && camera_scene != nullptr) {
			/* la rotation de la caméra est appliquée aux points dans
			 * RenduCamera, donc on recrée une matrice sans rotation, et dont
			 * la taille dans la scène est de 1.0 (en mettant à l'échelle
			 * avec un facteur de 1.0 / distance éloignée. */
			auto matrice = dls::math::mat4x4d(1.0);
			matrice = dls::math::translation(matrice, dls::math::vec3d(camera_scene->pos()));
			matrice = dls::math::dimension(matrice, dls::math::vec3d(static_cast<double>(1.0f / camera_scene->eloigne())));
			pile.pousse(matrice);
			contexte.matrice_objet(converti_matrice_glm(pile.sommet()));

			RenduCamera rendu_camera(camera_scene);
			rendu_camera.initialise();
			rendu_camera.dessine(contexte);

			pile.enleve_sommet();
		}

		for (auto i = 0; i < m_delegue->nombre_objets(); ++i) {
			auto objet = m_delegue->objet(i);

			pile.pousse(objet->transformation.matrice());

			objet->corps.accede_lecture([&pile, &contexte](Corps const &corps)
			{
				pile.pousse(corps.transformation.matrice());

				contexte.matrice_objet(converti_matrice_glm(pile.sommet()));

				RenduCorps rendu_corps(&corps);
				rendu_corps.initialise(contexte);
				rendu_corps.dessine(contexte);

				pile.enleve_sommet();
			});

			pile.enleve_sommet();
		}
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	/* ****************************************************************** */

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, largeur, hauteur, GL_RGBA, GL_FLOAT, tampon);

	glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &render_buf);
	glDeleteRenderbuffers(1, &tampon_prof);

	glViewport(taille_original[0], taille_original[1], taille_original[2], taille_original[3]);

	/* Inverse la direction de l'image */
	for (int x = 0; x < largeur * 4; x += 4) {
		for (int y = 0; y < hauteur / 2; ++y) {
			auto idx0 = x + largeur * y * 4;
			auto idx1 = x + largeur * (hauteur - y - 1) * 4;

			std::swap(tampon[idx0 + 0], tampon[idx1 + 0]);
			std::swap(tampon[idx0 + 1], tampon[idx1 + 1]);
			std::swap(tampon[idx0 + 2], tampon[idx1 + 2]);
			std::swap(tampon[idx0 + 3], tampon[idx1 + 3]);
		}
	}
}

void MoteurRendu::construit_scene()
{

}
