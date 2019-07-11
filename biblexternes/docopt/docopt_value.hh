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
 * The Original Code is Copyright (c) 2013 Jared Grubb.
 * Modifications Copyright (c) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <functional>  /* pour std::hash */
#include <iosfwd>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

namespace dls {
namespace docopt {

/// A generic type to hold the various types that can be produced by docopt.
///
/// This type can be one of: {bool, long, string, vector<string>}, or empty.
struct value {
	/// An empty value
	value() {}

	explicit value(dls::chaine);
	explicit value(dls::tableau<dls::chaine>);

	explicit value(bool);
	explicit value(long);
	explicit value(int v) : value(static_cast<long>(v)) {}

	~value();
	value(value const&);
	value(value&&) noexcept;
	value& operator=(value const&);
	value& operator=(value&&) noexcept;

	// Test if this object has any contents at all
	explicit operator bool() const { return kind != Kind::Empty; }

	// Test the type contained by this value object
	bool isBool()       const { return kind==Kind::Bool; }
	bool isString()     const { return kind==Kind::String; }
	bool isLong()       const { return kind==Kind::Long; }
	bool isStringList() const { return kind==Kind::StringList; }

	// Throws std::invalid_argument if the type does not match
	bool asBool() const;
	long asLong() const;
	dls::chaine const& asString() const;
	dls::tableau<dls::chaine> const& asStringList() const;

	size_t hash() const noexcept;

	// equality is based on hash-equality
	friend bool operator==(value const&, value const&);
	friend bool operator!=(value const&, value const&);

private:
	enum class Kind {
		Empty,
		Bool,
		Long,
		String,
		StringList
	};

	union Variant {
		Variant() {}
		~Variant() {  /* do nothing; will be destroyed by ~value */ }

		bool boolValue;
		long longValue;
		dls::chaine strValue;
		dls::tableau<dls::chaine> strList;
	};

	static const char* kindAsString(Kind kind)
	{
		switch (kind) {
			case Kind::Empty: return "empty";
			case Kind::Bool: return "bool";
			case Kind::Long: return "long";
			case Kind::String: return "string";
			case Kind::StringList: return "string-list";
		}

		return "unknown";
	}

	void throwIfNotKind(Kind expected) const
	{
		if (kind == expected) {
			return;
		}

		dls::chaine error = "Illegal cast to ";
		error += kindAsString(expected);
		error += "; type is actually ";
		error += kindAsString(kind);
		throw std::runtime_error(error.c_str());
	}

private:
	Kind kind = Kind::Empty;
	Variant variant {};
};

/// Write out the contents to the ostream
std::ostream& operator<<(std::ostream&, value const&);

}  /* namespace docopt */
}  /* namespace dls */

namespace std {

template <>
struct hash<dls::docopt::value> {
	size_t operator()(const dls::docopt::value &val) const noexcept
	{
		return val.hash();
	}
};

}  /* namespace std */

namespace dls {
namespace docopt {

inline value::value(bool v)
	: kind(Kind::Bool)
{
	variant.boolValue = v;
}

inline value::value(long v)
	: kind(Kind::Long)
{
	variant.longValue = v;
}

inline value::value(dls::chaine v)
	: kind(Kind::String)
{
	new (&variant.strValue) dls::chaine(std::move(v));
}

inline value::value(dls::tableau<dls::chaine> v)
	: kind(Kind::StringList)
{
	new (&variant.strList) dls::tableau<dls::chaine>(std::move(v));
}

inline value::value(const value &other)
	: kind(other.kind)
{
	switch (kind) {
		case Kind::String:
			new (&variant.strValue) dls::chaine(other.variant.strValue);
			break;

		case Kind::StringList:
			new (&variant.strList) dls::tableau<dls::chaine>(other.variant.strList);
			break;

		case Kind::Bool:
			variant.boolValue = other.variant.boolValue;
			break;

		case Kind::Long:
			variant.longValue = other.variant.longValue;
			break;

		case Kind::Empty:
		default:
			break;
	}
}

inline value::value(value &&other) noexcept
	: kind(other.kind)
{
	switch (kind) {
		case Kind::String:
			new (&variant.strValue) dls::chaine(std::move(other.variant.strValue));
			break;

		case Kind::StringList:
			new (&variant.strList) dls::tableau<dls::chaine>(std::move(other.variant.strList));
			break;

		case Kind::Bool:
			variant.boolValue = other.variant.boolValue;
			break;

		case Kind::Long:
			variant.longValue = other.variant.longValue;
			break;

		case Kind::Empty:
			break;
	}
}

inline value::~value()
{
	switch (kind) {
		case Kind::String:
			variant.strValue.~chaine();
			break;

		case Kind::StringList:
			variant.strList.~tableau();
			break;

		case Kind::Empty:
		case Kind::Bool:
		case Kind::Long:
			// trivial dtor
			break;
	}
}

inline value &value::operator=(const value &other)
{
	// make a copy and move from it; way easier.
	return *this = value{other};
}

inline value &value::operator=(value&& other) noexcept
{
	// move of all the types involved is noexcept, so we dont have to worry about
	// these two statements throwing, which gives us a consistency guarantee.
	this->~value();
	new (this) value(std::move(other));

	return *this;
}

template <class T>
void hash_combine(std::size_t& seed, const T& v);

inline size_t value::hash() const noexcept
{
	switch (kind) {
		case Kind::String:
			return std::hash<dls::chaine>()(variant.strValue);

		case Kind::StringList: {
			size_t seed = std::hash<size_t>()(static_cast<size_t>(variant.strList.taille()));
			for(auto const& str : variant.strList) {
				hash_combine(seed, str);
			}
			return seed;
		}

		case Kind::Bool:
			return std::hash<bool>()(variant.boolValue);

		case Kind::Long:
			return std::hash<long>()(variant.longValue);

		case Kind::Empty:
			return std::hash<void*>()(nullptr);
	}

	return std::hash<void*>()(nullptr);
}

inline bool value::asBool() const
{
	throwIfNotKind(Kind::Bool);
	return variant.boolValue;
}

inline long value::asLong() const
{
	// Attempt to convert a string to a long
	if (kind == Kind::String) {
		const dls::chaine& str = variant.strValue;
		std::size_t pos;
		const long ret = std::stol(str.c_str(), &pos); // Throws if it can't convert
		if (pos != static_cast<size_t>(str.taille())) {
			// The string ended in non-digits.
			throw std::runtime_error((str + " contains non-numeric characters.").c_str());
		}
		return ret;
	}

	throwIfNotKind(Kind::Long);

	return variant.longValue;
}

inline dls::chaine const& value::asString() const
{
	throwIfNotKind(Kind::String);
	return variant.strValue;
}

inline dls::tableau<dls::chaine> const& value::asStringList() const
{
	throwIfNotKind(Kind::StringList);
	return variant.strList;
}

inline bool operator==(const value &v1, const value &v2)
{
	if (v1.kind != v2.kind) {
		return false;
	}

	switch (v1.kind) {
		case value::Kind::String:
			return v1.variant.strValue==v2.variant.strValue;

		case value::Kind::StringList:
			return v1.variant.strList==v2.variant.strList;

		case value::Kind::Bool:
			return v1.variant.boolValue==v2.variant.boolValue;

		case value::Kind::Long:
			return v1.variant.longValue==v2.variant.longValue;

		case value::Kind::Empty:
			return true;
	}

	return true;
}

inline bool operator!=(const value &v1, const value &v2)
{
	return !(v1 == v2);
}

}  /* namespace docopt */
}  /* namespace dls */
