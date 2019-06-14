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

set(NOM_BIBLIOTHEQUE memoire)

set(INCLUSIONS
)

set(BIBLIOTHEQUES
)

set(ENTETES
    allocators/affix_allocator.h
    allocators/bitmapped_block.h
    allocators/bucketizer.h
    allocators/cascade_allocator.h
    allocators/freelist.h
    allocators/mallocator.h
    allocators/null_allocator.h
    allocators/stack_allocator.h
    allocators/stats_allocator.h
    composers/fallback_allocator.h
    composers/segregator.h
    allocator.h
    block.h
    guard.h
)

set(SOURCES
    main.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${ENTETES})

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${INCLUSIONS})

target_link_libraries(${NOM_BIBLIOTHEQUE} "${BIBLIOTHEQUES}")

install(TARGETS ${NOM_BIBLIOTHEQUE} ARCHIVE DESTINATION lib)

install(FILES ${ENTETES} DESTINATION include/delsace/${NOM_BIBLIOTHEQUE})
