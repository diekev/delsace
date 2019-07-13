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

#include "language.h"

#include <algorithm>

static auto replace_substr(dls::chaine &str, const dls::chaine &substr, const dls::chaine &rep)
{
	auto index = 0l;
	while (true) {
	     /* Locate the substring to replace. */
		 index = str.trouve(substr, index);

		 if (index == dls::chaine::npos) {
			 break;
		 }

	     /* Make the replacement. */
		 str.remplace(index, substr.taille(), rep);

	     /* Advance index forward so the next iteration doesn't pick it up as well. */
	     index += rep.taille();
	}
}

/* ************************************************************************** */

Language::Language(dls::chaine name)
    : Language()
{
	m_name = std::move(name);
}

Language::Ptr Language::create(dls::chaine name)
{
	return Ptr(new Language(name));
}

void Language::parent(Language * const parent)
{
	this->m_parents.pousse(parent);
}

const Language *Language::parent() const noexcept
{
	if (m_parents.est_vide()) {
		return nullptr;
	}

	return m_parents[0];
}

const Language *Language::parent(const dls::chaine &name) const noexcept
{
	auto iter = std::find_if(m_parents.debut(), m_parents.fin(),
	                         [&name](const Language *language) -> bool
	{
		return language->name() == name;
	});

	return *iter;
}

void Language::name(const dls::chaine &n)
{
	m_name = n;
}

const dls::chaine &Language::name() const noexcept
{
	return m_name;
}

void Language::setTable(const table_t &table)
{
	m_table = table;
}

dls::chaine Language::translate(const dls::chaine &word) const
{
	auto mot = word;

	for (const auto &pair : m_table) {
		replace_substr(mot, pair.first, pair.second);
	}

	return mot;
}

/* ************************************************************************** */

void LangageTree::addLanguage(const Language::Ptr &langage)
{
	m_langages.pousse(langage);
}

void LangageTree::connect(const Language::Ptr &child, const Language::Ptr &parent)
{
	child->parent(parent.get());
}

void LangageTree::walk(dls::chaine &word) const
{
	const auto langage = m_langages[0].get();
	walk(langage, word);
}

void LangageTree::walk(const Language * const node, dls::chaine &word) const
{
	if (!node) {
		return;
	}

	word = node->translate(word);

	walk(node->parent(), word);
}
