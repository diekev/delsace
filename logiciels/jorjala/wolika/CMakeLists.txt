# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2019 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE wolika)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

set(BIBLIOTHEQUES
)

add_library(${NOM_CIBLE} STATIC
    base_grille.cc
	echantillonnage.cc
	filtre_2d.cc
	filtre_3d.cc
	fusion_3d.cc
    grille_dense.cc
    grille_eparse.cc
    grille_temporelle.cc
	interruptrice.cc
	iteration.cc
	outils.cc

    base_grille.hh
	echantillonnage.hh
	filtre_2d.hh
	filtre_3d.hh
	fusion_3d.hh
    grille_dense.hh
    grille_eparse.hh
    grille_temporelle.hh
	interruptrice.hh
	iteration.hh
	outils.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

