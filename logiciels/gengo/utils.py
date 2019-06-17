# -*- coding: utf-8 -*-

# ##### BEGIN GPL LICENSE BLOCK #####
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
# along with this program; if not, write to the Free Software  Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2015 Kévin Dietrich.
# All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

import array

LONGUEUR_MOT_MAX = 25

def make_unicode(text):
    return unicode(text, 'utf-8')


def convertie_mot(mot):
    octets_mot = array.array('B', mot)

    octets_normalises = []

    for octet in octets_mot:
        octets_normalises.append(octet / 255.0)

    donnees_entrees = []
    longueur_mot = len(octets_normalises)

    if longueur_mot <= LONGUEUR_MOT_MAX:
        donnees_entrees = octets_normalises

        if longueur_mot < LONGUEUR_MOT_MAX:
            for i in range(LONGUEUR_MOT_MAX - longueur_mot):
                donnees_entrees.append(-1.0)
    else:
        for i in range(LONGUEUR_MOT_MAX):
            donnees_entrees.append(octets_normalises[i])

    # Biais
    donnees_entrees.append(1.0)

    return donnees_entrees
