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

#include "operatrice_scene.h"

#include <ego/outils.h>
#include <GL/glew.h>

#include "bibliotheques/vision/camera.h"

#include "rendu/rendu_corps.h"

#include "corps/corps.h"

#include "contexte_evaluation.hh"
#include "objet.h"

template <typename T>
static auto converti_matrice_glm(dls::math::mat4x4<T> const &matrice)
{
	dls::math::mat4x4f resultat;

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			resultat[i][j] = static_cast<float>(matrice[i][j]);
		}
	}

	return resultat;
}

/* ************************************************************************** */

OperatriceScene::OperatriceScene(Graphe &graphe_parent, Noeud *node)
	: OperatriceImage(graphe_parent, node)
	, m_graphe(cree_noeud_image, supprime_noeud_image)
{
	entrees(1);
	sorties(1);
}

int OperatriceScene::type() const
{
	return OPERATRICE_SCENE;
}

int OperatriceScene::type_entree(int n) const
{
	return OPERATRICE_CAMERA;
}

int OperatriceScene::type_sortie(int) const
{
	return OPERATRICE_IMAGE;
}

const char *OperatriceScene::chemin_entreface() const
{
	return "";
}

const char *OperatriceScene::nom_classe() const
{
	return NOM;
}

const char *OperatriceScene::texte_aide() const
{
	return AIDE;
}

Scene *OperatriceScene::scene()
{
	return &m_scene;
}

Graphe *OperatriceScene::graphe()
{
	return &m_graphe;
}

int OperatriceScene::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	auto camera = entree(0)->requiers_camera(contexte, donnees_aval);

	if (camera == nullptr) {
		ajoute_avertissement("Aucune caméra trouvée !");
		return EXECUTION_ECHOUEE;
	}

	m_scene.camera(camera);

	auto const &rectangle = contexte.resolution_rendu;

	m_image.reinitialise();
	auto tampon = m_image.ajoute_calque("image", rectangle);

	GLuint fbo, render_buf, tampon_prof;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	glGenRenderbuffers(1 ,&render_buf);
	glBindRenderbuffer(GL_RENDERBUFFER, render_buf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, static_cast<int>(rectangle.largeur), static_cast<int>(rectangle.hauteur));
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buf);

	glGenRenderbuffers(1 ,&tampon_prof);
	glBindRenderbuffer(GL_RENDERBUFFER, tampon_prof);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, static_cast<int>(rectangle.largeur), static_cast<int>(rectangle.hauteur));
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tampon_prof);

	/* ****************************************************************** */

	int taille_original[4];
	glGetIntegerv(GL_VIEWPORT, taille_original);

	glViewport(0, 0, static_cast<int>(rectangle.largeur), static_cast<int>(rectangle.hauteur));

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (camera) {
		/* Met en place le contexte. */
		auto const &MV = camera->MV();
		auto const &P = camera->P();
		auto const &MVP = P * MV;

		m_contexte.vue(camera->dir());
		m_contexte.modele_vue(MV);
		m_contexte.projection(P);
		m_contexte.MVP(MVP);
		m_contexte.matrice_objet(converti_matrice_glm(m_pile.sommet()));
		m_contexte.pour_surlignage(false);

		for (auto objet : m_scene.objets()) {
			m_pile.pousse(objet->transformation.matrice());

	//		if (objet->corps != nullptr) {
				m_pile.pousse(objet->corps.transformation.matrice());
				m_contexte.matrice_objet(converti_matrice_glm(m_pile.sommet()));

				RenduCorps rendu_corps(&objet->corps);
				rendu_corps.initialise(m_contexte);
				rendu_corps.dessine(m_contexte);

				m_pile.enleve_sommet();
	//		}

			m_pile.enleve_sommet();
		}
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	/* ****************************************************************** */

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	auto image_tampon = type_image(tampon->tampon.dimensions());

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0,0,static_cast<int>(rectangle.largeur),static_cast<int>(rectangle.hauteur),GL_RGBA,GL_FLOAT,&image_tampon[0][0][0]);

	glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &render_buf);
	glDeleteRenderbuffers(1, &tampon_prof);

	glViewport(taille_original[0], taille_original[1], taille_original[2], taille_original[3]);

	/* Inverse la direction de l'image */
	for (int y = 0; y < static_cast<int>(rectangle.hauteur); ++y) {
		for (int x = 0; x < static_cast<int>(rectangle.largeur); ++x) {
			tampon->valeur(static_cast<size_t>(x), static_cast<size_t>(y), image_tampon[static_cast<int>(rectangle.hauteur) - y - 1][x]);
		}
	}

	return EXECUTION_REUSSIE;
}
