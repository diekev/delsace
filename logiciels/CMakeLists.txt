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

ajoute_sousdossier(AVEC_ARACHNE arachne)
ajoute_sousdossier(AVEC_CRESUS cresus)
ajoute_sousdossier(AVEC_DANJO danjo)
ajoute_sousdossier(AVEC_HIKAGE hikage)
ajoute_sousdossier(AVEC_HIMITSU himitsu)
ajoute_sousdossier(AVEC_IMAGO imago)
ajoute_sousdossier(AVEC_INFOGRAPHIE infographie)
ajoute_sousdossier(AVEC_KANBA kanba)
ajoute_sousdossier(AVEC_KOUDOU koudou)
ajoute_sousdossier(AVEC_KUMO kumo)
ajoute_sousdossier(AVEC_KURI kuri)
ajoute_sousdossier(AVEC_KYANBASU kyanbasu)
ajoute_sousdossier(AVEC_MIKISA mikisa)
ajoute_sousdossier(AVEC_MESURE mesure)
ajoute_sousdossier(AVEC_PARCER parcer)
ajoute_sousdossier(AVEC_POSEIDON poseidon)
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

if (AVEC_KOUDOU)
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
	install(
		FILES ${STYLES}
		DESTINATION silvatheque/styles
	)

	install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION silvatheque/texte
	)
endif (AVEC_SILVATHEQUE)
