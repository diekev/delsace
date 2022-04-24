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
# The Original Code is Copyright (C) 2020 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

find_package(Alembic REQUIRED)

set(NOM_PROJET alembic)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
    /opt/lib/alembic/include/
    /opt/lib/openexr/include/
    /opt/lib/openexr/include/OpenEXR
)

set(BIBLIOTHEQUES
    /opt/lib/alembic/lib/libAlembic.so
)

set(SOURCES
  AbcKuri/alembic_archive.cc
  AbcKuri/alembic_export.cc
  AbcKuri/alembic_import.cc
  AbcKuri/utilitaires.cc

  alembic_ipa_c.cc

  AbcKuri/alembic_archive.hh
  AbcKuri/alembic_export.hh
  AbcKuri/alembic_import.hh
  AbcKuri/utilitaires.hh

  alembic.h
  alembic_ipa_c.h
  alembic_types.h

  ${CMAKE_CURRENT_SOURCE_DIR}/../InterfaceCKuri/contexte_kuri.hh
)

# À FAIRE : depends alembic_types.h
compile_module_c(${NOM_PROJET} alembic alembic_ipa_c.h Alembic "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")
