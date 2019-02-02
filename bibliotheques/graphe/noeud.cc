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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "noeud.h"

#include <tbb/tick_count.h>

/* ************************************************************************** */

PriseSortie::PriseSortie(std::string const &nom_prise)
	: nom(nom_prise)
{}

/* ************************************************************************** */

PriseEntree::PriseEntree(std::string const &nom_prise)
	: nom(nom_prise)
{}

/* ************************************************************************** */

Noeud::Noeud(void (*suppression_donnees)(std::any))
	: Noeud()
{
	supprime_donnees = suppression_donnees;
}

Noeud::~Noeud()
{
	supprime_donnees(m_donnees);

	for (auto &socket : this->m_entrees) {
		delete socket;
	}

	for (auto &socket : this->m_sorties) {
		delete socket;
	}
}

std::any Noeud::donnees() const
{
	return this->m_donnees;
}

void Noeud::donnees(std::any pointeur)
{
	this->m_donnees = pointeur;
}

std::string const &Noeud::nom() const
{
	return m_nom;
}

void Noeud::nom(std::string const &name)
{
	m_nom = name;
}

double Noeud::pos_x() const
{
	return static_cast<double>(m_rectangle.x);
}

void Noeud::pos_x(float x)
{
	m_rectangle.x = x;
}

double Noeud::pos_y() const
{
	return static_cast<double>(m_rectangle.y);
}

void Noeud::pos_y(float y)
{
	m_rectangle.y = y;
}

int Noeud::hauteur() const
{
	return static_cast<int>(m_rectangle.hauteur);
}

void Noeud::hauteur(int h)
{
	m_rectangle.hauteur = static_cast<float>(h);
}

int Noeud::largeur() const
{
	return static_cast<int>(m_rectangle.largeur);
}

void Noeud::largeur(int l)
{
	m_rectangle.largeur = static_cast<float>(l);
}

Rectangle const &Noeud::rectangle() const
{
	return m_rectangle;
}

void Noeud::ajoute_entree(std::string const &name, const int type)
{
	auto prise = new PriseEntree(name);
	prise->parent = this;
	prise->type = type;

	this->m_entrees.push_back(prise);
}

void Noeud::ajoute_sortie(std::string const &name, const int type)
{
	auto prise = new PriseSortie(name);
	prise->parent = this;
	prise->type = type;

	this->m_sorties.push_back(prise);
}

PriseEntree *Noeud::entree(size_t index)
{
	return m_entrees[index];
}

PriseEntree *Noeud::entree(std::string const &nom)
{
	for (auto const &entree : m_entrees) {
		if (entree->nom == nom) {
			return entree;
		}
	}

	return nullptr;
}

PriseSortie *Noeud::sortie(size_t index)
{
	return m_sorties[index];
}

PriseSortie *Noeud::sortie(std::string const &nom)
{
	for (auto const &sortie : m_sorties) {
		if (sortie->nom == nom) {
			return sortie;
		}
	}

	return nullptr;
}

Noeud::plage_entrees Noeud::entrees() const noexcept
{
	return plage_entrees(m_entrees.begin(), m_entrees.end());
}

Noeud::plage_sorties Noeud::sorties() const noexcept
{
	return plage_sorties(m_sorties.begin(), m_sorties.end());
}

bool Noeud::est_lie() const
{
	return a_des_entrees_liees() || a_des_sorties_liees();
}

bool Noeud::a_des_entrees_liees() const
{
	auto linked_input = false;

	for (auto const &input : m_entrees) {
		if (input->lien != nullptr) {
			linked_input = true;
			break;
		}
	}

	return linked_input;
}

bool Noeud::a_des_sorties_liees() const
{
	auto linked_output = false;

	for (auto const &output : m_sorties) {
		if (!output->liens.empty()) {
			linked_output = true;
			break;
		}
	}

	return linked_output;
}

int Noeud::type() const
{
	return m_type;
}

void Noeud::type(int what)
{
	m_type = what;
}

bool Noeud::besoin_execution() const
{
	return m_besoin_traitement;
}

void Noeud::besoin_execution(bool ouinon)
{
	m_besoin_traitement = ouinon;

	/* Réinitialise le temps d'exécution. */
	this->temps_execution(0.0f);
}

float Noeud::temps_execution() const
{
	return m_temps_execution;
}

float Noeud::temps_execution_minimum() const
{
	return m_temps_execution_min;
}

void Noeud::temps_execution(float time)
{
	m_temps_execution = time;

	if (m_temps_execution < m_temps_execution_min && m_temps_execution != 0.0f) {
		m_temps_execution_min = m_temps_execution;
	}
}

void Noeud::incremente_compte_execution()
{
	m_executions++;
}

int Noeud::compte_execution() const
{
	return m_executions;
}

/* ************************************************************************** */

void marque_surannee(Noeud *noeud)
{
	if (noeud == nullptr) {
		return;
	}

	noeud->besoin_execution(true);

	for (PriseSortie *sortie : noeud->sorties()) {
		for (PriseEntree *entree : sortie->liens) {
			marque_surannee(entree->parent);
		}
	}
}
