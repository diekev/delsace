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
# The Original Code is Copyright (C) 2017 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE koudou_coeur)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_MATH}
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_IMAGE}
	bib_commandes
	bib_objets
	bib_observation
	bib_outils
	bib_vision
	ego
)

qt5_wrap_cpp(
	moc_headers

	tache.h
)

add_compile_options(-fPIC)

add_library(qt_koudou_coeur ${moc_headers})
target_include_directories(qt_koudou_coeur PUBLIC "${INCLUSIONS}")
get_target_property(EXTLIB_COMPILE_FLAGS qt_koudou_coeur COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wconversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wsign-conversion)
set_target_properties(qt_koudou_coeur PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

add_library(${NOM_CIBLE} STATIC
	commandes/adaptrice_creation_maillage.h
	commandes/commandes_fichier.h
	commandes/commandes_rendu.h
	commandes/commandes_vue3d.h

	bsdf.h
	configuration.h
	evenement.h
	koudou.h
	lumiere.h
	maillage.h
	objet.h
	pellicule.h
	moteur_rendu.h
	nuanceur.h
	scene.h
	statistiques.h
	structure_acceleration.h
	tache.h
	types.h
	volume.h

	commandes/adaptrice_creation_maillage.cc
	commandes/commandes_fichier.cc
	commandes/commandes_rendu.cc
	commandes/commandes_vue3d.cc

	bsdf.cc
	configuration.cc
	evenement.cc
	koudou.cc
	lumiere.cc
	maillage.cc
	moteur_rendu.cc
	nuanceur.cc
	objet.cc
	pellicule.cc
	scene.cc
	statistiques.cc
	structure_acceleration.cc
	tache.cc
	types.cc
	volume.cc
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}" qt_koudou_coeur)
