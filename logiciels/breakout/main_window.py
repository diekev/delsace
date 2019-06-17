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

from PyQt4 import QtCore, QtGui

from breakout import BreakOut, WIDTH, HEIGHT, BALL_RADIUS
from genetique import Genetique

# Stylo utilisé pour déssiner le texte.
stylo_blanc = QtGui.QPen(QtGui.QColor.fromRgbF(1.0, 1.0, 1.0))

def clamp(x):
    if x < 0.0:
        return 0.0

    if x > 1.0:
        return 1.0

    return x


class MainWindow(QtGui.QWidget):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        self.game = BreakOut()
        self.game.loop_finished.connect(self.handle_signal)

        self.genetique = Genetique(5, self.game)
        self.genetique.update_display.connect(self.handle_signal)

        font = QtGui.QFont("helvetica", 16)
        self.font_metrics = QtGui.QFontMetrics(font)

        self.setGeometry(200, 200, WIDTH * 2, HEIGHT)

        self.neurones_entrees = []
        self.neurones_caches = []
        self.neurones_sorties = []

        self.initialise_neurones()

        self.update()

    def initialise_neurones(self):
        half_width = WIDTH / 2
        start_x = WIDTH + WIDTH / 4
        start_y = ((half_width - (9 * BALL_RADIUS)) / 2)

        for i in range(5):
            pos = QtCore.QPoint(start_x, start_y)
            self.neurones_entrees.append(pos)
            start_y += BALL_RADIUS * 2

        start_x += WIDTH / 4
        start_y = ((half_width - (13 * BALL_RADIUS)) / 2)

        for i in range(7):
            pos = QtCore.QPoint(start_x, start_y)
            self.neurones_caches.append(pos)

            start_y += BALL_RADIUS * 2

        start_x += WIDTH / 4
        start_y = ((half_width - (3 * BALL_RADIUS)) / 2)

        for i in range(2):
            pos = QtCore.QPoint(start_x, start_y)
            self.neurones_sorties.append(pos)

            start_y += BALL_RADIUS * 2

    def handle_signal(self):
        self.update()

    def start_game(self):
        self.game.start()

    def start_genetique(self):
        self.genetique.start()

    def paintEvent(self, e):
        qp = QtGui.QPainter()
        qp.begin(self)
        self.draw(qp)
        qp.end()

    def draw(self, painter):
        # Peint l'arrière-plan.
        painter.setBrush(QtGui.QColor.fromRgbF(0.09, 0.09, 0.09))
        painter.drawRect(0, 0, WIDTH, HEIGHT)
        painter.drawRect(WIDTH, 0, WIDTH, HEIGHT)

        # Peint l'état du jeu si fini.
        if self.game.game_over is True:
            text = QtCore.QString("")

            if self.game.lives != 0:
                text += "Yay! I guess that's a win! Score: "
                text += QtCore.QString.number(self.game.score)
                text += QtCore.QString(", lives: ")
                text += QtCore.QString.number(self.game.lives)
            else:
                text += "Game Over, Man, Game Over..."

            painter.setPen(stylo_blanc)
            painter.drawText((WIDTH - self.font_metrics.width(text) / 2) / 2,
                             (HEIGHT - self.font_metrics.height()) / 2,
                             text)
            return

        painter.setBrush(self.game.ball.color)
        painter.drawEllipse(self.game.ball.loc_x,
                            self.game.ball.loc_y,
                            self.game.ball.width,
                            self.game.ball.height)

        painter.setBrush(self.game.paddle.color)
        painter.drawRect(self.game.paddle.loc_x,
                         self.game.paddle.loc_y,
                         self.game.paddle.width,
                         self.game.paddle.height)

        for brick in self.game.bricks:
            painter.setBrush(brick.color)
            painter.drawRect(brick.loc_x,
                             brick.loc_y,
                             brick.width,
                             brick.height)

        self.peint_neurones(painter)
        self.peint_texte(painter)

    def peint_neurones(self, painter):
        # Peint les neurones d'entrées

        # Entrées:
        #  - balle x
        #  - balle y
        #  - vélocité x
        #  - vélocité y
        #  - raquette x

        stylo = QtGui.QPen(QtGui.QColor.fromRgbF(0.25, 0.25, 0.25, 0.5))
        painter.setPen(stylo)
        pos_offset_in = QtCore.QPoint(14, 7)
        pos_offset_out = QtCore.QPoint(0, 7)

        # Dessine les entrées.
        entrees = self.genetique.entrees

        for i in range(5):
            pos = self.neurones_entrees[i]
            entree = clamp(entrees[i])
            painter.setBrush(QtGui.QColor.fromRgbF(entree, entree, entree))

            for j in range(7):
                painter.drawLine(pos + pos_offset_in,
                                 self.neurones_caches[j] + pos_offset_out)

            painter.drawEllipse(pos.x(), pos.y(), BALL_RADIUS, BALL_RADIUS)

        # Dessine les neurones cachés.
        neurones = self.genetique.neurones

        for i in range(7):
            pos = self.neurones_caches[i]
            neurone = clamp(neurones[i])
            painter.setBrush(QtGui.QColor.fromRgbF(neurone, neurone, neurone))

            for j in range(2):
                painter.drawLine(pos + pos_offset_in,
                                 self.neurones_sorties[j] + pos_offset_out)

            painter.drawEllipse(pos.x(), pos.y(), BALL_RADIUS, BALL_RADIUS)

        # Dessine les sorties.
        sorties = self.genetique.sorties

        for i in range(2):
            sortie = clamp(sorties[i])
            pos = self.neurones_sorties[i]

            painter.setBrush(QtGui.QColor.fromRgbF(sortie, sortie, sortie))
            painter.drawEllipse(pos.x(), pos.y(), BALL_RADIUS, BALL_RADIUS)

    def peint_texte_impl(self, peintre, texte, valeur, decalage):
        text = QtCore.QString(texte)
        text += QtCore.QString.number(valeur)

        peintre.drawText(WIDTH + 50,
                         WIDTH + self.font_metrics.height() + decalage,
                         text)

    def peint_texte(self, peintre):
        peintre.setPen(stylo_blanc)

        self.peint_texte_impl(peintre, "Score:      ", self.game.score, 0)
        self.peint_texte_impl(peintre, "Best Score: ", self.game.best_score, 15)
        self.peint_texte_impl(peintre, "Lives: ", self.game.lives, 45)
        self.peint_texte_impl(peintre, "Generation: ", self.genetique.generation, 75)
        self.peint_texte_impl(peintre, "Individual: ", self.genetique.individual, 90)

    def keyPressEvent(self, event):
        delta = 30

        if event.key() == QtCore.Qt.Key_Right:
            self.game.move_paddle(delta)
        elif event.key() == QtCore.Qt.Key_Left:
            self.game.move_paddle(-delta)
