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
#include <string>
#include <vector>

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
	size_t m_size;
	size_t m_capacity;

	enum {
		LEFTSTRIP,
		BOTHSTRIP,
		RIGHTSTRIP,
	};

public:
	explicit pystring(const size_t size = 0);

	pystring(const size_t size, const char c);

	explicit pystring(const char *ch);

	pystring(const char *ch, size_t n);

	pystring(const pystring &s) = default;

	explicit pystring(const std::string &s);

	pystring(pystring &&other);

	~pystring();

	pystring &operator=(pystring rhs);

	pystring &operator=(char c);

	pystring &operator=(const char *ch);

	static constexpr size_t npos = -1ul;

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

	size_t size() const noexcept;

	size_t length() const noexcept;

	size_t capacity() const noexcept;

	size_t max_size() const noexcept;

	void resize(size_t n);

	void resize(size_t n, char c);

	void reserve(size_t n);

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

	size_t count(const pystring &sub, size_t start = -1ul, size_t end = -1ul);

	bool endswith(const pystring &suffix, size_t start = -1ul, size_t end = -1ul) const;

	size_t find(const pystring &sub, size_t pos = 0) const;

	size_t find(const char *sub, size_t pos = 0);

	size_t find(char c, size_t pos = 0);

	bool isalnum() const;

	bool isalpha() const;

	bool isdecimal() const;

	bool islower() const;

	bool isnumeric() const;

	bool isspace() const;

	bool istitle() const;

	bool isupper() const;

	const pystring &lower();

	pystring substr(size_t pos = 0, size_t n = npos) const;

	std::vector<pystring> split(const pystring &sep = pystring(""), int maxsplit = -1) const;

	std::vector<pystring> rsplit(const pystring &sep = pystring(""), int maxsplit = -1) const;

	std::vector<pystring> partition(const pystring &sep) const;

//	std::vector<pystring> rpartition(const pystring & sep) const
//	{
//		std::vector<pystring> result(3);

//		auto index = rfind(sep);
//		if (index == npos) {
//			result[0] = "";
//			result[1] = "";
//			result[2] = m_data;
//		}
//		else {
//			result[0] = substr(0, index);
//			result[1] = sep;
//			result[2] = substr(index + sep.size(), m_data);
//		}

//		return result;
//	}

	pystring join(const std::vector<pystring> &seq) const;

	std::vector<pystring> splitlines(const bool keepends = false);

	bool startswith(const pystring &suffix, size_t start = -1ul, size_t end = -1ul) const;

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

	pystring ljust(size_t width);

	pystring rjust(size_t width);

	pystring center(size_t width);


	pystring slice(size_t start, size_t end);

	size_t find(const pystring &sub, size_t start, size_t end) const;

	size_t index(const pystring &sub, size_t start, size_t end) const;

//	size_t rfind(const pystring &sub, size_t start, size_t end) const
//	{
//		ADJUST_INDICES(start, end, m_size);

//		size_t result = rfind(sub, end);

//		if (   result == npos
//		    || result < start
//		    || (result + sub.size() > end))
//			return -1;

//		return result;
//	}

//	size_t rindex(const pystring &sub, size_t start, size_t end) const
//	{
//		return rfind(sub, start, end);
//	}

//	pystring expandtabs(int tabsize) const
//	{
//		pystring s(*this);

//		int offset = 0;
//		int j = 0;

//		for (size_t i = 0; i < m_size; ++i) {
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

	size_t count(const pystring &substr, size_t start, size_t end) const;

//	pystring replace(const pystring &oldstr, const pystring &newstr, int count) const
//	{
//		int sofar = 0;
//		pystring s(*this);

//		auto oldlen = oldstr.size(), newlen = newstr.size();
//		auto cursor = s.find(oldstr, 0);

//		while (cursor != -1 && cursor <= (int)s.size()) {
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

	std::vector<pystring> splitlines(bool keepends) const;

	/* ******************* element access ******************** */

	char &operator[](size_t pos);
	char &operator[](size_t pos) const;

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

		for (size_t i(0); i < m_size; ++i) {
			if (!op(m_data[i])) {
				return false;
			}
		}

		return true;
	}

	inline void save_data_n(char *buffer, size_t n);

	std::vector<pystring> split_whitespace(int maxsplit = -1) const;

	void reverse_strings(std::vector<pystring> &result) const;

	std::vector<pystring> rsplit_whitespace(int maxsplit = -1) const;

	pystring do_strip(const pystring &chars, int striptype);

	bool _string_tailmatch(const pystring &self, const pystring &substr,
						  size_t start, size_t end, int direction) const;
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

void permute(std::ostream &os, std::string str, size_t pos);

// SPI pystring
#if 0

std::string mul(const std::string & str, int n)
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
