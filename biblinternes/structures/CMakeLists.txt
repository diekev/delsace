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

set(NOM_CIBLE bib_structures)

set(BIBLIOTHEQUES
    PUBLIC dls::math
    PUBLIC dls::outils
    PUBLIC ${BIBLIOTHEQUES_TBB}
    PUBLIC dls::memoire
    PUBLIC dls::moultfilage
)

add_library(${NOM_CIBLE} STATIC
    arbre_kd.cc
    chaine.cc
    chaine_referencee.cc
    chaine_utf8.cc
    dico.cc
    dico_desordonne.cc
    dico_fixe.cc
    ensemble.cc
    ensemblon.cc
    file.cc
    flux_chaine.cc
    grille_particules.cc
    hachage_spatiale.cc
    liste.cc
    liste_recyclee.cc
    magasin.cc
    pile.cc
    plage.cc
    pointeur_marque.cc
    ramasse_miettes.cc
    tableau.cc
    tableau_boucle.cc
    tableau_fixe.cc
    tableau_page.cc
    tableau_simple_compact.cc
    tablet.cc
    vue_chaine.cc
    vue_chaine_compacte.cc

    arbre_kd.hh
    chaine.hh
    chaine_referencee.hh
    chaine_utf8.hh
    dico.hh
    dico_desordonne.hh
    dico_fixe.hh
    ensemble.hh
    ensemblon.hh
    file.hh
    file_fixe.hh
    flux_chaine.hh
    grille_particules.hh
    hachage_spatiale.hh
    liste.hh
    liste_recyclee.hh
    magasin.hh
    matrice_eparse.hh
    pile.hh
    plage.hh
    pointeur_marque.hh
    ramasse_miettes.hh
    tableau.hh
    tableau_boucle.hh
    tableau_fixe.hh
    tableau_page.hh
    tableau_simple_compact.hh
    tablet.hh
    tuples.hh
    vue_chaine.hh
    vue_chaine_compacte.hh
)

add_library(dls::structures ALIAS ${NOM_CIBLE})
target_include_directories(${NOM_CIBLE} PUBLIC .)
target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})
set_property(TARGET ${NOM_CIBLE} PROPERTY POSITION_INDEPENDENT_CODE ON)
installe_dans_module_kuri(${NOM_CIBLE} Kuri)

add_executable(test_structs main.cc)
