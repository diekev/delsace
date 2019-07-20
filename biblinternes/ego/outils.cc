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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "outils.h"

#include <execinfo.h>  /* for backtrace() */
#include <fstream>
#include <GL/glew.h>
#include <iostream>
#include <memory>
#include <sstream>

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN
namespace util {

/* Adapted from Blender. */
static void system_backtrace(std::ostream &os)
{
	constexpr auto SIZE = 100;

	void *buffer[SIZE];
	int nptrs = backtrace(buffer, SIZE);
	char **strings = backtrace_symbols(buffer, nptrs);

	for (int i = 0; i < nptrs; ++i) {
		os << strings[i] << '\n';
	}

	free(strings);
}

void gl_check_errors(const dls::chaine &message,
                     const dls::chaine &file,
                     const int line)
{
	unsigned int error = glGetError();

	if (error == GL_NO_ERROR) {
		return;
	}

	std::ostream &os = std::cerr;

	/* include a backtrace for good measure */
	system_backtrace(os);

	os << file << ":" << line << ": GL Error: - " << message << ": ";

	switch (error) {
		case GL_INVALID_ENUM:
			os << "invalid enum!\n";
			break;
		case GL_INVALID_VALUE:
			os << "invalid value!\n";
			break;
		case GL_INVALID_OPERATION:
			os << "invalid operation!\n";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			os << "invalid framebuffer operation!\n";
			break;
	}

	/* Cause a crash if an error was caught. */
	abort();
}

bool check_bad_alloc()
{
	return glGetError() == GL_OUT_OF_MEMORY;
}

void gl_check_framebuffer(const dls::chaine &message,
                          const dls::chaine &file,
                          const int line)
{
	unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status == GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	std::ostream &os = std::cerr;

	/* include a backtrace for good measure */
	system_backtrace(os);

	os << file << ":" << line << ": Framebuffer Error: - " << message << ": ";

	switch (status) {
		case GL_FRAMEBUFFER_UNDEFINED:
			os << "default framebuffer does not exist!\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			os << "framebuffer attachment points are incomplete!\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			os << "framebuffer does not have an image attached to it!\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			os << "draw buffer color attachment point is 'none'!\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			os << "read buffer color attachment point is 'none'!\n";
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			os << "combination of internal formats is unsupported!\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			os << "incomplete multisample!\n";
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			os << "incomplete layer targets!\n";
			break;
	}

	/* Cause a crash if an error was caught. */
	abort();
}

/* Utility function to check whether a program or shader was linked properly or
 * not. Prints an error message if linking failed. */
bool check_status(unsigned int index,
                  unsigned int pname,
                  const dls::chaine &prefix,
                  get_ivfunc ivfunc,
                  get_logfunc log_func,
                  std::ostream &os)
{
	int status;
	ivfunc(index, pname, &status);

	if (status == GL_TRUE) {
		return true;
	}

	int log_length;
	ivfunc(index, GL_INFO_LOG_LENGTH, &log_length);

	auto log = std::unique_ptr<char[]>(new char[log_length]);
	log_func(index, log_length, &log_length, log.get());

	os << prefix << ": " << log.get() << '\n';

	return false;
}

}  /* namespace util */
EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
