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

#include "scene.h"

#include <algorithm>

#include "noeud_image.h"
#include "objet.h"

/* ************************************************************************** */

static auto fonction_creation_noeud()
{
	auto noeud = memoire::loge<Noeud>("Noeud");
	noeud->type(NOEUD_OBJET);

	return noeud;
}

static auto fonction_destruction_noeud(Noeud *noeud)
{
	memoire::deloge("Noeud", noeud);
}

/* ************************************************************************** */

Scene::Scene()
	: graphe(Graphe(fonction_creation_noeud, fonction_destruction_noeud))
{}

void Scene::reinitialise()
{
	m_objets.efface();
	m_camera = nullptr;
}

void Scene::ajoute_objet(Objet *objet)
{
	auto noeud = graphe.cree_noeud(objet->nom);

	if (objet->nom != noeud->nom()) {
		objet->nom = noeud->nom();
	}

	noeud->donnees(objet);

	ajoute_objet(noeud, objet);
}

void Scene::ajoute_objet(Noeud *noeud, Objet *objet)
{
	table_objet_noeud.insere({objet, noeud});
	m_objets.pousse(objet);
}

void Scene::enleve_objet(Objet *objet)
{
	auto iter = std::find(m_objets.debut(), m_objets.fin(), objet);
	m_objets.erase(iter);

	auto iter_noeud = table_objet_noeud.trouve(objet);

	if (iter_noeud == table_objet_noeud.fin()) {
		throw std::runtime_error("L'objet n'est pas la table objets/noeuds de la scène !");
	}

	graphe.supprime(iter_noeud->second);
}

const dls::tableau<Objet *> &Scene::objets()
{
	return m_objets;
}

const dls::dico_desordonne<Objet *, Noeud *> &Scene::table_objets() const
{
	return table_objet_noeud;
}

void Scene::camera(vision::Camera3D *camera)
{
	m_camera = camera;
}

vision::Camera3D *Scene::camera()
{
	return m_camera;
}
