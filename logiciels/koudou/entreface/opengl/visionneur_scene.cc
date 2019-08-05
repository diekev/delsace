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

#include "biblinternes/opengl/rendu_grille.h"
#include "biblinternes/opengl/rendu_camera.h"
#include "biblinternes/opengl/rendu_texte.h"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/koudou.h"
#include "coeur/objet.h"

#include "rendu_lumiere.h"
#include "rendu_maillage.h"
#include "rendu_monde.h"

VisionneurScene::VisionneurScene(VueCanevas3D *parent, kdo::Koudou *koudou)
	: m_parent(parent)
	, m_koudou(koudou)
	, m_rendu_camera(new RenduCamera(koudou->camera))
	, m_rendu_grille(nullptr)
	, m_rendu_monde(nullptr)
	, m_rendu_texte(nullptr)
{}

VisionneurScene::~VisionneurScene()
{
	for (auto &rendu_maillage : m_maillages) {
		delete rendu_maillage;
	}

	for (auto &rendu_lumiere : m_lumieres) {
		delete rendu_lumiere;
	}

	delete m_rendu_texte;
	delete m_rendu_monde;
	delete m_rendu_camera;
	delete m_rendu_grille;
}

void VisionneurScene::initialise()
{
	glClearColor(0.5, 0.5, 0.5, 1.0);

	glEnable(GL_DEPTH_TEST);

	m_rendu_grille = new RenduGrille(20, 20);
	m_rendu_monde = new RenduMonde(m_koudou);
	m_rendu_texte = new RenduTexte();

	reconstruit_scene();

#ifdef NOUVELLE_CAMERA
	m_rendu_camera = new RenduCamera();
	m_rendu_camera->initialise();
#endif

	m_chrono_rendu.commence();
}

void VisionneurScene::peint_opengl()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_koudou->camera->ajourne();

	/* Met en place le contexte. */
	auto const &MV = m_koudou->camera->MV();
	auto const &P = m_koudou->camera->P();
	auto const &MVP = P * MV;

	m_contexte.vue(m_koudou->camera->dir());
	m_contexte.modele_vue(MV);
	m_contexte.projection(P);
	m_contexte.MVP(MVP);
	m_contexte.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
	m_contexte.matrice_objet(math::matf_depuis_matd(m_stack.sommet()));
	m_contexte.pour_surlignage(false);

	/* Peint la scene. */
	m_rendu_monde->ajourne();
	m_rendu_monde->dessine(m_contexte);

	m_rendu_grille->dessine(m_contexte);

#ifdef NOUVELLE_CAMERA
	auto const transform = m_koudou->parametres_rendu.camera->camera_vers_monde();
	auto const matrice = math::matf_depuis_matd(transform.matrice());

	m_stack.pousse(matrice);
	m_contexte.matrice_objet(m_stack.sommet());

	m_rendu_camera->dessine(m_contexte);

	m_stack.enleve_sommet();
#endif

	for (auto &rendu_maillage : m_maillages) {
		m_stack.pousse(rendu_maillage->matrice());
		m_contexte.matrice_objet(math::matf_depuis_matd(m_stack.sommet()));

		rendu_maillage->dessine(m_contexte, m_koudou->parametres_rendu.scene);

		m_stack.enleve_sommet();
	}

	for (auto &rendu_lumiere : m_lumieres) {
		m_stack.pousse(rendu_lumiere->matrice());
		m_contexte.matrice_objet(math::matf_depuis_matd(m_stack.sommet()));

		rendu_lumiere->dessine(m_contexte);

		m_stack.enleve_sommet();
	}

	auto const fps = static_cast<int>(1.0 / m_chrono_rendu.arrete());

	dls::flux_chaine ss;
	ss << fps << " IPS";

	glEnable(GL_BLEND);

	m_rendu_texte->reinitialise();
	m_rendu_texte->dessine(m_contexte, ss.chn());

	glDisable(GL_BLEND);

	m_chrono_rendu.commence();
}

void VisionneurScene::redimensionne(int largeur, int hauteur)
{
	m_rendu_texte->etablie_dimension_fenetre(largeur, hauteur);
	m_koudou->camera->redimensionne(largeur, hauteur);
}

void VisionneurScene::reconstruit_scene()
{
	for (auto &rendu_maillage : m_maillages) {
		delete rendu_maillage;
	}

	for (auto &rendu_lumiere : m_lumieres) {
		delete rendu_lumiere;
	}

	auto &scene = m_koudou->parametres_rendu.scene;

	m_maillages.efface();
	m_maillages.reserve(scene.maillages.taille());

	m_lumieres.efface();
	m_lumieres.reserve(scene.lumieres.taille());

	for (auto &objet : scene.objets) {
		if (objet->type == kdo::TypeObjet::LUMIERE) {
			auto rendu_lumiere = new RenduLumiere(objet->lumiere);
			rendu_lumiere->initialise();

			m_lumieres.pousse(rendu_lumiere);
		}
		else if (objet->type == kdo::TypeObjet::MAILLAGE) {
			auto rendu_maillage = new RenduMaillage(objet->maillage);
			rendu_maillage->initialise();

			m_maillages.pousse(rendu_maillage);
		}
	}
}
