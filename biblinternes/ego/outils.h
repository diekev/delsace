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

#pragma once

#include <iostream>
#include "biblinternes/structures/chaine.hh"

#include "version.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN
namespace util {

#define GPU_check_errors(message) \
	gl_check_errors(message, __FILE__, __LINE__);

#define GPU_check_framebuffer(message) \
	gl_check_framebuffer(message, __FILE__, __LINE__);

void gl_check_errors(const dls::chaine &message,
                     const dls::chaine &file,
                     const int line);

void gl_check_framebuffer(const dls::chaine &message,
                          const dls::chaine &file,
	                      const int line);

typedef void(* get_ivfunc)(unsigned int index, unsigned int pname, int *param);
typedef void(* get_logfunc)(unsigned int index, int bufSize, int *length, char *infoLog);

bool check_status(unsigned int index,
                  unsigned int pname,
                  const dls::chaine &prefix,
                  get_ivfunc ivfunc,
                  get_logfunc log_func,
                  std::ostream &os = std::cerr);

bool check_bad_alloc();

}  /* namespace util */
EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
