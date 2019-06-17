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

from PyQt4 import QtCore, QtGui

TAILLE_NEURONE = 13
LARGEUR = 400
HAUTEUR = 720


def clamp(x):
    if x < 0.0:
        return 0.0

    if x > 1.0:
        return 1.0

    return x


class DessinateurReseau(QtGui.QWidget):
    def __init__(self, reseau_neuronal, parent=None):
        super(DessinateurReseau, self).__init__(parent)

        self.reseau = reseau_neuronal

        self.setFixedSize(LARGEUR, HAUTEUR)

        self.neurones_entrees = []
        self.neurones_caches = []
        self.neurones_sorties = []

        self.initialise_neurones()

        self.update()

    def initialise_neurones(self):
        nombre_entrees = self.reseau.nombre_entrees
        nombre_caches = self.reseau.nombre_caches
        nombre_sorties = self.reseau.nombre_sorties

        start_x = LARGEUR / 4
        start_y = ((HAUTEUR - ((2 * nombre_entrees - 1) * TAILLE_NEURONE)) / 2)

        for i in range(self.reseau.nombre_entrees):
            pos = QtCore.QPoint(start_x, start_y)
            self.neurones_entrees.append(pos)
            start_y += TAILLE_NEURONE * 2

        start_x += LARGEUR / 4
        start_y = ((HAUTEUR - ((2 * nombre_caches - 1) * TAILLE_NEURONE)) / 2)

        for i in range(nombre_caches):
            pos = QtCore.QPoint(start_x, start_y)
            self.neurones_caches.append(pos)

            start_y += TAILLE_NEURONE * 2

        start_x += LARGEUR / 4
        start_y = ((HAUTEUR - ((2 * nombre_sorties - 1) * TAILLE_NEURONE)) / 2)

        for i in range(nombre_sorties):
            pos = QtCore.QPoint(start_x, start_y)
            self.neurones_sorties.append(pos)

            start_y += TAILLE_NEURONE * 2

    def paintEvent(self, event):
        peintre = QtGui.QPainter()

        peintre.begin(self)
        self.dessine_reseau(peintre)
        peintre.end()

    def dessine_reseau(self, peintre):
        nombre_entrees = self.reseau.nombre_entrees
        nombre_caches = self.reseau.nombre_caches
        nombre_sorties = self.reseau.nombre_sorties

        peintre.setBrush(QtGui.QColor.fromRgbF(0.09, 0.09, 0.09))
        peintre.drawRect(self.rect())

        stylo = QtGui.QPen(QtGui.QColor.fromRgbF(0.25, 0.25, 0.25, 0.5))
        peintre.setPen(stylo)
        pos_offset_in = QtCore.QPoint(10, 5)
        pos_offset_out = QtCore.QPoint(0, 5)

        # Dessine les entrées.
        entrees = self.reseau.entrees

        for i in range(nombre_entrees):
            pos = self.neurones_entrees[i]
            entree = clamp(entrees[i])
            peintre.setBrush(QtGui.QColor.fromRgbF(entree, entree, entree))

            for j in range(nombre_caches):
                peintre.drawLine(pos + pos_offset_in,
                                 self.neurones_caches[j] + pos_offset_out)

            peintre.drawEllipse(pos.x(), pos.y(),
                                TAILLE_NEURONE, TAILLE_NEURONE)

        # Dessine les neurones cachés.
        neurones = self.reseau.neurones

        for i in range(nombre_caches):
            pos = self.neurones_caches[i]
            neurone = clamp(neurones[i])
            peintre.setBrush(QtGui.QColor.fromRgbF(neurone, neurone, neurone))

            for j in range(nombre_sorties):
                peintre.drawLine(pos + pos_offset_in,
                                 self.neurones_sorties[j] + pos_offset_out)

            peintre.drawEllipse(pos.x(), pos.y(),
                                TAILLE_NEURONE, TAILLE_NEURONE)

        # Dessine les sorties.
        sorties = self.reseau.sorties

        for i in range(nombre_sorties):
            sortie = clamp(sorties[i])
            pos = self.neurones_sorties[i]

            peintre.setBrush(QtGui.QColor.fromRgbF(sortie, sortie, sortie))
            peintre.drawEllipse(pos.x(), pos.y(),
                                TAILLE_NEURONE, TAILLE_NEURONE)
