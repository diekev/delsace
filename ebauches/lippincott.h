#include <exception>
#include <iostream>

/**
 * @file lippincott.h
 * @brief Generic exception catchers.
 * @author Kévin Dietrich
 *
 * @details Those functions are intended to simplify handling of many different
 * exceptions, mainly to avoid a lot of code duplication.
 * The technique itself was coined by Lisa Lippincott.
 *
 * @note those functions cannot be called outside of a catch () block, otherwise
 *       std::terminate will be called
 * @note those must be noexcept
 *
 * Example usage:
 * @code
 * void boundary_function()
 * {
 *     try {
 *         // code that may throw
 *     }
 *     catch (...) {
 *         catch_exception();
 *     }
 * }
 * @encode
 */

/**
 * Rethrows the currently caught exception.
 */
inline void catch_exception()
{
	try {
		throw;
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Unknown exception..." << std::endl;
	}
}

/**
 * Same as above but with extra safeties:
 * - check that the exception is not null before rethrowing it.
 * - to prevent an exception from being thrown out of the function, its body is
 * wrapped inside a try/catch.
 */
inline void catch_exception_paranoid()
{
	try {
		try {
			if (std::exception_ptr eptr = std::current_exception()) {
				std::rethrow_exception(eptr);
			}
			else {
				std::cerr << "Unknown exception..." << std::endl;
			}
		}
		catch (const std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
		catch (...) {
			std::cerr << "Unknown exception..." << std::endl;
		}
	}
	catch (...) {
		std::cerr << "Unknown exception..." << std::endl;
	}
}

/**
 * To be used with a lambda expression, or so.
 *
 * Example usage:
 * @code
 * void boundary_function()
 * {
 *     catch_exception([&]
 *     {
 *         // ... code that may throw ...
 *     });
 *     }
 * @endcode
 */
template<typename Callable>
void catch_exception(Callable &&f)
{
	try {
		f();
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Unknown exception..." << std::endl;
	}
}

