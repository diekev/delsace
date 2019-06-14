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

#include <memory>
#include <string>
#include <vector>

using sound_pair = std::pair<const char *, const char *>;
using table_t = std::vector<sound_pair>;

class Language {
	std::vector<Language *> m_parents{};
	std::vector<Language *> m_children{};
	std::string m_family = "";
	std::string m_name = "";
	table_t m_table{};

public:
	Language() = default;

	using Ptr = std::shared_ptr<Language>;

	explicit Language(std::string name);

	static Ptr create(std::string name);

	void parent(Language * const parent);

	const Language *parent() const noexcept;

	const Language *parent(const std::string &name) const noexcept;

	void name(const std::string &n);

	const std::string &name() const noexcept;

	void setTable(const table_t &table);

	std::string translate(const std::string &word) const;
};

class LangageTree {
	std::vector<Language::Ptr> m_langages{};

public:
	LangageTree() = default;
	~LangageTree() = default;

	void addLanguage(const Language::Ptr &langage);

	void connect(const Language::Ptr &child, const Language::Ptr &parent);

	void walk(std::string &word) const;

private:
	void walk(const Language * const node, std::string &word) const;
};
