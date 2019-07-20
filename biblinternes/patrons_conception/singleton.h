#pragma once

#include <atomic>
#include <mutex>

using lock_t = std::unique_lock<std::mutex>;

template <typename T, int dummy>
class Singleton;

/* Singleton allocated on the stack */
template <typename T>
class Singleton<T, 0> {
	static std::mutex m_mutex;

public:
	static T *instance()
	{
		/* Thread safe initialization, done by the linker. */
		static T instance;
		return &instance;
	}

	T &operator=(const T&) = delete;
};

template <typename T>
std::mutex Singleton<T, 0>::m_mutex;

/* Singleton allocated on the heap */
template <typename T>
class Singleton<T, 1> {
	static std::mutex m_mutex;

public:
	/* Double check locking, for properly initializing dynamically allocated,
	 * memory. Note that the compiler can reorder the operations behind memory
	 * allocation which can lead to a thread getting an allocated instance with
	 * garbage memory. */
	static T *instance()
	{
		/* initialized by the linker */
		static std::atomic<T *> instance{nullptr};

		/* Put a barrier on the load operation so that any writes that happened
		 * to the instance prior are visible to us, thus we actually avoid any
		 * race condition, deadlocks or other starvation. */
		if (instance.load(std::memory_order_acquire) == nullptr) {
			lock_t lock(m_mutex);

			/* perform a non atomic load */
			if (instance.load() == nullptr) {
				/* Release the barrier to guarantee that all memory operations
				 * scheduled before the barrier in the program order become
				 * visible before the (next) barrier. */
				instance.store(new T, std::memory_order_release);
			}
		}

		return instance;
	}

	T &operator=(const T&) = delete;
};

template <typename T>
std::mutex Singleton<T, 1>::m_mutex;
