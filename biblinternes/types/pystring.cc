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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "pystring.h"

namespace dls {
namespace types {

pystring::pystring(const size_t size)
	: m_data((size == 0) ? nullptr : new char[size + 1])
	, m_size(size)
	, m_capacity(size)
{}

pystring::pystring(const size_t size, const char c)
	: pystring(size)
{
	std::fill(begin(), end(), c);
	m_data[size] = '\0';
}

pystring::pystring(const char *ch)
	: pystring(strlen(ch))
{
	if (m_size != 0) {
		copy_data(ch);
	}
}

pystring::pystring(const char *ch, size_t n)
	: pystring()
{
	if (strlen(ch) < n) {
		throw std::out_of_range("");
	}

	m_data = new char[n + 1];
	m_size = n;
	m_capacity = n;

	copy_data(ch);
}

pystring::pystring(const std::string &s)
	: pystring(s.size())
{
	if (m_size != 0) {
		copy_data(s.c_str());
	}
}

pystring::pystring(pystring &&other)
	: pystring()
{
	swap(*this, other);
}

pystring::~pystring()
{
	delete [] m_data;
}

pystring &pystring::operator=(pystring rhs)
{
	swap(*this, rhs);
	return *this;
}

pystring &pystring::operator=(char c)
{
	return (*this = pystring(1, c));
}

pystring &pystring::operator=(const char *ch)
{
	return (*this = pystring(ch));
}

pystring::iterator pystring::begin() noexcept
{
	return iterator(&m_data[0]);
}

pystring::const_iterator pystring::begin() const noexcept
{
	return const_iterator(&m_data[0]);
}

pystring::const_iterator pystring::cbegin() const noexcept
{
	return const_iterator(&m_data[0]);
}

pystring::iterator pystring::end() noexcept
{
	return iterator(&m_data[m_size]);
}

pystring::const_iterator pystring::end() const noexcept
{
	return const_iterator(&m_data[m_size]);
}

pystring::const_iterator pystring::cend() const noexcept
{
	return const_iterator(&m_data[m_size]);
}

pystring::reverse_iterator pystring::rbegin() noexcept
{
	return reverse_iterator(this->end());
}

pystring::const_reverse_iterator pystring::rbegin() const noexcept
{
	return const_reverse_iterator(this->end());
}

pystring::const_reverse_iterator pystring::crbegin() const noexcept
{
	return const_reverse_iterator(this->cend());
}

pystring::reverse_iterator pystring::rend() noexcept
{
	return reverse_iterator(this->begin());
}

pystring::const_reverse_iterator pystring::rend() const noexcept
{
	return const_reverse_iterator(this->begin());
}

pystring::const_reverse_iterator pystring::crend() const noexcept
{
	return const_reverse_iterator(this->cbegin());
}

size_t pystring::size() const noexcept
{
	return m_size;
}

size_t pystring::length() const noexcept
{
	return m_size;
}

size_t pystring::capacity() const noexcept
{
	return m_capacity;
}

size_t pystring::max_size() const noexcept
{
	return std::numeric_limits<size_t>::max();
}

void pystring::resize(size_t n)
{
	this->resize(n, '\0');
}

void pystring::resize(size_t n, char c)
{
	if (n == m_size) {
		return;
	}

	char *buffer = new char[n + 1];
	auto min_size = std::min(n, m_size);
	auto max_size = std::max(n, m_size);

	if (m_data != nullptr) {
		save_data_n(buffer, min_size);
	}

	for (size_t i(min_size); i < max_size; ++i) {
		buffer[i] = c;
	}

	buffer[n] = '\0';

	m_data = buffer;
	m_size = n;
	m_capacity = std::max(m_size, m_capacity);
}

void pystring::reserve(size_t n)
{
	if (n <= m_capacity) {
		return;
	}

	char *buffer = new char[n + 1];

	if (m_data != nullptr) {
		save_data_n(buffer, m_size);
	}

	buffer[m_size] = '\0';

	m_data = buffer;
	m_capacity = n;
}

bool pystring::empty() const
{
	return !bool(m_size);
}

void pystring::clear()
{
	delete [] m_data;
	m_data = nullptr;
	m_size = m_capacity = 0;
}

const char *pystring::c_str() const noexcept
{
	return m_data;
}

const char *pystring::data() const noexcept
{
	return m_data;
}

int pystring::compare(const pystring &s) const
{
	return strcmp(m_data, s.data());
}

int pystring::compare(const char *rhs) const
{
	return strcmp(m_data, rhs);
}

void pystring::push_back(char c)
{
	if (m_size < m_capacity) {
		m_data[m_size] = c;
		m_data[m_size + 1] = '\0';
		m_size += 1;
		return;
	}

	char *buffer = new char[m_size + 2];

	if (m_data != nullptr) {
		save_data_n(buffer, m_size);
	}

	buffer[m_size] = c;
	buffer[m_size + 1] = '\0';

	m_data = buffer;
	m_size += 1;
	m_capacity += 1;
}

const pystring &pystring::capitalize()
{
	std::transform(begin(), end(), begin(), ::tolower);
	m_data[0] = static_cast<char>(toupper(m_data[0]));
	return *this;
}

size_t pystring::count(const pystring &sub, size_t start, size_t end)
{
	if (m_size == 0 || sub.length() == 0) {
		return 0;
	}

	const auto st = ((start == npos) ? 0 : start);
	const auto en = ((end == npos) ? m_size : end);

	auto count = 0ul;

	for (size_t i = st; i < en; ++i) {
		auto idx = find(sub, i);

		if (idx != npos) {
			++count;
			i = idx + (sub.length() - 1);
		}
	}

	return count;
}

bool pystring::endswith(const pystring &suffix, size_t start, size_t end) const
{
	return _string_tailmatch(*this, suffix, start, end, -1);
}

size_t pystring::find(const pystring &sub, size_t pos) const
{
	if (sub.size() > m_size) {
		return npos;
	}

	for (size_t i = pos, ie = m_size; i < ie; ++i) {
		if (m_data[i] != sub[0]) {
			continue;
		}

		auto subcount = 1ul;

		for (size_t j = 1, je = sub.size(); j < je; ++j, ++subcount) {
			if (m_data[i + j] != sub[j]) {
				break;
			}
		}

		if (subcount == sub.length()) {
			return i;
		}
	}

	return npos;
}

size_t pystring::find(const char *sub, size_t pos)
{
	return find(pystring(sub), pos);
}

size_t pystring::find(char c, size_t pos)
{
	for (size_t i = pos; i < m_size; ++i) {
		if (m_data[i] == c) {
			return i;
		}
	}

	return npos;
}

bool pystring::isalnum() const
{
	return check_string(::isalnum);
}

bool pystring::isalpha() const
{
	return check_string(::isalpha);
}

bool pystring::isdecimal() const
{
	return check_string(::isdigit);
}

bool pystring::islower() const
{
	return check_string(::islower);
}

bool pystring::isnumeric() const
{
	return check_string(::isdigit);
}

bool pystring::isspace() const
{
	return check_string(::isspace);
}

bool pystring::isupper() const
{
	return check_string(::isupper);
}

const pystring &pystring::lower()
{
	std::transform(begin(), end(), begin(), ::tolower);
	return *this;
}

pystring pystring::substr(size_t pos, size_t n) const
{
	if (pos > size()) {
		throw std::out_of_range("");
	}

	auto rlen = std::min(n, size() - pos);
	return pystring(data() + pos, rlen);
}

std::vector<pystring> pystring::split(const pystring &sep, int maxsplit) const
{
	std::vector<pystring> v;

	if (sep.size() == 0) {
		return split_whitespace(maxsplit);
	}

	auto i = size_t(0), j = size_t(0), n = sep.size();

	while (i + n < m_size) {
		if (m_data[i] == sep[0] && (substr(i, n) == sep)) {
			if (v.size() == maxsplit) {
				break;
			}

			v.push_back(substr(j, i - j));
			i = j = i + n;
		}
		else {
			++i;
		}
	}

	return v;
}

std::vector<pystring> pystring::rsplit(const pystring &sep, int maxsplit) const
{
	if (maxsplit < 0) {
		return split(sep, maxsplit);
	}

	std::vector<pystring> result;

	if (sep.size() == 0) {
		return rsplit_whitespace(maxsplit);
	}

	size_t i,j, n = sep.size();

	i = j = m_size;

	while (i >= n) {
		if (m_data[i - 1] == m_data[n - 1] && substr(i - n, n) == sep) {
			if (result.size() == static_cast<size_t>(maxsplit)) {
				break;
			}

			result.push_back(substr(i, j - i));
			i = j = i - n;
		}
		else {
			i--;
		}
	}

	result.push_back(substr(0, j));
	reverse_strings(result);

	return result;
}

std::vector<pystring> pystring::partition(const pystring &sep) const
{
	std::vector<pystring> result(3);

	auto index = find(sep);
	if (index == npos) {
		result[0] = m_data;
		result[1] = "";
		result[2] = "";
	}
	else {
		result[0] = substr(0, index);
		result[1] = sep;
		result[2] = substr(index + sep.size(), m_size);
	}

	return result;
}

pystring pystring::join(const std::vector<pystring> &seq) const
{
	size_t seqlen = seq.size(), i;
	pystring sep(this->m_data);

	if (seqlen == 0) return sep;
	if (seqlen == 1) return seq[0];

	pystring result(seq[0]);

	for (i = 1; i < seqlen; ++i) {
		result += sep;
		result += seq[i];
	}

	return result;
}

bool pystring::startswith(const pystring &suffix, size_t start, size_t end) const
{
	return _string_tailmatch(*this, suffix, start, end, 1);
}

pystring pystring::strip(const pystring &chars)
{
	return do_strip(chars, BOTHSTRIP);
}

pystring pystring::rstrip(const pystring &chars)
{
	return do_strip(chars, RIGHTSTRIP);
}

pystring pystring::lstrip(const pystring &chars)
{
	return do_strip(chars, LEFTSTRIP);
}

const pystring &pystring::swapcase()
{
	std::transform(begin(), end(), begin(), [](char ch) -> char
	{
		if (::isupper(ch)) {
			return static_cast<char>(::tolower(ch));
		}

		if (::islower(ch)) {
			return static_cast<char>(::toupper(ch));
		}

		return ch;
	});

	return *this;
}

const pystring &pystring::title()
{
	if (m_size == 0) {
		return *this;
	}

	m_data[0] = static_cast<char>(::toupper(m_data[0]));

	for (auto it = (begin() + 1), ie = end(); it != ie; ++it) {
		if (!::isalpha(*(it - 1)) && ::islower(*it)) {
			*it = static_cast<char>(::toupper(*it));
		}
	}

	return *this;
}

bool pystring::istitle() const
{
	if (m_size == 0) return false;
	if (m_size == 1) return ::isupper(m_data[0]);

	auto cased = false, previous_is_cased = false;

	for (auto i = size_t(0); i < m_size; ++i) {
		if (::isupper(m_data[i])) {
			if (previous_is_cased) {
				return false;
			}

			previous_is_cased = true;
			cased = true;
		}
		else if (::islower(m_data[i])) {
			if (!previous_is_cased) {
				return false;
			}

			previous_is_cased = true;
			cased = true;
		}
		else {
			previous_is_cased = false;
		}
	}

	return cased;
}

const pystring &pystring::upper()
{
	std::transform(begin(), end(), begin(), ::toupper);
	return *this;
}

pystring pystring::zfill(unsigned width) const
{
	if (m_size >= width) {
		return *this;
	}

	auto fill = width - m_size;

	auto s = pystring(fill, '0');
	s += (*this);

	if (s[fill] == '+' || s[fill] == '-') {
		s[0] = s[fill];
		s[fill] = '0';
	}

	return s;
}

pystring pystring::ljust(size_t width)
{
	if (m_size >= width)
		return *this;

	return *this + pystring(width - m_size, ' ');
}

pystring pystring::rjust(size_t width)
{
	if (m_size >= width)
		return *this;

	return pystring(width - m_size, ' ') + *this;
}

pystring pystring::center(size_t width)
{
	if (m_size >= width)
		return *this;

	const auto marg = width - m_size;
	const auto left = marg / 2 + (marg & width & 1);

	return pystring(left, ' ') + *this + pystring(marg - left, ' ');
}

pystring pystring::slice(size_t start, size_t end)
{
	ADJUST_INDICES(start, end, m_size);

	if (start >= end)
		return pystring("");

	return substr(start, end - start);
}

size_t pystring::find(const pystring &sub, size_t start, size_t end) const
{
	ADJUST_INDICES(start, end, m_size);

	size_t result = find(sub, start);

	// If we cannot find the string, or if the end-point of our found substring is past
	// the allowed end limit, return that it can't be found.
	if (    result == npos
			|| (result + sub.size() > end))
	{
		return -1ul;
	}

	return result;
}

size_t pystring::index(const pystring &sub, size_t start, size_t end) const
{
	return find(sub, start, end);
}

size_t pystring::count(const pystring &substr, size_t start, size_t end) const
{
	auto nummatches = 0ul;
	auto cursor = start;

	while (true) {
		cursor = find(substr, cursor, end);

		if (cursor == npos)
			break;

		cursor += substr.size();
		++nummatches;
	}

	return nummatches;
}

std::vector<pystring> pystring::splitlines(bool keepends) const
{
	std::vector<pystring> result;
	size_t i, j;

	for (i = j = 0; i < m_size;) {
		while (i < m_size && m_data[i] != '\n' && m_data[i] != '\r') ++i;

		auto eol = i;
		if (i < m_size) {
			if (m_data[i] == '\r' && i + 1 < m_size && m_data[i + 1] == '\n') {
				i += 2;
			}
			else {
				++i;
			}

			if (keepends)
				eol = i;
		}

		result.push_back(substr(j, eol - j));
		j = i;

	}

	if (j < m_size) {
		result.push_back(substr(j, m_size - j));
	}

	return result;
}

char &pystring::operator[](size_t pos)       { return *(m_data + pos); }

char &pystring::operator[](size_t pos) const { return *(m_data + pos); }

char &pystring::front()                { return operator[](0); }

char &pystring::front() const noexcept { return operator[](0); }

char &pystring::back()                 { return operator[](this->size() - 1); }

char &pystring::back() const noexcept { return operator[](this->size() - 1); }

void swap(pystring &first, pystring &second)
{
	using std::swap;

	swap(first.m_data, second.m_data);
	swap(first.m_capacity, second.m_capacity);
	swap(first.m_size, second.m_size);
}

pystring &pystring::append(const pystring &other)
{
	const auto len = m_size + other.size();

	char *buffer = new char[len + 1];
	strcpy(buffer, m_data);
	strcpy(buffer + m_size, other.data());
	buffer[len] = '\0';

	delete [] m_data;

	m_data = buffer;
	m_size = len;
	m_capacity = std::max(m_capacity, m_size);

	return *this;
}

pystring &pystring::operator+=(const pystring &other)
{
	return append(other);
}

pystring &pystring::operator+=(char c)
{
	push_back(c);
	return *this;
}

void pystring::copy_data(const char *ch)
{
	assert(m_size != 0);
	assert(m_data != nullptr);
	assert(ch != nullptr);

	memcpy(m_data, ch, m_size * sizeof(char));
	m_data[m_size] = '\0';
}

void pystring::save_data_n(char *buffer, size_t n)
{
	memcpy(buffer, m_data, n * sizeof(char));

	delete [] m_data;
	m_data = nullptr;
}

std::vector<pystring> pystring::split_whitespace(int maxsplit) const
{
	std::vector<pystring> v;

	size_t j = 0;
	for (size_t i = 0; i < m_size;) {
		while (i < m_size && ::isspace(m_data[i])) {
			++i;
		}

		j = i;

		while (i < m_size && !::isspace(m_data[i])) {
			++i;
		}

		if (j < i) {
			if (v.size() == static_cast<size_t>(maxsplit)) {
				break;
			}

			v.push_back(substr(j, i - j));

			while (i < m_size && ::isspace(m_data[i])) {
				++i;
			}

			j = i;
		}
	}

	if (j < m_size) {
		v.push_back(substr(j, m_size - j));
	}

	return v;
}

void pystring::reverse_strings(std::vector<pystring> &result) const
{
	for (std::vector<pystring>::size_type i = 0; i < result.size() / 2; ++i) {
		std::swap(result[i], result[result.size() - 1 - i]);
	}
}

std::vector<pystring> pystring::rsplit_whitespace(int maxsplit) const
{
	std::vector<pystring> result;
	std::string::size_type i, j;
	for (i = j = m_size; i > 0;) {
		while (i > 0 && ::isspace(m_data[i - 1]))
			i--;

		j = i;

		while (i > 0 && !::isspace(m_data[i - 1]))
			i--;

		if (j > i) {
			if (maxsplit-- <= 0) break;

			result.push_back(substr(i, j - i));

			while (i > 0 && ::isspace(m_data[i - 1]))
				i--;

			j = i;
		}
	}

	if (j > 0) {
		result.push_back(substr(0, j));
	}

	//std::reverse(result, result.begin(), result.end());
	reverse_strings(result);

	return result;
}

pystring pystring::do_strip(const pystring &chars, int striptype)
{
	size_t i, j, charslen = chars.size();

	if (charslen == 0) {
		i = 0;
		if (striptype != RIGHTSTRIP) {
			while (i < m_size && ::isspace(m_data[i])) {
				i++;
			}
		}

		j = m_size;
		if (striptype != LEFTSTRIP) {
			do {
				j--;
			}
			while (j >= i && ::isspace(m_data[j]));

			j++;
		}
	}
	else {
		const char * sep = chars.c_str();
		i = 0;
		if (striptype != RIGHTSTRIP) {
			while (i < m_size && memchr(sep, m_data[i], charslen)) {
				i++;
			}
		}

		j = m_size;
		if (striptype != LEFTSTRIP) {
			do {
				j--;
			}
			while (j >= i &&  memchr(sep, m_data[j], charslen));
			j++;
		}
	}

	if (i == 0 && j == m_size) {
		return *this;
	}

	return substr(i, j - i);
}

bool pystring::_string_tailmatch(const pystring &self, const pystring &substr, size_t start, size_t end, int direction) const
{
	const auto len = self.size();
	const auto slen = substr.size();

	const char* sub = substr.c_str();
	const char* str = self.c_str();
	ADJUST_INDICES(start, end, len);

	if (direction < 0) {
		// startswith
		if (start + slen > len)
			return false;
	}
	else {
		// endswith
		if (end-start < slen || start > len)
			return false;
		if (end-slen > start)
			start = end - slen;
	}
	if (end-start >= slen)
		return (!std::memcmp(str + start, sub, slen));

	return false;
}

void permute(std::ostream &os, std::string str, size_t pos)
{
	if (pos == str.length() - 1) {
		os << str << "\n";
		return;
	}

	for (size_t ch(pos), e(str.length()); ch != e; ++ch) {
		std::swap(str[pos], str[ch]);
		permute(os, str, pos + 1);
	}
}

std::ostream &operator<<(std::ostream &os, const pystring &str)
{
	os << str.c_str();
	return os;
}

}  /* namespace types */
}  /* namespace dls */
