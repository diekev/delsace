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
# The Original Code is Copyright (C) 2016 Kévin Dietrich.
# All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

import numpy as np

langues = [
    "anglais",
    "français",
    "japonais",
]

fichiers_echantillons = {
        'anglais': "échantillons/anglais.txt",
        'français': "échantillons/français.txt",
        'japonais': "échantillons/japonais.txt"
}

vecteurs_langues = {
    "anglais": [1.0, 0.0, 0.0],
    "français": [0.0, 1.0, 0.0],
    "japonais": [0.0, 0.0, 1.0],
}


def defini_langue(vecteur):
    meilleur = -1.0
    index = -1

    for i, v in np.ndenumerate(vecteur):
        if v < meilleur:
            continue

        meilleur = v
        index = i

    return {'langue': langues[index[0]], 'confidence': meilleur}
