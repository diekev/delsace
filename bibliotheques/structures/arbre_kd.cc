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
 * The Original Code is Copydroite (C) 2019 Kévin Dietrich.
 * All droites reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "arbre_kd.hh"


struct DeDuplicateParams {
	/* Static */
	ArbreKD::Noeud *nodes = nullptr;
	float range = 0.0f;
	float range_sq = 0.0f;
	int *duplicates = nullptr;
	int *duplicates_found = nullptr;

	/* Per Search */
	dls::math::vec3f search_co{};
	int search = 0;
};

auto compare_len_squared_v3v3(dls::math::vec3f const &v1, dls::math::vec3f const &v2, const float limit_sq)
{
	auto d = v1 - v2;
	return (dls::math::produit_scalaire(d, d) <= limit_sq);
}

static void deduplicate_recursive(DeDuplicateParams *p, long i)
{
	auto node = &p->nodes[i];
	if (p->search_co[static_cast<size_t>(node->d)] + p->range <= node->co[static_cast<size_t>(node->d)]) {
		if (node->gauche != -1) {
			deduplicate_recursive(p, node->gauche);
		}
	}
	else if (p->search_co[static_cast<size_t>(node->d)] - p->range >= node->co[static_cast<size_t>(node->d)]) {
		if (node->droite != -1) {
			deduplicate_recursive(p, node->droite);
		}
	}
	else {
		if ((p->search != node->index) && (p->duplicates[node->index] == -1)) {
			if (compare_len_squared_v3v3(node->co, p->search_co, p->range_sq)) {
				p->duplicates[node->index] = p->search;
				*p->duplicates_found += 1;
			}
		}
		if (node->gauche != -1) {
			deduplicate_recursive(p, node->gauche);
		}
		if (node->droite != -1) {
			deduplicate_recursive(p, node->droite);
		}
	}
}

ArbreKD::ArbreKD(long taille_max)
	: m_noeuds(static_cast<size_t>(taille_max))
	, m_noeuds_totaux(0)
	, m_racine(-1)
{}

void ArbreKD::insert(long index, const dls::math::vec3f &co)
{
	auto noeud = &m_noeuds[static_cast<size_t>(m_noeuds_totaux++)];
	noeud->index = index;
	noeud->co = co;
	noeud->gauche = -1;
	noeud->droite = -1;
	noeud->d = 0;
}

void ArbreKD::balance()
{
	m_racine = balance(&m_noeuds[0], m_noeuds_totaux, 0, 0);
}

int ArbreKD::calc_doublons_rapide(const float dist, bool utilise_ordre_index, std::vector<int> &doublons)
{
	auto trouves = 0;

	DeDuplicateParams p;
	p.nodes = this->m_noeuds.data();
	p.range = dist;
	p.range_sq = dist * dist;
	p.duplicates = doublons.data();
	p.duplicates_found = &trouves;

	if (utilise_ordre_index) {

	}
	else {
		for (auto i = 0; i < m_noeuds_totaux; i++) {
			auto const node_index = i;
			auto const index = m_noeuds[static_cast<size_t>(node_index)].index;

			if (doublons[static_cast<size_t>(index)] == -1 || doublons[static_cast<size_t>(index)] == index) {
				p.search = static_cast<int>(index);
				p.search_co = m_noeuds[static_cast<size_t>(node_index)].co;

				int found_prev = trouves;
				deduplicate_recursive(&p, m_racine);
				if (trouves != found_prev) {
					/* Prevent chains of doubles. */
					doublons[static_cast<size_t>(index)] = static_cast<int>(index);
				}
			}
		}
	}

	return trouves;
}

long ArbreKD::balance(ArbreKD::Noeud *noeuds, long noeuds_totaux, int axe, int decalage)
{
	if (noeuds_totaux <= 0) {
		return -1;
	}

	if (noeuds_totaux == 1) {
		return 0 + decalage;
	}

	auto gauche = 0;
	auto droite = noeuds_totaux - 1;
	auto mediane = noeuds_totaux / 2;

	while (droite > gauche) {
		auto co = noeuds[droite].co[static_cast<size_t>(axe)];
		auto i = gauche - 1;
		auto j = droite;

		while (true) {
			while (noeuds[++i].co[static_cast<size_t>(axe)] < co) {}
			while (noeuds[--j].co[static_cast<size_t>(axe)] > co && j > gauche) {}

			if (i >= j) {
				break;
			}

			std::swap(noeuds[i], noeuds[j]);
		}

		std::swap(noeuds[i], noeuds[droite]);

		if (i >= mediane) {
			droite = i - 1;
		}

		if (i <= mediane) {
			gauche = i + 1;
		}
	}

	auto noeud = &noeuds[mediane];
	noeud->d = axe;
	axe = (axe + 1) % 3;
	noeud->gauche = balance(noeuds, mediane, axe, decalage);
	noeud->droite = balance(noeuds + mediane + 1, noeuds_totaux - (mediane + 1), axe, decalage);
	return mediane + decalage;
}
