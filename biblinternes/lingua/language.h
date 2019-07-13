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
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

using sound_pair = std::pair<const char *, const char *>;
using table_t = dls::tableau<sound_pair>;

class Language {
	dls::tableau<Language *> m_parents{};
	dls::tableau<Language *> m_children{};
	dls::chaine m_family = "";
	dls::chaine m_name = "";
	table_t m_table{};

public:
	Language() = default;

	using Ptr = std::shared_ptr<Language>;

	explicit Language(dls::chaine name);

	static Ptr create(dls::chaine name);

	void parent(Language * const parent);

	const Language *parent() const noexcept;

	const Language *parent(const dls::chaine &name) const noexcept;

	void name(const dls::chaine &n);

	const dls::chaine &name() const noexcept;

	void setTable(const table_t &table);

	dls::chaine translate(const dls::chaine &word) const;
};

class LangageTree {
	dls::tableau<Language::Ptr> m_langages{};

public:
	LangageTree() = default;
	~LangageTree() = default;

	void addLanguage(const Language::Ptr &langage);

	void connect(const Language::Ptr &child, const Language::Ptr &parent);

	void walk(dls::chaine &word) const;

private:
	void walk(const Language * const node, dls::chaine &word) const;
};
