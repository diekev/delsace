#pragma once

struct octet;
struct kilooctet;
struct megaoctet;
struct gigaoctet;
struct teraoctet;

struct octet {
	long valeur;

	octet(const octet &o) = default;
	octet(octet &&o) = default;

	explicit octet(const kilooctet &ko)
	    : valeur(ko.valeur * 1024)
	{}

	explicit octet(const megaoctet &mo)
	    : valeur(mo.valeur * (1024 * 1024))
	{}

	explicit octet(const gigaoctet &go)
	    : valeur(go.valeur * (1024 * 1024 * 1024))
	{}

	explicit octet(const teraoctet &to)
	    : valeur(to.valeur * (1024 * 1024 * 1024 * 1024))
	{}

	octet &operator=(const octet &o) = default;
	octet &operator=(octet &&o) = default;
};

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_istream<type_char, traits_type> &operator>>(
    std::basic_istream<type_char, traits_type> &is,
    octet &o)
{
	is >> o.valeur;
	return is;
}

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_ostream<type_char, traits_type> &operator<<(
    std::basic_ostream<type_char, traits_type> &os,
    const octet &o)
{
	os << o.valeur;
	return os;
}

struct kilooctet {
	long valeur;

	kilooctet(const kilooctet &ko) = default;
	kilooctet(kilooctet &&ko) = default;

	explicit kilooctet(const octet &o)
	    : valeur(o.valeur / 1024)
	{}

	explicit kilooctet(const megaoctet &mo)
	    : valeur(mo.valeur * (1024))
	{}

	explicit kilooctet(const gigaoctet &go)
	    : valeur(go.valeur * (1024 * 1024))
	{}

	explicit kilooctet(const teraoctet &to)
	    : valeur(to.valeur * (1024 * 1024 * 1024))
	{}

	kilooctet &operator=(const kilooctet &ko) = default;
	kilooctet &operator=(kilooctet &&ko) = default;
};

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_istream<type_char, traits_type> &operator>>(
    std::basic_istream<type_char, traits_type> &is,
    kilooctet &ko)
{
	is >> ko.valeur;
	return is;
}

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_ostream<type_char, traits_type> &operator<<(
    std::basic_ostream<type_char, traits_type> &os,
    const kilooctet &ko)
{
	os << ko.valeur;
	return os;
}

struct megaoctet {
	long valeur;

	megaoctet(const megaoctet &mo) = default;
	megaoctet(megaoctet &&mo) = default;

	explicit megaoctet(const octet &o)
	    : valeur(o.valeur / (1024 * 1024))
	{}

	explicit megaoctet(const kilooctet &ko)
	    : valeur(mo.valeur / (1024))
	{}

	explicit megaoctet(const gigaoctet &go)
	    : valeur(go.valeur * (1024))
	{}

	explicit megaoctet(const teraoctet &to)
	    : valeur(to.valeur * (1024 * 1024))
	{}

	megaoctet &operator=(const megaoctet &mo) = default;
	megaoctet &operator=(megaoctet &&mo) = default;
};

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_istream<type_char, traits_type> &operator>>(
    std::basic_istream<type_char, traits_type> &is,
    megaoctet &mo)
{
	is >> mo.valeur;
	return is;
}

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_ostream<type_char, traits_type> &operator<<(
    std::basic_ostream<type_char, traits_type> &os,
    const megaoctet &mo)
{
	os << mo.valeur;
	return os;
}

struct gigaoctet {
	long valeur;

	gigaoctet(const gigaoctet &go) = default;
	gigaoctet(gigaoctet &&go) = default;

	explicit gigaoctet(const octet &o)
	    : valeur(o.valeur / (1024 * 1024 * 1024))
	{}

	explicit gigaoctet(const kilooctet &ko)
	    : valeur(mo.valeur / (1024 * 1024))
	{}

	explicit gigaoctet(const megaoctet &mo)
	    : valeur(mo.valeur / (1024))
	{}

	explicit gigaoctet(const teraoctet &to)
	    : valeur(to.valeur * (1024))
	{}

	gigaoctet &operator=(const gigaoctet &o) = default;
	gigaoctet &operator=(gigaoctet &&o) = default;
};

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_istream<type_char, traits_type> &operator>>(
    std::basic_istream<type_char, traits_type> &is,
    gigaoctet &go)
{
	is >> go.valeur;
	return is;
}

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_ostream<type_char, traits_type> &operator<<(
    std::basic_ostream<type_char, traits_type> &os,
    const gigaoctet &go)
{
	os << go.valeur;
	return os;
}

struct teraoctet {
	long valeur;

	teraoctet(const teraoctet &to) = default;
	teraoctet(teraoctet &&to) = default;

	explicit teraoctet(const octet &o)
	    : valeur(o.valeur / (1024 * 1024 * 1024 * 1024))
	{}

	explicit teraoctet(const kilooctet &ko)
	    : valeur(mo.valeur / (1024 * 1024 * 1024))
	{}

	explicit teraoctet(const megaoctet &mo)
	    : valeur(mo.valeur / (1024 * 1024))
	{}

	explicit teraoctet(const gigaoctet &go)
	    : valeur(go.valeur / (1024))
	{}

	teraoctet &operator=(const teraoctet &to) = default;
	teraoctet &operator=(teraoctet &&to) = default;
};

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_istream<type_char, traits_type> &operator>>(
    std::basic_istream<type_char, traits_type> &is,
    teraoctet &to)
{
	is >> to.valeur;
	return is;
}

template <typename type_char, typename traits_type = std::char_traits<type_char>>
std::basic_ostream<type_char, traits_type> &operator<<(
    std::basic_ostream<type_char, traits_type> &os,
    const teraoctet &to)
{
	os << to.valeur;
	return os;
}
