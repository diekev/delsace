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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrice.h"

#include <iostream>

#include <tbb/tick_count.h>

#include "context.h"
#include "noeud.h"
#include "primitive.h"

/* ************************************************************************** */

void execute_operatrice(Operatrice *operatrice, Context const &contexte, double temps)
{
	if (operatrice->a_tampon() && !operatrice->besoin_execution()) {
		return;
	}

	operatrice->supprime_avertissements();

	auto t0 = tbb::tick_count::now();

	try {
		operatrice->execute(contexte, temps);
	}
	catch (std::exception const &e) {
		operatrice->ajoute_avertissement(e.what());
	}

	auto t1 = tbb::tick_count::now();
	auto delta = (t1 - t0).seconds();

	operatrice->incremente_nombre_execution();

	operatrice->temps_agrege(delta);

	/* Pour calculer le temps d'exécution de l'opérateur, on soustrait le temps
	 * d'exécution agrégé des noeuds en amont de celui du neoud courant. */
	auto temps_agrege_parent = 0.0;

	for (size_t i = 0; i < operatrice->entrees(); ++i) {
		auto entree = operatrice->entree(i);

		temps_agrege_parent += entree->temps_execution_parent();
	}

	operatrice->temps_execution(delta - temps_agrege_parent);

	operatrice->besoin_execution(false);
}

/* ************************************************************************** */

EntreeOperatrice::EntreeOperatrice(PriseEntree *prise)
	: m_prise(prise)
{}

PrimitiveCollection *EntreeOperatrice::requiers_collection(PrimitiveCollection *collection, Context const &contexte, double temps)
{
	if (!est_connectee()) {
		return nullptr;
	}

	auto operatrice = m_prise->lien->parent->operatrice();

	execute_operatrice(operatrice, contexte, temps);

	auto collection_operatrice = operatrice->collection();

	if (collection_operatrice == nullptr) {
		return nullptr;
	}

	if (collection == nullptr) {
		return collection_operatrice;
	}

	/* S'il y a plusieurs liens, copie la collection afin d'éviter tout conflit. */
	if (m_prise->lien->liens.size() > 1) {
		collection_operatrice->copy_collection(*collection);
		operatrice->a_tampon(true);
	}
	else {
		/* Autrement, copie la collection et vide l'original. */
		collection->merge_collection(*collection_operatrice);
		operatrice->a_tampon(false);
	}

	return collection;
}

double EntreeOperatrice::temps_execution_parent() const
{
	if (!est_connectee()) {
		return 0.0;
	}

	return m_prise->lien->parent->operatrice()->temps_agrege();
}

bool EntreeOperatrice::est_connectee() const
{
	return (m_prise != nullptr && m_prise->lien != nullptr);
}

/* ************************************************************************** */

Operatrice::Operatrice(Noeud *noeud, Context const &contexte)
{
	noeud->operatrice(this);
	m_collection = new PrimitiveCollection(contexte.primitive_factory);
}

Operatrice::~Operatrice()
{
	delete m_collection;
}

type_operatrice Operatrice::type() const
{
	return type_operatrice::STATIC;
}

EntreeOperatrice *Operatrice::entree(size_t index)
{
	if (index >= m_donnees_entree.size()) {
		return nullptr;
	}

	return &m_donnees_entree[index];
}

void Operatrice::entrees(size_t nombre)
{
	m_nombre_entrees = nombre;
	m_donnees_entree.resize(nombre);
}

size_t Operatrice::entrees() const
{
	return m_nombre_entrees;
}

void Operatrice::sorties(size_t nombre)
{
	m_nombre_sorties = nombre;
}

size_t Operatrice::sorties() const
{
	return m_nombre_sorties;
}

void Operatrice::donnee_entree(size_t index, PriseEntree *prise)
{
	if (index >= m_donnees_entree.size()) {
		std::cerr << "Débordement lors de l'assignement de données d'entrées"
				  << "(nombre d'entrées: " << m_donnees_entree.size()
				  << ", index: " << index << ")\n";
		return;
	}

	m_donnees_entree[index] = EntreeOperatrice(prise);
}

PrimitiveCollection *Operatrice::collection() const
{
	return m_collection;
}

bool Operatrice::besoin_execution() const
{
	return m_besoin_execution;
}

void Operatrice::besoin_execution(bool ouinon)
{
	m_besoin_execution = ouinon;
}

double Operatrice::temps_agrege() const
{
	return m_temps_agrege;
}

void Operatrice::temps_agrege(double temps)
{
	m_min_temps_agrege = std::min(m_temps_agrege, temps);
	m_temps_agrege = temps;
}

double Operatrice::temps_execution() const
{
	return m_temps_execution;
}

void Operatrice::temps_execution(double temps)
{
	m_min_temps_execution = std::min(m_temps_execution, temps);
	m_temps_execution = temps;
}

double Operatrice::min_temps_agrege() const
{
	return m_min_temps_agrege;
}

double Operatrice::min_temps_execution() const
{
	return m_min_temps_execution;
}

int Operatrice::nombre_executions() const
{
	return m_nombre_executions;
}

void Operatrice::incremente_nombre_execution()
{
	m_nombre_executions += 1;
}

void Operatrice::ajoute_avertissement(std::string const &avertissement)
{
	m_avertissements.push_back(avertissement);
}

std::vector<std::string> const &Operatrice::avertissements() const
{
	return m_avertissements;
}

bool Operatrice::a_avertissements() const
{
	return !m_avertissements.empty();
}

void Operatrice::supprime_avertissements()
{
	m_avertissements.clear();
}

void Operatrice::a_tampon(bool ouinon)
{
	m_a_tampon = ouinon;
}

bool Operatrice::a_tampon() const
{
	return m_a_tampon;
}

std::string Operatrice::chemin_icone() const
{
	return m_chemin_icone;
}

void Operatrice::chemin_icone(std::string const &chemin)
{
	m_chemin_icone = chemin;
}

const char *Operatrice::chemin_entreface() const
{
	return "";
}

/* ************************************************************************** */

DescOperatrice::DescOperatrice(std::string const &opname, std::string const &ophelp, std::string const &opcategorie, DescOperatrice::fonction_usine func)
	: nom(opname)
	, categorie(opcategorie)
	, text_aide(ophelp)
	, construction_operatrice(func)
{}

/* ************************************************************************** */

size_t UsineOperatrice::enregistre_type(std::string const &nom, DescOperatrice desc)
{
	auto const iter = m_tableau.find(nom);
	assert(iter == m_tableau.end());

	m_categories.emplace(desc.categorie);
	m_tableau[nom] = desc;

	return nombre_entrees();
}

Operatrice *UsineOperatrice::operator()(std::string const &nom, Noeud *noeud, Context const &contexte)
{
	auto const iter = m_tableau.find(nom);
	assert(iter != m_tableau.end());

	DescOperatrice const &desc = iter->second;

	return desc.construction_operatrice(noeud, contexte);
}

std::set<std::string> const &UsineOperatrice::categories() const
{
	return m_categories;
}

std::vector<DescOperatrice> UsineOperatrice::cles(std::string const &categorie) const
{
	std::vector<DescOperatrice> v;
	v.reserve(nombre_entrees());

	for (auto const &entree : m_tableau) {
		DescOperatrice desc = entree.second;

		if (desc.categorie == categorie) {
			v.push_back(entree.second);
		}
	}

	return v;
}

bool UsineOperatrice::est_enregistre(std::string const &cle) const
{
	return (m_tableau.find(cle) != m_tableau.end());
}
