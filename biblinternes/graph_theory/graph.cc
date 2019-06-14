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

#include "graph.h"

#include <algorithm>

/* ************************************************************************** */

Edge::Edge(Vertex *first, Vertex *second)
    : m_first(first)
    , m_second(second)
{}

Edge::Ptr Edge::create(Vertex *first, Vertex *second)
{
	return Ptr(new Edge(first, second));
}

Vertex *Edge::first() const
{
	return m_first;
}

Vertex *Edge::second() const
{
	return m_second;
}

Vertex *Edge::getOtherEnd(const Vertex * const vertex) const
{
	if (vertex == m_first) {
		return m_second;
	}

	return m_first;
}

/* ************************************************************************** */

bool operator==(const Edge &lhs, const Edge &rhs)
{
	if (lhs.first() == rhs.first() && lhs.second() == rhs.second()) {
		return true;
	}

	if (lhs.first() == rhs.second() && lhs.second() == rhs.first()) {
		return true;
	}

	return false;
}

bool operator!=(const Edge &lhs, const Edge &rhs)
{
	return !(lhs == rhs);
}

/* ************************************************************************** */

Vertex::Vertex(std::string name)
    : Vertex()
{
	m_name = std::move(name);
}

void Vertex::addIncidentEdge(Edge *e)
{
	m_incident_edges.push_back(e);
	++m_degree;
}

void Vertex::removeIncidentEdge(Edge *e)
{
	auto iter = std::find(m_incident_edges.begin(), m_incident_edges.end(), e);

	if (iter == m_incident_edges.end()) {
		throw "Trying to remove an edge not incident to the current vertex!";
	}

	m_incident_edges.erase(iter);
	--m_degree;
}

const std::vector<Edge *> &Vertex::incident_egdes() const
{
	return m_incident_edges;
}

const std::string &Vertex::name() const
{
	return m_name;
}

bool Vertex::processed() const
{
	return m_processed;
}

void Vertex::processed(bool yesno)
{
	m_processed = yesno;
}

/* ************************************************************************** */

static void walk(Vertex * const v, std::ostream &os)
{
	if (v->processed()) {
		return;
	}

	v->processed(true);

	for (const auto &e : v->incident_egdes()) {
		walk(e->getOtherEnd(v), os);
	}

	os << v->name() << '\n';
}

/* ************************************************************************** */

void Graph::addEdge(Vertex * const first, Vertex * const second)
{
	/* avoid loops */
	if (first == second) {
		return;
	}

	Edge::Ptr edge = Edge::create(first, second);

	auto iter = std::find(m_edges.begin(), m_edges.end(), edge);

	if (iter == m_edges.end()) {
		first->addIncidentEdge(edge.get());
		second->addIncidentEdge(edge.get());
		m_edges.push_back(std::move(edge));
	}
}

void Graph::addVertex(Vertex * const v)
{
	m_vertices.push_back(v);
}

void Graph::addVertex(const std::initializer_list<Vertex *> &list)
{
	for (const auto &v : list) {
		m_vertices.push_back(v);
	}
}

void Graph::walk(std::ostream &os) const
{
	::walk(m_vertices[0], os);
}

void Graph::walk_edges(std::ostream &os) const
{
	for (const auto &e : m_edges) {
		os << e->first()->name() << " -> " << e->second()->name() << '\n';
	}
}
