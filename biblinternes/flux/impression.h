#pragma once

#include <stdexcept>
#include <type_traits>

#include "biblinternes/structures/chaine.hh"

namespace dls {
namespace flux {

namespace __detail {

template<typename T>
struct is_char : std::false_type {};

template<>
struct is_char<char> : std::true_type {};

template<typename T>
struct __is_c_string_helper : std::false_type {};

template<typename T>
struct __is_c_string_helper<T *> : is_char<typename std::remove_cv<T>::type> {};

template<typename T>
struct is_c_string : __is_c_string_helper<typename std::remove_cv<T>::type> {};

template<typename T>
auto normalize_args(T arg) -> typename std::enable_if<std::is_integral<T>::value, long>::type
{
	return arg;
}

template<typename T>
auto normalize_args(T arg) -> typename std::enable_if<std::is_floating_point<T>::value, double>::type
{
	return arg;
}

template<typename T>
auto normalize_args(T arg) -> typename std::enable_if<std::is_pointer<T>::value, T>::type
{
	return arg;
}

auto normalize_args(const dls::chaine &arg)
{
	return arg.c_str();
}

auto check_printf(const char *f)
{
	for (; *f; ++f) {
		if (*f != '%' || *++f == '%') {
			continue;
		}

		throw std::invalid_argument("Bad format");
	}
}

template<typename T> const char* typename_as_string()             { return "unknown"; }
template<> inline const char* typename_as_string<bool>()          { return "bool"; }
template<> inline const char* typename_as_string<float>()         { return "float"; }
template<> inline const char* typename_as_string<double>()        { return "double"; }
template<> inline const char* typename_as_string<long double>()   { return "long double"; }
template<> inline const char* typename_as_string<char>()          { return "char"; }
template<> inline const char* typename_as_string<int>()           { return "int"; }
template<> inline const char* typename_as_string<unsigned int>()  { return "unsigned int"; }
template<> inline const char* typename_as_string<long>()          { return "long"; }
template<> inline const char* typename_as_string<unsigned long>() { return "unsigned long"; }
template<> inline const char* typename_as_string<const char *>()  { return "const char *"; }

#define ENFORCE(x, type)                                                        \
	if (!(x)) {                                                                 \
		dls::chaine err{typename_as_string<T>()};                               \
		err += " is of a wrong type, expected: ";                               \
		err += typename_as_string<type>();                                      \
		throw std::invalid_argument(err.c_str());                                       \
	}

template<typename T, typename... Ts>
auto check_printf(const char *f, const T &t, const Ts &... ts)
{
	for (; *f; ++f) {
		if (*f != '%' || *++f == '%') {
			continue;
		}

		while (*f >= '0' && *f <= '9') {
			++f;
		}

		switch (*f) {
			default: throw std::invalid_argument("Invalid format char: %");
			case 'f':
			case 'F':
			case 'g':
			case 'G':
				ENFORCE(std::is_floating_point<T>::value, double);
				break;
			case 'd':
			case 'i':
				ENFORCE(std::is_integral<T>::value, int);
				ENFORCE(std::is_signed<T>::value, int);
				break;
			case 'u':
				ENFORCE(std::is_integral<T>::value, unsigned int);
				break;
			case 's':
				ENFORCE(is_c_string<T>::value, const char *);
				break;
			case 'c':
				ENFORCE(std::is_integral<T>::value, char);
				//ENFORCE(_is_char<T>::value, char); // arg is normalized...
				break;
			case 'p':
				ENFORCE(std::is_pointer<T>::value, T*);
				break;
			case 'x':
			case 'X':
			case 'e':
			case 'E':
			case 'a':
			case 'A':
			case 'n':
				break;
		}

		return check_printf(++f, ts...);
	}

	throw std::invalid_argument("Too few format specifier");
}

}  /* namespace __detail */

template<typename... Ts>
auto print(const char *f, const Ts &... ts)
{
	__detail::check_printf(f, __detail::normalize_args(ts)...);
	return std::printf(f, __detail::normalize_args(ts)...);
}

}  /* namespace flux */
}  /* namespace dls */
