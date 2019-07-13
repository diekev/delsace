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

#include "base_de_donnees.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "composite.h"
#include "objet.h"
#include "scene.h"

/* À FAIRE : noms uniques. Les graphes des composites et objets ainsi que les
 * scènes sont pour le moment chargés de maintenir les listes des noms pour
 * éviter tout conflit. Lors de l'ouverture de fichier notamment, nous devons
 * tous détruire pour s'assurer que les listes des noms soient bel et bien
 * détruites pour ne pas colléser avec les noms des objets lus. Peut-être que
 * la gestion des noms peut être releguée à la BaseDeDonnées. */

BaseDeDonnees::~BaseDeDonnees()
{
	reinitialise();
}

void BaseDeDonnees::reinitialise()
{
	for (auto objet : m_objets) {
		memoire::deloge("objet", objet);
	}

	m_objets.efface();

	for (auto scene : m_scenes) {
		memoire::deloge("scene", scene);
	}

	m_scenes.efface();

	for (auto compo : m_composites) {
		memoire::deloge("compo", compo);
	}

	m_composites.efface();
}

Objet *BaseDeDonnees::cree_objet(dls::chaine const &nom)
{
	auto objet = memoire::loge<Objet>("objet");
	objet->nom = nom;

	m_objets.pousse(objet);

	return objet;
}

Objet *BaseDeDonnees::objet(dls::chaine const &nom) const
{
	for (auto objet : m_objets) {
		if (objet->nom == nom) {
			return objet;
		}
	}

	return nullptr;
}

void BaseDeDonnees::enleve_objet(Objet *objet)
{
	auto iter = std::find(m_objets.debut(), m_objets.fin(), objet);
	m_objets.erase(iter);
	delete objet;
}

const dls::tableau<Objet *> &BaseDeDonnees::objets() const
{
	return m_objets;
}

Scene *BaseDeDonnees::cree_scene(dls::chaine const &nom)
{
	auto scene = memoire::loge<Scene>("scene");
	scene->nom = nom;

	m_scenes.pousse(scene);

	return scene;
}

Scene *BaseDeDonnees::scene(const dls::chaine &nom) const
{
	for (auto scene : m_scenes) {
		if (scene->nom == nom) {
			return scene;
		}
	}

	return nullptr;
}

const dls::tableau<Scene *> &BaseDeDonnees::scenes() const
{
	return m_scenes;
}

Composite *BaseDeDonnees::cree_composite(dls::chaine const &nom)
{
	auto compo = memoire::loge<Composite>("compo");
	compo->nom = nom;

	m_composites.pousse(compo);

	return compo;
}

Composite *BaseDeDonnees::composite(dls::chaine const &nom) const
{
	for (auto compo : m_composites) {
		if (compo->nom == nom) {
			return compo;
		}
	}

	return nullptr;
}

const dls::tableau<Composite *> &BaseDeDonnees::composites() const
{
	return m_composites;
}
