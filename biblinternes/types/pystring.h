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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "../outils/definitions.h"
#include "../outils/iterateurs.h"

A_FAIRE("finir l'implémentation")

namespace dls {
namespace types {

// forward decls
class pystring;

pystring operator+(const pystring &str1, const pystring &str2);
bool operator==(const pystring &str1, const pystring &str2);

#define ADJUST_INDICES(start, end, len)         \
    if (end > len)                          \
        end = len;                          \
    else if (end == pystring::npos) {                     \
        end = len;                         \
    }                                       \
    if (start == pystring::npos) {                        \
        start = 0;                       \
    }

class pystring {
	char  *m_data;
	long m_size;
	long m_capacity;

	enum {
		LEFTSTRIP,
		BOTHSTRIP,
		RIGHTSTRIP,
	};

public:
	explicit pystring(const long size = 0);

	pystring(const long size, const char c);

	explicit pystring(const char *ch);

	pystring(const char *ch, long n);

	pystring(const pystring &s) = default;

	explicit pystring(const dls::chaine &s);

	pystring(pystring &&other);

	~pystring();

	pystring &operator=(pystring rhs);

	pystring &operator=(char c);

	pystring &operator=(const char *ch);

	static constexpr long npos = -1l;

	/* ******************* iterators ******************** */

	using iterator = outils::iterateur_normal<char>;
	using const_iterator = outils::iterateur_normal<const char>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	iterator begin() noexcept;

	const_iterator begin() const noexcept;

	const_iterator cbegin() const noexcept;

	iterator end() noexcept;

	const_iterator end() const noexcept;

	const_iterator cend() const noexcept;

	reverse_iterator rbegin() noexcept;

	const_reverse_iterator rbegin() const noexcept;

	const_reverse_iterator crbegin() const noexcept;

	reverse_iterator rend() noexcept;

	const_reverse_iterator rend() const noexcept;

	const_reverse_iterator crend() const noexcept;

	/* ******************* capacity ******************** */

	long taille() const noexcept;

	long length() const noexcept;

	long capacity() const noexcept;

	long max_size() const noexcept;

	void resize(long n);

	void resize(long n, char c);

	void reserve(long n);

	bool empty() const;

	void clear();

	void shrink_to_fit();

	/* ******************* methods ******************** */

	const char *c_str() const noexcept;
	const char *data()  const noexcept;

	int compare(const pystring &s) const;

	int compare(const char *rhs) const;

	void push_back(char c);

	const pystring &capitalize();

	long count(const pystring &sub, long start = -1l, long end = -1l);

	bool endswith(const pystring &suffix, long start = -1l, long end = -1l) const;

	long find(const pystring &sub, long pos = 0) const;

	long find(const char *sub, long pos = 0);

	long find(char c, long pos = 0);

	bool isalnum() const;

	bool isalpha() const;

	bool isdecimal() const;

	bool islower() const;

	bool isnumeric() const;

	bool isspace() const;

	bool istitle() const;

	bool isupper() const;

	const pystring &lower();

	pystring substr(long pos = 0, long n = npos) const;

	dls::tableau<pystring> split(const pystring &sep = pystring(""), int maxsplit = -1) const;

	dls::tableau<pystring> rsplit(const pystring &sep = pystring(""), int maxsplit = -1) const;

	dls::tableau<pystring> partition(const pystring &sep) const;

//	dls::tableau<pystring> rpartition(const pystring & sep) const
//	{
//		dls::tableau<pystring> result(3);

//		auto index = rfind(sep);
//		if (index == npos) {
//			result[0] = "";
//			result[1] = "";
//			result[2] = m_data;
//		}
//		else {
//			result[0] = substr(0, index);
//			result[1] = sep;
//			result[2] = substr(index + sep.taille(), m_data);
//		}

//		return result;
//	}

	pystring join(const dls::tableau<pystring> &seq) const;

	dls::tableau<pystring> splitlines(const bool keepends = false);

	bool startswith(const pystring &suffix, long start = -1l, long end = -1l) const;

	pystring strip(const pystring &chars);

	pystring rstrip(const pystring &chars);

	pystring lstrip(const pystring &chars);

	// return copy where case is swapped
	const pystring &swapcase();

	// return a titlecased version of the string where words start with an
	// uppercase character and the remaining characters are lowercase
	const pystring &title();

	const pystring &upper();

	pystring zfill(unsigned width) const;

	pystring ljust(long width);

	pystring rjust(long width);

	pystring center(long width);


	pystring slice(long start, long end);

	long find(const pystring &sub, long start, long end) const;

	long index(const pystring &sub, long start, long end) const;

//	long rfind(const pystring &sub, long start, long end) const
//	{
//		ADJUST_INDICES(start, end, m_size);

//		long result = rfind(sub, end);

//		if (   result == npos
//		    || result < start
//		    || (result + sub.taille() > end))
//			return -1;

//		return result;
//	}

//	long rindex(const pystring &sub, long start, long end) const
//	{
//		return rfind(sub, start, end);
//	}

//	pystring expandtabs(int tabsize) const
//	{
//		pystring s(*this);

//		int offset = 0;
//		int j = 0;

//		for (long i = 0; i < m_size; ++i) {
//			if (m_data[i] == '\t') {
//				if (tabsize > 0) {
//					int fillsize = tabsize - (j % tabsize);
//					j += fillsize;
//					s.replace(i + offset, 1, pystring(fillsize, ' '));
//					offset += fillsize - 1;
//				}
//				else {
//					s.replace(i + offset, 1, "");
//					offset -= 1;
//				}
//			}
//			else {
//				j++;

//				if (m_data[i] == '\n' || m_data[i] == '\r') {
//					j = 0;
//				}
//			}
//		}

//		return s;
//	}

	long count(const pystring &substr, long start, long end) const;

//	pystring replace(const pystring &oldstr, const pystring &newstr, int count) const
//	{
//		int sofar = 0;
//		pystring s(*this);

//		auto oldlen = oldstr.taille(), newlen = newstr.taille();
//		auto cursor = s.find(oldstr, 0);

//		while (cursor != -1 && cursor <= (int)s.taille()) {
//			if (count > -1 && sofar >= count) {
//				break;
//			}

//			s.replace(cursor, oldlen, newstr);
//			cursor += newlen;

//			if (oldlen != 0) {
//				cursor = find(s, oldstr, cursor);
//			}
//			else {
//				++cursor;
//			}

//			++sofar;
//		}

//		return s;
//	}

	dls::tableau<pystring> splitlines(bool keepends) const;

	/* ******************* element access ******************** */

	char &operator[](long pos);
	char &operator[](long pos) const;

	char &front();
	char &front() const noexcept;
	char &back();
	char &back() const  noexcept;

	friend void swap(pystring &first, pystring &second);

	pystring &append(const pystring &other);

	pystring &operator+=(const pystring &other);

	pystring &operator+=(char c);

private:
	void copy_data(const char *ch);

	template <typename BooleanOp>
	bool check_string(BooleanOp &&op) const noexcept
	{
		if (empty()) {
			return false;
		}

		for (long i(0); i < m_size; ++i) {
			if (!op(m_data[i])) {
				return false;
			}
		}

		return true;
	}

	inline void save_data_n(char *buffer, long n);

	dls::tableau<pystring> split_whitespace(int maxsplit = -1) const;

	void reverse_strings(dls::tableau<pystring> &result) const;

	dls::tableau<pystring> rsplit_whitespace(int maxsplit = -1) const;

	pystring do_strip(const pystring &chars, int striptype);

	bool _string_tailmatch(const pystring &self, const pystring &substr,
						  long start, long end, int direction) const;
};

inline pystring operator+(const pystring &lhs, const pystring &rhs)
{
	return pystring(lhs).append(rhs);
}

inline pystring operator+(pystring &&lhs, pystring &&rhs)
{
	return std::move(lhs.append(rhs));
}

inline bool operator==(const pystring &lhs, const pystring &rhs)
{
	return (lhs.compare(rhs) == 0);
}

inline bool operator==(const pystring &lhs, const char *rhs)
{
	return (lhs.compare(rhs) == 0);
}

inline bool operator==(const char *lhs, const pystring &rhs)
{
	return (rhs.compare(lhs) == 0);
}

inline bool operator!=(const pystring &lhs, const pystring &rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const pystring &lhs, const char *rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const char *lhs, const pystring &rhs)
{
	return !(lhs == rhs);
}

inline bool operator<(const pystring &lhs, const pystring &rhs)
{
	return (lhs.compare(rhs) < 0);
}

inline bool operator<(const pystring &lhs, const char *rhs)
{
	return (lhs.compare(rhs) < 0);
}

inline bool operator<(const char *lhs, const pystring &rhs)
{
	return (rhs.compare(lhs) < 0);
}

inline bool operator<=(const pystring &lhs, const pystring &rhs)
{
	return (lhs < rhs) || (lhs == rhs);
}

inline bool operator<=(const pystring &lhs, const char *rhs)
{
	return (lhs < rhs) || (lhs == rhs);
}

inline bool operator<=(const char *lhs, const pystring &rhs)
{
	return (lhs < rhs) || (lhs == rhs);
}

inline bool operator>(const pystring &lhs, const pystring &rhs)
{
	return (lhs.compare(rhs) > 0);
}

inline bool operator>(const pystring &lhs, const char *rhs)
{
	return (lhs.compare(rhs) > 0);
}

inline bool operator>(const char *lhs, const pystring &rhs)
{
	return (rhs.compare(lhs) > 0);
}

inline bool operator>=(const pystring &lhs, const pystring &rhs)
{
	return (lhs > rhs) || (lhs == rhs);
}

inline bool operator>=(const pystring &lhs, const char *rhs)
{
	return (lhs > rhs) || (lhs == rhs);
}

inline bool operator>=(const char *lhs, const pystring &rhs)
{
	return (lhs > rhs) || (lhs == rhs);
}

std::ostream &operator<<(std::ostream &os, const pystring &str);

void permute(std::ostream &os, dls::chaine str, long pos);

// SPI pystring
#if 0

dls::chaine mul(const dls::chaine & str, int n)
{
	// Early exits
	if (n <= 0) return "";
	if (n == 1) return str;

	std::ostringstream os;
	for(int i=0; i<n; ++i) {
		os << str;
	}

	return os.str();
}
#endif

}  /* namespace types */
}  /* namespace dls */
