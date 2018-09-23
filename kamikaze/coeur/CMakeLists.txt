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

set(NOM_CIBLE kamikaze_coeur)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_DANJO}
	${INCLUSIONS_SYSFICHIER}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_DANJO}
	${BIBLIOTHEQUES_SYSFICHIER}
	${BIBLIOTHEQUES_IMAGE}
	${BIBLIOTHEQUES_MATH}
	${BIBLIOTHEQUES_TBB}
	${BIBLIOTHEQUES_QT5_CORE}
	${BIBLIOTHEQUES_QT5_WIDGETS}
	bib_objets
	bib_vision
	bib_xml
	bib_commandes
	bib_geometrie
)

add_compile_options(-fPIC)

qt5_wrap_cpp(
	task_notifier_moc

	task.h
)

add_library(${NOM_CIBLE} STATIC
	context.h
	kamikaze_main.h
	object.h
	sauvegarde.h
	scene.h
	task.h

	${task_notifier_moc}

	commandes/commandes_graphes.h
	commandes/commandes_objet.h
	commandes/commandes_projet.h
	commandes/commandes_scene.h
	commandes/commandes_vue3d.h

	graphs/depsgraph.h
	graphs/graph_dumper.h
	graphs/graph_tools.h
	graphs/object_graph.h
	graphs/scene_node.h

	operatrices/operatrices_physiques.h
	operatrices/operatrices_standards.h

	context.cc
	kamikaze_main.cc
	object.cc
	task.cc
	sauvegarde.cc
	scene.cc

	commandes/commandes_graphes.cc
	commandes/commandes_objet.cc
	commandes/commandes_projet.cc
	commandes/commandes_scene.cc
	commandes/commandes_vue3d.cc

	graphs/depsgraph.cc
	graphs/graph_dumper.cc
	graphs/object_graph.cc
	graphs/scene_node.cc

	operatrices/operatrices_physiques.cc
	operatrices/operatrices_standards.cc

	sdk/outils/chaîne_caractère.h
	sdk/outils/géométrie.cc
	sdk/outils/géométrie.h
	sdk/outils/mathématiques.h
	sdk/outils/rendu.h
	sdk/attribute.cc
	sdk/attribute.h
	sdk/context.cc
	sdk/context.h
	sdk/factory.h
	sdk/geomlists.cc
	sdk/geomlists.h
	sdk/mesh.cc
	sdk/mesh.h
	sdk/noeud.cc
	sdk/noeud.h
	sdk/operatrice.cc
	sdk/operatrice.h
	sdk/primitive.cc
	sdk/primitive.h
	sdk/prim_points.cc
	sdk/prim_points.h
	sdk/segmentprim.cc
	sdk/segmentprim.h

	util/util_input.h
	util/util_memory.h
	util/utils.cc
	util/utils.h
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
