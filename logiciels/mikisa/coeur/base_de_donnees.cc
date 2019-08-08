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

#include "base_de_donnees.hh"
#include "composite.h"
#include "noeud_image.h"
#include "objet.h"

/* ************************************************************************** */

static auto cree_noeud_objet()
{
	auto noeud = memoire::loge<Noeud>("Noeud");
	noeud->type(NOEUD_OBJET);

	return noeud;
}

static auto cree_noeud_composite()
{
	auto noeud = memoire::loge<Noeud>("Noeud");
	noeud->type(NOEUD_COMPOSITE);

	return noeud;
}

static auto detruit_noeud(Noeud *noeud)
{
	memoire::deloge("Noeud", noeud);
}

/* ************************************************************************** */

/* À FAIRE : noms uniques. Les graphes des composites et objets ainsi que les
 * scènes sont pour le moment chargés de maintenir les listes des noms pour
 * éviter tout conflit. Lors de l'ouverture de fichier notamment, nous devons
 * tous détruire pour s'assurer que les listes des noms soient bel et bien
 * détruites pour ne pas colléser avec les noms des objets lus. Peut-être que
 * la gestion des noms peut être releguée à la BaseDeDonnées. */

BaseDeDonnees::BaseDeDonnees()
	: m_graphe_composites(Graphe(cree_noeud_composite, detruit_noeud))
	, m_graphe_objets(Graphe(cree_noeud_objet, detruit_noeud))
{}

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
	m_graphe_objets.supprime_tout();
	m_table_objet_noeud.efface();

	for (auto compo : m_composites) {
		memoire::deloge("compo", compo);
	}

	m_composites.efface();
	m_graphe_composites.supprime_tout();
	m_table_composites_noeud.efface();
}

/* ************************************************************************** */

Objet *BaseDeDonnees::cree_objet(dls::chaine const &nom, type_objet type)
{
	auto objet = memoire::loge<Objet>("objet");
	objet->nom = nom;
	objet->type = type;

	switch (objet->type) {
		case type_objet::NUL:
		{
			break;
		}
		case type_objet::CORPS:
		{
			objet->donnees = memoire::loge<DonneesCorps>("DonneesCorps");
			break;
		}
		case type_objet::CAMERA:
		{
			objet->donnees = memoire::loge<DonneesCamera>("DonneesCamera", 1920, 1080);
			break;
		}
	}

	auto noeud = m_graphe_objets.cree_noeud(objet->nom);

	if (objet->nom != noeud->nom()) {
		objet->nom = noeud->nom();
	}

	noeud->donnees(objet);

	m_table_objet_noeud.insere({objet, noeud});
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

	auto iter_noeud = m_table_objet_noeud.trouve(objet);

	if (iter_noeud == m_table_objet_noeud.fin()) {
		throw std::runtime_error("L'objet n'est pas la table objets/noeuds de la base de données !");
	}

	m_graphe_objets.supprime(iter_noeud->second);

	memoire::deloge("objet", objet);
}

const dls::tableau<Objet *> &BaseDeDonnees::objets() const
{
	return m_objets;
}

Graphe *BaseDeDonnees::graphe_objets()
{
	return &m_graphe_objets;
}

const Graphe *BaseDeDonnees::graphe_objets() const
{
	return &m_graphe_objets;
}

const dls::dico_desordonne<Objet *, Noeud *> &BaseDeDonnees::table_objets() const
{
	return m_table_objet_noeud;
}

/* ************************************************************************** */

Composite *BaseDeDonnees::cree_composite(dls::chaine const &nom)
{
	auto compo = memoire::loge<Composite>("compo");
	compo->nom = nom;

	auto noeud = m_graphe_composites.cree_noeud(compo->nom);

	if (compo->nom != noeud->nom()) {
		compo->nom = noeud->nom();
	}

	noeud->donnees(compo);

	m_table_composites_noeud.insere({compo, noeud});

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

void BaseDeDonnees::enleve_composite(Composite *compo)
{
	auto iter = std::find(m_composites.debut(), m_composites.fin(), compo);
	m_composites.erase(iter);

	auto iter_noeud = m_table_composites_noeud.trouve(compo);

	if (iter_noeud == m_table_composites_noeud.fin()) {
		throw std::runtime_error("Le composite n'est pas la table objets/composites de la base de données !");
	}

	m_graphe_composites.supprime(iter_noeud->second);

	memoire::deloge("compo", compo);
}

const dls::tableau<Composite *> &BaseDeDonnees::composites() const
{
	return m_composites;
}

Graphe *BaseDeDonnees::graphe_composites()
{
	return &m_graphe_composites;
}

const Graphe *BaseDeDonnees::graphe_composites() const
{
	return &m_graphe_composites;
}
