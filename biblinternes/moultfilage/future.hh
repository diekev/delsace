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

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include "biblinternes/structures/tableau.hh"

using lock_t = std::unique_lock<std::mutex>;

/**
 * Return the result of an expression.
 */

template <typename>
struct result_of_;

template <typename R, typename... Args>
struct result_of_<R(Args...)> {
	using type = R;
};

template <typename F>
using result_of_t_ = typename result_of_<F>::type;

/**
 * forward declaration
 */

template <typename>
class packaged_task;

template <typename>
class future;

/**
 * Given a function, return a packaged task and the future holding to it.
 */
template <typename S, typename F>
auto package(F &&f) -> std::pair<packaged_task<S>, future<result_of_t_<S>>>;

template <typename R>
struct shared_base {
	dls::tableau<R> _r; // poor man's boost::optionnal
	std::mutex _mutex;
	std::condition_variable _ready; // for debugging with future.get()
	dls::tableau<std::function<void()>> _then;

	virtual ~shared_base() {}

	void set(R &&r)
	{
		dls::tableau<std::function<void()>> then;
		{
			lock_t lock(_mutex);
			_r.pousse(std::move(r));
			swap(_then, then);
		}

		_ready.notify_all();

		for (const auto &f : then) {
		//	_system.async_(std::move(f));
		}
	}

	template <typename F>
	void then(F &&f)
	{
		bool resolved(false);

		{
			lock_t lock(_mutex);

			/* check whether or not the value already arrived */
			if (_r.est_vide()) {
				_then.pousse(std::forward<F>(f));
			}
			else {
				resolved = true;
			}
		}

		if (resolved) {
			//	_system.async_(std::move(f));
		}
	}

	const R &get()
	{
		lock_t lock(_mutex);

		while (_r.est_vide()) {
			_ready.wait(lock);
		}

		return _r.back();
	}
};

template <typename>
struct shared;

template <typename R, typename... Args>
struct shared<R(Args...)> : public shared_base<R> {
	std::function<R(Args...)> _f;

	template <typename F>
	shared(F &&f)
	    : _f(std::forward<F>(f))
	{}

	template <typename... A>
	void operator()(A &&... args)
	{
		/* set the result of calling the function */
		this->set(_f(std::forward<A>(args)...));
		_f = nullptr;
	}
};

template <typename R>
class future {
	std::shared_ptr<shared_base<R>> _p;

	template <typename S, typename F>
	friend auto package(F &&f) -> std::pair<packaged_task<S>, future<result_of_t_<S>>>;

	explicit future(std::shared_ptr<shared_base<R>> p)
	    : _p(std::move(p))
	{}

public:
	future() = default;

	template <typename F>
	auto then(F &&f)
	{
		auto pack = package<result_of_t_<F(R)>()>([p = _p, f = std::forward<F>(f)]() {
			return f(p->_r.back());
		});

		_p->then(std::move(pack.first));
		return pack.second;
	}

	/* for debugging */
	const R &get() const
	{
		return _p->get();
	}
};

template <typename S, typename F>
auto package(F &&f) -> std::pair<packaged_task<S>, future<result_of_t_<S>>>
{
	auto p = std::make_shared<shared<S>>(std::forward<F>(f));
	return std::make_pair(packaged_task<S>(p), future<result_of_t_<S>>(p));
}

template <typename R, typename... Args>
class packaged_task<R(Args...)> {
	std::weak_ptr<shared<R(Args...)>> _p;

	template <typename S, typename F>
	friend auto package(F &&f) -> std::pair<packaged_task<S>, future<result_of_t_<S>>>;

	explicit packaged_task(std::weak_ptr<shared<R(Args...)>> p)
	    : _p(std::move(p))
	{}

public:
	packaged_task() = default;

	template <typename... A>
	void operator()(A &&... args) const
	{
		auto p = _p.lock();

		if (p) {
			(*p)(std::forward<A>(args)...);
		}
	}
};

template <typename F, typename... Args>
auto async(F &&f, Args &&... args)
{
	using result_type = result_of_t_<F(Args...)>;
	using packaged_type = packaged_task<result_type()>;

	auto pack = package<result_type()>(std::bind(std::forward<F>(f),
	                                             std::forward<Args>(args)...));

	/* À FAIRE: replace with task queue */
	std::thread(std::move(std::get<0>(pack))).detach();

	return std::get<1>(pack);
}
