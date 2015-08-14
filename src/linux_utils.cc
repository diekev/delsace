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

#include <algorithm>
#include <string>
#include <vector>

#include "linux_utils.h"

namespace Linux {

auto execBuildImageList(const char *cmd) -> std::vector<std::string>
{
	FILE *pipe = popen(cmd, "r");

	char buffer[128];
	auto result = std::vector<std::string>{};

	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != nullptr) {
			auto str = std::string{buffer};
			str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
			result.push_back(str);
		}
	}

	pclose(pipe);

	return result;
}

auto execRemoveImage(const char *cmd) -> void
{
	FILE *pipe = popen(cmd, "r");
	pclose(pipe);
}

} /* namespace Linux */
