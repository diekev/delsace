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

#include <tuple>
#include <utility>

template <typename Function, typename... Args>
class curry_binder {
	const Function &m_f;
	std::tuple<typename std::remove_reference<Args>::type...> m_args;

	struct curry_arguments {};
	struct try_to_invoke_function : public curry_arguments {};

	template <typename... Args_>
	auto dispatch(try_to_invoke_function, Args_ &&... args) const -> decltype(m_f(args...))
	{
		return m_f(std::forward<Args_>(args)...);
	}

	template <typename... Args_>
	auto dispatch(curry_arguments, Args_ &&... args) const
	{
		return curry(m_f, std::forward<Args_>(args)...);
	}

	template <std::size_t... Ns, typename... OtherArgs>
	auto call(std::index_sequence<Ns...>, OtherArgs &&... args)
	{
		return dispatch(try_to_invoke_function{},
		                std::get<Ns>(m_args)...,
		                std::forward<OtherArgs>(args)...);
	}

public:
	curry_binder(const Function &f, Args &&... args)
	    : m_f(f)
	    , m_args(std::forward<Args>(args)...)
	{}

	template <typename... OtherArgs>
	auto operator()(OtherArgs &&... args)
	{
		return call(std::make_index_sequence<sizeof...(Args)>(),
		            std::forward<OtherArgs>(args)...);
	}
};

template <typename Function, typename... Args>
auto curry(Function &&f, Args &&... args) -> curry_binder<Function, Args...>
{
	return { f, std::forward<Args>(args)... };
}
