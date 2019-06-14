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

static auto replace_substr(std::string &str, const std::string &substr, const std::string &rep)
{
	size_t index = 0;
	while (true) {
	     /* Locate the substring to replace. */
	     index = str.find(substr, index);

	     if (index == std::string::npos)
			 break;

	     /* Make the replacement. */
	     str.replace(index, substr.size(), rep);

	     /* Advance index forward so the next iteration doesn't pick it up as well. */
	     index += rep.size();
	}
}

/* ************************************************************************** */

Language::Language(std::string name)
    : Language()
{
	m_name = std::move(name);
}

Language::Ptr Language::create(std::string name)
{
	return Ptr(new Language(name));
}

void Language::parent(Language * const parent)
{
	this->m_parents.push_back(parent);
}

const Language *Language::parent() const noexcept
{
	if (m_parents.empty()) {
		return nullptr;
	}

	return m_parents[0];
}

const Language *Language::parent(const std::string &name) const noexcept
{
	auto iter = std::find_if(m_parents.begin(), m_parents.end(),
	                         [&name](const Language *language) -> bool
	{
		return language->name() == name;
	});

	return *iter;
}

void Language::name(const std::string &n)
{
	m_name = n;
}

const std::string &Language::name() const noexcept
{
	return m_name;
}

void Language::setTable(const table_t &table)
{
	m_table = table;
}

std::string Language::translate(const std::string &word) const
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
	m_langages.push_back(langage);
}

void LangageTree::connect(const Language::Ptr &child, const Language::Ptr &parent)
{
	child->parent(parent.get());
}

void LangageTree::walk(std::string &word) const
{
	const auto langage = m_langages[0].get();
	walk(langage, word);
}

void LangageTree::walk(const Language * const node, std::string &word) const
{
	if (!node) {
		return;
	}

	word = node->translate(word);

	walk(node->parent(), word);
}
