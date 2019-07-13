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

#pragma once

#include <iostream>
#include <memory>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

class Vertex;

/* ************************************************************************** */

class Edge {
	Vertex *m_first;
	Vertex *m_second;

public:
	Edge() = delete;

	Edge(Vertex *first, Vertex *second);

	~Edge() = default;

	using Ptr = std::unique_ptr<Edge>;

	static Ptr create(Vertex *first, Vertex *second);

	Vertex *first() const;
	Vertex *second() const;

	Vertex *getOtherEnd(const Vertex * const vertex) const;
};

bool operator==(const Edge &lhs, const Edge &rhs);
bool operator!=(const Edge &lhs, const Edge &rhs);

/* ************************************************************************** */

class Vertex {
	dls::tableau<Edge *> m_incident_edges{};
	int m_degree = 0;
	dls::chaine m_name = "";
	bool m_processed = false;

public:
	Vertex() = default;

	explicit Vertex(dls::chaine name);

	~Vertex() = default;

	void addIncidentEdge(Edge *e);

	void removeIncidentEdge(Edge *e);

	const dls::tableau<Edge *> &incident_egdes() const;

	const dls::chaine &name() const;

	bool processed() const;

	void processed(bool yesno);
};

/* ************************************************************************** */

class Graph {
	dls::tableau<Vertex *> m_vertices{};
	dls::tableau<Edge::Ptr> m_edges{};

public:
	Graph() = default;
	~Graph() = default;

	void addEdge(Vertex * const first, Vertex * const second);

	void addVertex(Vertex * const v);

	void addVertex(const std::initializer_list<Vertex *> &list);

	void walk(std::ostream &os) const;

	void walk_edges(std::ostream &os) const;
};
