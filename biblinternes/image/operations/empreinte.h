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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <experimental/filesystem>

namespace filesystem = std::experimental::filesystem;

namespace dls {
namespace image {
namespace operation {

/**
 * Calcul l'empreinte d'une image pour pouvoir insérer l'image dans une table de
 * hachage. La fonction se base sur le gradient horizontal de l'image et est
 * inspirée de :
 * https://www.hackerfactor.com/blog/index.php?/archives/432-Looks-Like-It.html
 */
size_t empreinte_image(const filesystem::path &chemin_image);

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
