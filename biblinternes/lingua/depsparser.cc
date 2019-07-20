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

#include "depsparser.h"

#include <cassert>
#include <iostream>

/* Algorithms derived from
 * "A Fundamental Algorithm for Dependency Parsing", Michael A. Covington, 2001
 */

void link(word *head, word *dependent)
{
	assert(dependent->head == nullptr);

	head->dependents.pousse(dependent);
	dependent->head = head;
}

static bool grammar_permits(word */*head*/, word */*dependent*/)
{
	/* TODO */
	return false;
}

/* Exhaustive left-to-right search, heads first */
void esh_heads_first(const dls::tableau<word *> &sentence)
{
	auto begin = sentence.debut();
	auto first = sentence.debut() + 1;
	auto last = sentence.fin();

	for (; first != last; ++first) {
		for (auto second = first - 1; second != begin; --second) {
			if (grammar_permits(*second, *first)) {
				link(*second, *first);
			}

			if (grammar_permits(*first, *second)) {
				link(*first, *second);
			}
		}
	}
}

/* Exhaustive left-to-right search, dependents first */
void esh_dependents_first(const dls::tableau<word *> &sentence)
{
	auto begin = sentence.debut();
	auto first = sentence.debut() + 1;
	auto last = sentence.fin();

	for (; first != last; ++first) {
		for (auto second = first - 1; second != begin; --second) {
			if (grammar_permits(*first, *second)) {
				link(*first, *second);
			}

			if (grammar_permits(*second, *first)) {
				link(*second, *first);
			}
		}
	}
}

/* Exhaustive left-to-right search, heads first, with uniqueness */
void esh_heads_first_unique(const dls::tableau<word *> &sentence)
{
	auto begin = sentence.debut();
	auto first = sentence.debut() + 1;
	auto last = sentence.fin();

	for (; first != last; ++first) {
		word *wi = *first;

		for (auto second = first - 1; second != begin; --second) {
			if (wi->head == nullptr) {
				if (grammar_permits(*second, *first)) {
					link(*second, *first);
				}
			}

			word *wj = *second;

			if (wj->dependents.est_vide()) {
				if (grammar_permits(*first, *second)) {
					link(*first, *second);
				}
			}
		}
	}
}

/* Exhaustive left-to-right search, dependents first, with uniqueness */
void esh_dependents_first_unique(const dls::tableau<word *> &sentence)
{
	auto begin = sentence.debut();
	auto first = sentence.debut() + 1;
	auto last = sentence.fin();

	for (; first != last; ++first) {
		word *wi = *first;

		for (auto second = first - 1; second != begin; --second) {
			word *wj = *second;

			if (wj->dependents.est_vide()) {
				if (grammar_permits(*first, *second)) {
					link(*first, *second);
				}
			}

			if (wi->head == nullptr) {
				if (grammar_permits(*second, *first)) {
					link(*second, *first);
				}
			}
		}
	}
}

static bool can_depend(word *head, word *dependent)
{
	/* object/subject and verb */
	if (dependent->cat == category::noun && head->cat == category::verb) {
		/* subject case */
		if (dependent->cas == gcase::nominative) {
			return     (dependent->num == head->num)
			        && (dependent->person == head->person);
		}

		/* object case */
		if (dependent->cas == gcase::accusative) {
			return true;
		}

		return false;
	}

	if (dependent->cat == category::adjective && head->cat == category::noun) {
		return     (dependent->num == head->num)
		        && (dependent->cas == head->cas)
		        && (dependent->gend == head->gend);
	}

	return false;
}

static bool is_independent(word */*head*/)
{
	return true;
}

/* List-based search with uniqueness and projectivity */
void lsup(const dls::tableau<word *> &sentence)
{
	dls::tableau<word *> head_list{};
	dls::tableau<word *> word_list{};

	auto first = sentence.debut() + 1;
	auto last = sentence.fin();

	for (; first != last; ++first) {
		word_list.pousse(*first);

		/* look for dependents of w, they can only be the consecutive elements
		 * of head_list, starting with the most recently added */
		for (auto iter = head_list.debut(); iter != head_list.fin(); ++iter) {
			if (can_depend(*first, *iter)) {
				link(*first, *iter);
				head_list.erase(iter);

				continue;
			}

			break;
		}

		/* look for the head of w, it must comprise the word preceeding w */

		word *head = *(first - 1);

		while (head != nullptr) {
			if (can_depend(head, *first)) {
				link(head, *first);
				break;
			}

			if (is_independent(head)) {
				break;
			}

			head = head->head;
		}

		if ((*first)->head == nullptr) {
			head_list.pousse(*first);
		}
	}

	std::cerr << "Head list size: " << head_list.taille() << '\n';
}
