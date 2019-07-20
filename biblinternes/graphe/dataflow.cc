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

#include "dataflow.h"

#include <chrono>
#include <iostream>
#include <queue>
#include <thread>

/*** Presto ****/

/* ----------------- graphes et réseaux ----------------- */

class Graph {

};

class Reseau {
	const char *m_nom = "";

public:
	Reseau(const char *nom)
	    : m_nom(nom)
	{}

	const char *nom() const
	{
		return m_nom;
	}
};

class Compilateur {
	Graph *m_data = nullptr;

public:
	Compilateur() = default;

	Compilateur(Graph *data)
	    : m_data(data)
	{}

	Reseau *compile(Graph *, const char *nom)
	{
		/* compile m_data into a network that can be evaluated */
		return new Reseau(nom);
	}
};

class Executeur {
public:
	void execute(Reseau *reseau)
	{
		std::cerr << "Execution du réseau '" << reseau->nom() << "'...\n";
	}
};

/* ----------------- planification de tâches ----------------- */

using time_point = std::chrono::system_clock::time_point;

class Plan {
	time_point m_temps;
	std::shared_ptr<Reseau> m_reseau;

public:
	Plan(time_point temps, Reseau *reseau)
	    : m_temps(temps)
	    , m_reseau(reseau)
	{}

	Plan(const Plan &rhs)
	    : m_temps(rhs.temps())
	    , m_reseau(rhs.m_reseau)
	{
	}

	~Plan() = default;

	Plan &operator=(const Plan &rhs)
	{
		m_temps = rhs.temps();
		m_reseau = rhs.m_reseau;
		return *this;
	}

	const time_point &temps() const
	{
		return m_temps;
	}

	Reseau *reseau() const
	{
		return m_reseau.get();
	}
};

inline bool operator<(const Plan &lhs, const Plan &rhs)
{
	return lhs.temps() > rhs.temps();
}

class Planificateur {
	std::priority_queue<Plan> m_plans{};
	Executeur m_executeur = {};
	bool m_continue = true;
	std::unique_ptr<std::thread> m_thread;

public:
	Planificateur()
	    : m_thread(new std::thread([this](){ thread_loop(); }))
	{}

	~Planificateur()
	{
		while (!m_plans.empty()) {
			/* Faisons en sorte que tous les plans soient executé. */
		}

		m_continue = false;
		m_thread->join();
	}

	void planifie(Reseau *reseau)
	{
		m_plans.push(Plan(std::chrono::system_clock::now(), reseau));
	}

	void thread_loop()
	{
		while (m_continue) {
			auto now = std::chrono::system_clock::now();

			if (m_plans.empty()) {
				std::cerr << "Il n'y a aucun plan à exécuter\n";
			}
			else if (m_plans.top().temps() >= now) {
				std::cerr << "Le premier plan est dans le future\n";
			}

			while(!m_plans.empty() && m_plans.top().temps() <= now) {
				Plan plan = m_plans.top();
				m_executeur.execute(plan.reseau());
				m_plans.pop();
			}

			if (m_plans.empty()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			else {
				std::this_thread::sleep_for(m_plans.top().temps() - std::chrono::system_clock::now());
			}
		}
	}
};

/* ----------------- tests ----------------- */

void test_planification()
{
	Planificateur planificateur;
	Compilateur compilateur;

	planificateur.planifie(compilateur.compile(nullptr, "primidi"));
	planificateur.planifie(compilateur.compile(nullptr, "duodi"));
	planificateur.planifie(compilateur.compile(nullptr, "tridi"));
	planificateur.planifie(compilateur.compile(nullptr, "quartidi"));
	planificateur.planifie(compilateur.compile(nullptr, "quintidi"));
	planificateur.planifie(compilateur.compile(nullptr, "sextidi"));
	planificateur.planifie(compilateur.compile(nullptr, "septidi"));
	planificateur.planifie(compilateur.compile(nullptr, "octidi"));
	planificateur.planifie(compilateur.compile(nullptr, "nonidi"));
	planificateur.planifie(compilateur.compile(nullptr, "décadi"));
}
