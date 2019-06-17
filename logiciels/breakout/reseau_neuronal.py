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

np.random.seed(1)

def active(x):
    return (np.tanh(x) + 1.0) / 2.0


def melange_matrices(m1, m2, m3):
    for (x, y), v in np.ndenumerate(m1):
        if np.random.ranf(1) < 0.1:
            continue

        if np.random.ranf(1) < 0.5:
            m1[x, y] = m2[x, y]
        else:
            m1[x, y] = m3[x, y]


def accouple_reseaux(ra, rb):
    reseau_fils = ReseauNeuronal()

    melange_matrices(reseau_fils.entrees_couches,
                     ra.entrees_couches,
                     rb.entrees_couches)

    melange_matrices(reseau_fils.couches_sorties,
                     ra.couches_sorties,
                     rb.couches_sorties)

    return reseau_fils


class ReseauNeuronal():
    def __init__(self):
        self.entrees_couches = 2.0 * np.random.random((6, 8)) - 1.0
        self.couches_sorties = 2.0 * np.random.random((8, 2)) - 1.0
        self.sorties = []
        self.neurones = []

    def avance(self, entrees):
        l0 = np.array(entrees)
        l1 = active(np.dot(l0, self.entrees_couches))
        l2 = active(np.dot(l1, self.couches_sorties))

        self.neurones = l1
        self.sorties = l2

#entree = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6]
#reseau = ReseauNeuronal()
#reseau.avance(entree)
