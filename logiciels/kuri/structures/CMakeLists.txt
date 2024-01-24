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
# The Original Code is Copyright (C) 2018 Kévin Dietrich.
# All rights réserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE kuri_structures)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
    dls::memoire
    dls::outils
    ${BIBLIOTHEQUE_CXX_FILESYSTEM}
    kuri::utilitaires
)

set(SOURCES
    chaine.cc
    chaine_statique.cc
    chemin_systeme.cc
    date.cc
    enchaineuse.cc
    ensemble.cc
    ensemblon.cc
    file.cc
    intervalle.cc
    pile.cc
    rassembleuse.cc
    table_hachage.cc
    tablet.cc
    tableau.cc
    tableau_compresse.cc
    tableau_statique.cc
    tranche.cc
    trie.cc

    chaine.hh
    chaine_statique.hh
    chemin_systeme.hh
    date.hh
    enchaineuse.hh
    ensemble.hh
    ensemblon.hh
    file.hh
    file_fixe.hh
    intervalle.hh
    pile.hh
    rassembleuse.hh
    table_hachage.hh
    tablet.hh
    tableau.hh
    tableau_compresse.hh
    tableau_statique.hh
    tranche.hh
    trie.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
add_library(kuri::structures ALIAS kuri_structures)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
