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

set(NOM_PROJET qt)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../danjo/coeur/
)

set(BIBLIOTHEQUES
    ${BIBLIOTHEQUES_QT_WIDGETS}
    ${BIBLIOTHEQUES_QT_OPENGL}
    dls::outils
    danjo
)

set(ENTETES_POUR_QT
    fenetre_principale.hh
    tabs.hh
    widgets.hh
)

if(Qt6_FOUND)
    qt_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
else()
    qt5_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
endif()

add_library(qt_entetes "${moc_headers}")
target_include_directories(qt_entetes PUBLIC "${INCLUSIONS}")
target_link_libraries(qt_entetes PUBLIC ${BIBLIOTHEQUES} ${BIBLIOTHEQUES_QT_WIDGETS} ${BIBLIOTHEQUES_QT_OPENGL})

desactive_drapeaux_compilation(qt_entetes)

list(APPEND BIBLIOTHEQUES qt_entetes)

set(SOURCES
    fenetre_principale.cc
    ipa_danjo.cc
    qt_ipa_c.cc
    tabs.cc
    widgets.cc

    ${ENTETES_POUR_QT}
    ipa_danjo.hh
    qt_ipa_c.h

    ${CMAKE_CURRENT_SOURCE_DIR}/../InterfaceCKuri/contexte_kuri.hh
)

compile_module_c(${NOM_PROJET} qt qt_ipa_c.h Qt "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")
