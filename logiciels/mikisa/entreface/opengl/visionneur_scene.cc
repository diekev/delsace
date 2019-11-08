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

#include "biblinternes/math/transformation.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/opengl/rendu_texte.h"
#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/contexte_evaluation.hh"
#include "coeur/manipulatrice.h"
#include "coeur/mikisa.h"
#include "coeur/rendu.hh"

#include "lcc/lcc.hh"

#include "rendu/moteur_rendu_koudou.hh"
#include "rendu/moteur_rendu_opengl.hh"
#include "rendu/rendu_corps.h"

#include "rendu_image.h"
#include "rendu_manipulatrice.h"

/* ************************************************************************** */

VisionneurScene::VisionneurScene(VueCanevas3D *parent, Mikisa &mikisa)
	: m_parent(parent)
	, m_mikisa(mikisa)
	, m_camera(mikisa.camera_3d)
	, m_rendu_texte(nullptr)
	, m_rendu_manipulatrice_pos(nullptr)
	, m_rendu_manipulatrice_rot(nullptr)
	, m_rendu_manipulatrice_ech(nullptr)
	, m_pos_x(0)
	, m_pos_y(0)
{}

VisionneurScene::~VisionneurScene()
{
	memoire::deloge("RenduTexte", m_rendu_texte);
	memoire::deloge("RenduManipulatricePosition", m_rendu_manipulatrice_pos);
	memoire::deloge("RenduManipulatriceRotation", m_rendu_manipulatrice_rot);
	memoire::deloge("RenduManipulatriceEchelle", m_rendu_manipulatrice_ech);

	memoire::deloge("TamponRendu", m_tampon_image);

	if (m_tampon != nullptr) {
		memoire::deloge_tableau("tampon_vue3d", m_tampon, m_camera->hauteur() * m_camera->largeur() * 4);
	}
}

void VisionneurScene::initialise()
{
	m_tampon_image = cree_tampon_image();
	m_rendu_texte = memoire::loge<RenduTexte>("RenduTexte");
	m_rendu_manipulatrice_pos = memoire::loge<RenduManipulatricePosition>("RenduManipulatricePosition");
	m_rendu_manipulatrice_rot = memoire::loge<RenduManipulatriceRotation>("RenduManipulatriceRotation");
	m_rendu_manipulatrice_ech = memoire::loge<RenduManipulatriceEchelle>("RenduManipulatriceEchelle");

	m_camera->ajourne();

	m_chrono_rendu.commence();
}

void VisionneurScene::peint_opengl()
{
	/* dessine la scène dans le tampon */
	auto stats = StatistiquesRendu{};

	auto contexte_eval = cree_contexte_evaluation(m_mikisa);
	contexte_eval.rendu_final = false;
	contexte_eval.camera_rendu = m_camera;
	contexte_eval.tampon_rendu = m_tampon;
	contexte_eval.stats_rendu = &stats;

	auto rendu = m_mikisa.bdd.rendu(m_nom_rendu);

	evalue_graphe_rendu(rendu, contexte_eval);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	/* dessine le tampon */

	int taille[2] = {
		m_camera->largeur(),
		m_camera->hauteur()
	};

	genere_texture_image(m_tampon_image, m_tampon, taille);

	m_contexte.MVP(dls::math::mat4x4f(1.0f));
	m_contexte.matrice_objet(dls::math::mat4x4f(1.0f));

	m_tampon_image->dessine(m_contexte);

	/* dessine les surperpositions */

	auto const &MV = m_camera->MV();
	auto const &P = m_camera->P();
	auto const &MVP = P * MV;

	m_contexte.vue(m_camera->dir());
	m_contexte.modele_vue(MV);
	m_contexte.projection(P);
	m_contexte.MVP(MVP);
	m_contexte.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));

	if (m_mikisa.manipulation_3d_activee && m_mikisa.manipulatrice_3d) {
		auto pos = m_mikisa.manipulatrice_3d->pos();
		auto matrice = dls::math::mat4x4d(1.0);
		matrice = dls::math::translation(matrice, dls::math::vec3d(pos.x, pos.y, pos.z));
		m_stack.pousse(matrice);
		m_contexte.matrice_objet(math::matf_depuis_matd(m_stack.sommet()));

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

	auto const fps = static_cast<int>(1.0 / m_chrono_rendu.arrete());

	glEnable(GL_BLEND);

	m_rendu_texte->reinitialise();

	dls::flux_chaine ss;
	ss << fps << " IPS";

	auto couleur_fps = dls::math::vec4f(1.0f);

	if (fps < 12) {
		couleur_fps = dls::math::vec4f(0.8f, 0.1f, 0.1f, 1.0f);
	}

	m_rendu_texte->dessine(m_contexte, ss.chn(), couleur_fps);

	ss.chn("");
	ss << "Mémoire allouée   : " << memoire::formate_taille(memoire::allouee());
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Mémoire consommée : " << memoire::formate_taille(memoire::consommee());
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre objets     : " << stats.nombre_objets;
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre points     : " << stats.nombre_points;
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre polygones  : " << stats.nombre_polygones;
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre polylignes : " << stats.nombre_polylignes;
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre volumes    : " << stats.nombre_volumes;
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre commandes  : " << m_mikisa.usine_commandes().taille();
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre noeuds     : " << m_mikisa.usine_operatrices().num_entries();
	m_rendu_texte->dessine(m_contexte, ss.chn());

	ss.chn("");
	ss << "Nombre fonctions  : " << m_mikisa.lcc->fonctions.table.taille();
	m_rendu_texte->dessine(m_contexte, ss.chn());

	if (stats.temps != 0.0) {
		ss.chn("");
		ss << "Temps             : " << stats.temps << 's';
		m_rendu_texte->dessine(m_contexte, ss.chn());
	}

	glDisable(GL_BLEND);

	m_chrono_rendu.commence();
}

void VisionneurScene::redimensionne(int largeur, int hauteur)
{
	if (m_tampon != nullptr) {
		memoire::deloge_tableau("tampon_vue3d", m_tampon, m_camera->largeur() * m_camera->hauteur() * 4);
	}

	m_rendu_texte->etablie_dimension_fenetre(largeur, hauteur);
	m_camera->redimensionne(largeur, hauteur);

	m_tampon = memoire::loge_tableau<float>("tampon_vue3d", hauteur * largeur * 4);
}

void VisionneurScene::position_souris(int x, int y)
{
	m_pos_x = static_cast<float>(x) / static_cast<float>(m_camera->largeur()) * 2.0f - 1.0f;
	m_pos_y = static_cast<float>(m_camera->hauteur() - y) / static_cast<float>(m_camera->hauteur()) * 2.0f - 1.0f;
}

void VisionneurScene::change_moteur_rendu(const dls::chaine &id)
{
	m_nom_rendu = id;
}
