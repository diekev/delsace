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

from breakout import BreakOut, WIDTH, HEIGHT
from PyQt4 import QtCore
import numpy as np
from reseau_neuronal import ReseauNeuronal, accouple_reseaux

import time


class ADN():
    def __init__(self, reseau):
        self.reseau = reseau
        self.fitness = 0.0


def compare_adn(lhs, rhs):
    return lhs.fitness > rhs.fitness


class Genetique(QtCore.QThread):
    update_display = QtCore.pyqtSignal()

    def __init__(self, taille_population, game):
        super(Genetique, self).__init__()

        self.taille_population = taille_population
        self.game = game
        self.population = []
        self.mating_pool = []
        self.entrees = [ 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 ]
        self.neurones = [ 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 ]
        self.sorties = [ 1.0, 1.0 ]
        self.generation = 0
        self.individual = 0

        self.timer = QtCore.QTimer();
        self.timer.timeout.connect(self.timeout)

        for i in range(taille_population):
            adn = ADN(ReseauNeuronal())
            self.population.append(adn)

    def timeout(self):
        if self.game.score == self.game.previous_score:
            self.timer.stop()
            self.game.game_over = True

    def run(self):
        for i in range(100):
            self.generation = i
            self.compute_fitness()
            self.generate_mating_pool()
            self.save_best()
            self.mate()

    def compute_fitness(self):
        game = self.game
        index = 0

        for individu in self.population:
            self.individual = index
            index += 1
            reseau = individu.reseau
            self.timer.stop()
            self.timer.start(25000)

            while game.game_over is False:
                # Joue le jeu.
                game.move_ball()

                # Passe les donnée au réseau neuronal.

                dist_x = game.ball.loc_x - game.paddle.loc_x
                dist_y = abs(game.ball.loc_y - game.paddle.loc_y)

                self.entrees = [
                        dist_x,
                        dist_y,
                        game.vx,
                        game.vy,
                        game.paddle.loc_x / WIDTH,
                        1.0,
                        ]

                reseau.avance(self.entrees)

                self.neurones = reseau.neurones

                # Utilise la sortie pour bouger la raquette
                self.sorties = reseau.sorties

                if self.sorties[0] > self.sorties[1]:
                    game.move_paddle(-30 * self.sorties[0])
                else:
                    game.move_paddle(30 * self.sorties[1])

                self.update_display.emit()
                if self.generation >= 0:
                    time.sleep(1.0 / 180)

            individu.fitness = game.score
            game.reset_game()

    def generate_mating_pool(self):
        self.mating_pool = []

        self.population.sort(compare_adn)

        for i in range(4):
            self.mating_pool.append(self.population[i])

    def mate(self):
        population = []
        mate0 = self.mating_pool[0]

        for mate1 in self.mating_pool:
            for i in range(self.taille_population / 4):
                descendant = accouple_reseaux(mate0.reseau, mate1.reseau)
                population.append(ADN(descendant))

        self.population = population

    def save_best(self):
        best = self.mating_pool[0]

        np.savetxt("reseaux/reseau_ec_gen%d.txt" % (self.generation),
                   best.reseau.entrees_couches)
        np.savetxt("reseaux/reseau_cs_gen%d.txt" % (self.generation),
                   best.reseau.couches_sorties)
