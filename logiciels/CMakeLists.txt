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

ajoute_sousdossier(AVEC_CRESUS cresus)
ajoute_sousdossier(AVEC_DANJO danjo)
ajoute_sousdossier(AVEC_DEPLOIE deploie)
ajoute_sousdossier(AVEC_HIKAGE hikage)
ajoute_sousdossier(AVEC_HIMITSU himitsu)
ajoute_sousdossier(AVEC_IMAGO imago)
ajoute_sousdossier(AVEC_INFOGRAPHIE infographie)
ajoute_sousdossier(AVEC_JORJALA jorjala)
ajoute_sousdossier(AVEC_KANBA kanba)
ajoute_sousdossier(AVEC_KUMO kumo)
ajoute_sousdossier(AVEC_KURI kuri)
ajoute_sousdossier(AVEC_KYANBASU kyanbasu)
ajoute_sousdossier(AVEC_MESURE mesure)
ajoute_sousdossier(AVEC_SILVATHEQUE silvatheque)
ajoute_sousdossier(AVEC_TORI tori)
ajoute_sousdossier(AVEC_TORI_V2 tori_v2)
ajoute_sousdossier(AVEC_VULKAN vulkan)

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
	fichiers/icones/icone_ocean.png
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
	install(
		FILES ${STYLES}
		DESTINATION kanba/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION kanba/texte
	)
endif (AVEC_KANBA)

if (AVEC_JORJALA)
	install(
		FILES ${STYLES}
		DESTINATION jorjala/styles
	)

	install(
		FILES ${ICONES}
		DESTINATION jorjala/icones
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION jorjala/texte
	)
endif (AVEC_JORJALA)

if (AVEC_SILVATHEQUE)
	install(
		FILES ${STYLES}
		DESTINATION silvatheque/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION silvatheque/texte
	)
endif (AVEC_SILVATHEQUE)
