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

add_subdirectory(arachne)
add_subdirectory(danjo)
add_subdirectory(kuri)
add_subdirectory(kumo)
add_subdirectory(mesure)
add_subdirectory(parcer)
add_subdirectory(tori)

set(ICONES
	fichiers/icones/icone_avertissement.png
	fichiers/icones/icon_box.png
	fichiers/icones/icon_circle.png
	fichiers/icones/icon_cone.png
	fichiers/icones/icon_grid.png
	fichiers/icones/icon_icosphere.png
	fichiers/icones/icon_jump_first.png
	fichiers/icones/icon_jump_last.png
	fichiers/icones/icon_pause.png
	fichiers/icones/icon_play_forward.png
	fichiers/icones/icon_play_backward.png
	fichiers/icones/icon_step_forward.png
	fichiers/icones/icon_step_backward.png
	fichiers/icones/icon_stop.png
	fichiers/icones/icon_torus.png
	fichiers/icones/icon_tube.png
	fichiers/icones/icon_uvsphere.png
	fichiers/icones/icone_echelle.png
	fichiers/icones/icone_manipulation.png
	fichiers/icones/icone_position.png
	fichiers/icones/icone_rotation.png
	fichiers/icones/icone_echelle_active.png
	fichiers/icones/icone_manipulation_active.png
	fichiers/icones/icone_position_active.png
	fichiers/icones/icone_rotation_active.png
)

set(STYLES
	fichiers/styles/main.qss
)

if (AVEC_KANBA)
	add_subdirectory(kanba)

	install(
		FILES ${STYLES}
		DESTINATION kanba/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION kanba/texte
	)
endif (AVEC_KANBA)

if (AVEC_KOUDOU)
	add_subdirectory(koudou)

	install(
		FILES ${STYLES}
		DESTINATION koudou/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION koudou/texte
	)
endif (AVEC_KOUDOU)

if (AVEC_MIKISA)
	add_subdirectory(mikisa)

	install(
		FILES ${STYLES}
		DESTINATION mikisa/styles
	)

	install(
		FILES ${ICONES}
		DESTINATION mikisa/icones
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION mikisa/texte
	)
endif (AVEC_MIKISA)

if (AVEC_POSEIDON)
	add_subdirectory(poseidon)

	install(
		FILES ${ICONES}
		DESTINATION poseidon/icones
	)

	install(
		FILES ${STYLES}
		DESTINATION poseidon/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION poseidon/texte
	)
endif (AVEC_POSEIDON)

if (AVEC_SILVATHEQUE)
	add_subdirectory(silvatheque)

	install(
		FILES ${STYLES}
		DESTINATION silvatheque/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION silvatheque/texte
	)
endif (AVEC_SILVATHEQUE)

if (AVEC_VULKAN)
	set(NOM_PROJET vulkan)
	add_subdirectory(vulkan)
endif (AVEC_VULKAN)
