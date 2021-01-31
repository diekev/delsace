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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "moteur_rendu_cycles.hh"

#include "cycles.hh"

#include "coeur/objet.h"
#include "corps/iteration_corps.hh"

// À FAIRE : map objet jorjala -> objet cycles
// À FAIRE : drapeau sur les Corps pour indiquer s'ils sont sales (à resynchroniser)

static ccl::Transform convertis_transformation(dls::math::mat4x4f const &matrice)
{
	// À FAIRE
	return ccl::transform_identity();
}

static void synchronise_lumiere(ccl::Scene *scene, const Lumiere &lumiere)
{
	auto noeud = scene->create_node<ccl::Light>();

	auto spectre = lumiere.spectre * lumiere.intensite;
	noeud->set_strength(ccl::make_float3(spectre.r, spectre.v, spectre.b));

	if (lumiere.type == LUMIERE_POINT) {
		noeud->set_light_type(ccl::LightType::LIGHT_POINT);
	}
	else if (lumiere.type == LUMIERE_DISTANTE) {
		noeud->set_light_type(ccl::LightType::LIGHT_DISTANT);
		// À FAIRE : direction
	}

	noeud->tag_update(scene);
}

static ccl::Mesh *synchronise_maillage(ccl::Scene *scene, const Corps &corps)
{
	auto noeud = scene->create_node<ccl::Mesh>();
	scene->geometry.push_back(noeud);

	// À FAIRE : nuanceurs
	auto used_shaders = ccl::array<ccl::Node *>();
	used_shaders.push_back_slow(scene->default_surface);
	noeud->set_used_shaders(used_shaders);

	int nombre_de_triangles = 0;

	pour_chaque_polygone(corps, [&](Corps const &, Polygone *poly)
	{
		nombre_de_triangles += static_cast<int>(poly->nombre_sommets() - 2);
	});

	auto points = corps.points_pour_lecture();
	noeud->reserve_mesh(static_cast<int>(points.taille()), nombre_de_triangles);

	for (auto j = 0; j < points.taille(); ++j) {
		auto const &p = points.point_local(j);
		noeud->add_vertex(ccl::make_float3(p.x, p.y, p.z));
	}

	// À FAIRE : divise les triangles selon l'axe le plus court
	// À FAIRE : attributs
	pour_chaque_polygone(corps, [&](Corps const &, Polygone *poly)
	{
		auto nombre_sommets = poly->nombre_sommets();

		if (nombre_sommets == 4) {
			auto i0 = static_cast<int>(poly->index_point(0));
			auto i1 = static_cast<int>(poly->index_point(1));
			auto i2 = static_cast<int>(poly->index_point(2));
			auto i3 = static_cast<int>(poly->index_point(3));

			noeud->add_triangle(i0, i1, i2, 0, true);
			noeud->add_triangle(i0, i2, i3, 0, true);
		}
		else {
			for (auto j = 2; j < poly->nombre_sommets(); ++j) {
				auto i0 = static_cast<int>(poly->index_point(0));
				auto i1 = static_cast<int>(poly->index_point(j - 1));
				auto i2 = static_cast<int>(poly->index_point(j));

				noeud->add_triangle(i0, i1, i2, 0, true);
			}
		}
	});

	noeud->tag_update(scene, true);

	return noeud;
}

static ccl::Geometry *synchronise_geometrie(ccl::Scene *scene, Corps const &corps)
{
	if (possede_sphere(corps)) {
		// pas de rendu de point dans Cycles pour le moment
		return nullptr;
	}

	if (possede_volume(corps)) {
		// À FAIRE
		return nullptr;
	}

	return synchronise_maillage(scene, corps);
}

MoteurRenduCycles::~MoteurRenduCycles()
{
	if (m_session) {
		memoire::deloge("ccl::Session", m_session);
	}

	if (t) {
		t->join();

		delete t;
	}
}

void MoteurRenduCycles::calcule_rendu(StatistiquesRendu &stats, float *tampon, int hauteur, int largeur, bool rendu_final)
{
	auto session_params = ccl::SessionParams();
	session_params.background = false;
	session_params.progressive = true;

	auto buffer_params = ccl::BufferParams();
	buffer_params.width = largeur;
	buffer_params.height = hauteur;
	buffer_params.full_width = largeur;
	buffer_params.full_height = hauteur;

	auto scene_params = ccl::SceneParams();

	if (m_session == nullptr) {
		m_session = memoire::loge<ccl::Session>("ccl::Session", session_params);
		m_session->scene = new ccl::Scene(scene_params, m_session->device);
		m_scene = m_session->scene;
	}

	m_session->reset(buffer_params, 10);

	/* synchronise les objets */
	for (auto i = 0; i < m_delegue->nombre_objets(); ++i) {
		auto const &objet_rendu = m_delegue->objet(i);
		auto objet = objet_rendu.objet;

		objet->donnees.accede_lecture([&](DonneesObjet const *donnees)
		{
			if (objet->type == type_objet::CORPS) {
				auto const &corps = extrait_corps(donnees);

				auto geometrie = synchronise_geometrie(m_scene, corps);

				if (!geometrie) {
					return;
				}

				if (objet_rendu.matrices.taille() == 0) {
					auto noeud = m_scene->create_node<ccl::Object>();
					noeud->set_geometry(geometrie);
					noeud->set_tfm(convertis_transformation(math::matf_depuis_matd(objet->transformation.matrice())));

					m_scene->objects.push_back(noeud);
				}
				else {
					/* Nous avons des instances. */
					for (auto &matrice : objet_rendu.matrices) {
						auto noeud = m_scene->create_node<ccl::Object>();
						noeud->set_geometry(geometrie);
						noeud->set_tfm(convertis_transformation(matrice));

						m_scene->objects.push_back(noeud);
					}
				}
			}
			else if (objet->type == type_objet::LUMIERE) {
				auto const &lumiere = extrait_lumiere(donnees);
				synchronise_lumiere(m_scene, lumiere);
			}
		});
	}

	if (!t) {
		t = new std::thread(
					[this] {
						m_session->start();
						m_session->wait();
			});
	}
}
