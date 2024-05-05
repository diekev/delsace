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
# The Original Code is Copyright (C) 2016 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_BIBLIOTHEQUE bib_patrons)

set(BIBLIOTHEQUES
    dls::structures
    pthread
)

set(ENTETES
    commande.h
    cowlockmap.h
    factory.h
    loquets.hh
    oberver.h
    observation.hh
    producers.h
    repondant_commande.h
    singleton.h
    thread_shared_ptr.h
)

set(SOURCES
    commande.cc
    loquets.cc
    oberver.cc
    observation.cc
    repondant_commande.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${ENTETES})
add_library(dls::patrons_conception ALIAS ${NOM_BIBLIOTHEQUE})
set_property(TARGET ${NOM_BIBLIOTHEQUE} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC .)

target_link_libraries(${NOM_BIBLIOTHEQUE} ${BIBLIOTHEQUES})

################################################################################

add_executable(patrons_conception main.cc)

target_include_directories(patrons_conception PUBLIC .)

target_link_libraries(patrons_conception ${BIBLIOTHEQUES} bib_patrons)
