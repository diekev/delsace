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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <system_error>

extern "C" {

struct FileExecution {
	std::function<void(void*)> rappel_;
	void *argument_;
	std::thread thread;
	bool fut_joint_ = false;
	bool termine_ = false;

	FileExecution(std::function<void(void*)> rappel, void *arg)
		: rappel_(rappel)
		, argument_(arg)
		, thread(&FileExecution::lance, this)
	{
	}

	FileExecution(FileExecution const&) = delete;
	FileExecution &operator=(FileExecution const&) = delete;

	~FileExecution()
	{
		if (!fut_joint_) {
			joins();
		}
	}

	static void lance(void *arg)
	{
		auto moi = static_cast<FileExecution *>(arg);
		moi->rappel_(moi->argument_);
		moi->termine_ = true;
	}

	void annule()
	{
		thread.~thread();
	}

	void detache()
	{
		thread.detach();
	}

	bool joins()
	{
		fut_joint_ = true;

		try {
			thread.join();
			termine_ = true;
			return true;
		}
		catch (const std::system_error &) {
			termine_ = true;
			return false;
		}
	}

	bool termine() const
	{
		return termine_;
	}
};

unsigned int EXETRON_nombre_fils_materiel()
{
	return std::thread::hardware_concurrency();
}

void *EXETRON_cree_fil(void(*rappel)(void*), void *argument)
{
	return new FileExecution(rappel, argument);
}

/* À FAIRE : ajout d'une durée minimum à attendre avant d'annuler si le fil ne fut pas joint. */
bool EXETRON_joins_fil(void *ptr_fil)
{
	auto fil = static_cast<FileExecution *>(ptr_fil);
	return fil->joins();
}

void EXETRON_detache_fil(void *ptr_fil)
{
	auto fil = static_cast<FileExecution *>(ptr_fil);
	fil->detache();
}

void EXETRON_detruit_fil(void *ptr_fil)
{
	auto fil = static_cast<FileExecution *>(ptr_fil);
	delete fil;
}

using Mutex = std::mutex;

Mutex *EXETRON_cree_mutex()
{
	return new Mutex;
}

void EXETRON_verrouille_mutex(Mutex *mutex)
{
	mutex->lock();
}

void EXETRON_deverrouille_mutex(Mutex *mutex)
{
	mutex->unlock();
}

void EXETRON_detruit_mutex(Mutex *mutex)
{
	delete mutex;
}

using VariableCondition = std::condition_variable;

VariableCondition *EXETRON_cree_variable_condition()
{
	return new VariableCondition;
}

void EXETRON_variable_condition_notifie_tous(VariableCondition *condition_variable)
{
	condition_variable->notify_all();
}

void EXETRON_variable_condition_notifie_un(VariableCondition *condition_variable)
{
	condition_variable->notify_one();
}

void EXETRON_variable_condition_attend(VariableCondition *condition_variable, Mutex *mutex)
{
	std::unique_lock l(*mutex);
	condition_variable->wait(l);
}

void EXETRON_detruit_variable_condition(VariableCondition *mutex)
{
	delete mutex;
}

}
