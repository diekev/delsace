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
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE kanba_coeur)

set(BIBLIOTHEQUES
    PUBLIC dls::math
    PUBLIC dls::structures
    PUBLIC dls::patrons_conception
    PUBLIC dls::outils

    PRIVATE dls::bruit
    PRIVATE dls::objets
    PRIVATE dls::texture
    PRIVATE dls::vision
    PRIVATE ${BIBLIOTHEQUES_QT_WIDGETS}
)

add_compile_options(-fPIC)

add_library(${NOM_CIBLE} STATIC
    commandes/adaptrice_creation_maillage.h
    commandes/commande_kanba.hh
    commandes/commandes_calques.h
    commandes/commandes_fichier.h
    commandes/commandes_vue2d.h
    commandes/commandes_vue3d.h

    brosse.h
    calques.h
    cannevas_peinture.hh
    evenement.h
    gestionnaire_fenetre.hh
    interface_graphique.hh
    kanba.h
    maillage.h
    melange.h
    outils.hh
    outils_couleur.h
    paqueuse_texture.h
    sauvegarde.hh

    commandes/adaptrice_creation_maillage.cc
    commandes/commande_kanba.cc
    commandes/commandes_calques.cc
    commandes/commandes_fichier.cc
    commandes/commandes_vue2d.cc
    commandes/commandes_vue3d.cc

    brosse.cc
    calques.cc
    cannevas_peinture.cc
    evenement.cc
    gestionnaire_fenetre.cc
    interface_graphique.cc
    kanba.cc
    maillage.cc
    melange.cc
    outils.cc
    outils_couleur.cc
    paqueuse_texture.cc
    sauvegarde.cc
)

add_library(kanba::coeur ALIAS ${NOM_CIBLE})

target_include_directories(${NOM_CIBLE} PUBLIC .)

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})
