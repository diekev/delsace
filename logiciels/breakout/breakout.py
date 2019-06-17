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

import PyQt4
import random

from PyQt4 import QtCore, QtGui, QtWebKit
from sound import Sound

# Largeur et hauteur de la fenêtre en pixels.
APPLICATION_WIDTH = 600
APPLICATION_HEIGHT = 900

# Largeur et hauteur de la zone de jeu en pixels.
WIDTH = APPLICATION_WIDTH
HEIGHT = APPLICATION_HEIGHT

# Largeur et hauteur de la raquette en pixels.
PADDLE_WIDTH = APPLICATION_HEIGHT / 10
PADDLE_HEIGHT = PADDLE_WIDTH / 6

# Espace entre la raquette et le bas de l'écran.
PADDLE_Y_OFFSET = APPLICATION_HEIGHT / 20

# Nombre de briques par rangée.
NBRICKS_PER_ROW = 10

# Nombre de rangées de briques.
NBRICKS_ROWS = 10

# Espace entre les briques.
BRICK_SEP = APPLICATION_WIDTH / 100

# Largeur d'une brique.
BRICK_WIDTH = (WIDTH - (NBRICKS_PER_ROW - 1) * BRICK_SEP) / NBRICKS_PER_ROW

# Hauteur d'une brique.
BRICK_HEIGHT = BRICK_SEP * 2

# Rayon de la balle.
BALL_RADIUS = APPLICATION_WIDTH / 40

# Espace entre la rangée de brique du haut et le haut de l'écran.
BRICK_Y_OFFSET = APPLICATION_HEIGHT / 10

# Nombre de tours.
NTURNS = 3


def get_color(index):
    # Rouge.
    if index is 0 or index is 1:
        return QtGui.QColor.fromRgbF(0.659, 0.157, 0.106)

    # Orange.
    if index is 2 or index is 3:
        return QtGui.QColor.fromRgbF(0.722, 0.302, 0.106)

    # Jaune.
    if index is 4 or index is 5:
        return QtGui.QColor.fromRgbF(0.827, 0.686, 0.137)

    # Vert.
    if index is 6 or index is 7:
        return QtGui.QColor.fromRgbF(0.063, 0.506, 0.106)

    # Bleu.
    if index is 8 or index is 9:
        return QtGui.QColor.fromRgbF(0.027, 0.400, 0.600)

    # Gris.
    if index is 10:
        return QtGui.QColor.fromRgbF(0.047, 0.514, 0.914)

    return QtGui.QColor.fromRgbF(0.6, 0.6, 0.6)


sounds = {
        'wall_bounce': Sound("audio/wall_collision.wav"),
        'red': Sound("audio/brick_collision_red.wav"),
        'orange': Sound("audio/brick_collision_orange.wav"),
        'yellow': Sound("audio/brick_collision_yellow.wav"),
        'green': Sound("audio/brick_collision_blue.wav"),
        'blue': Sound("audio/brick_collision_blue.wav"),
        'paddle': Sound("audio/paddle_collision.wav"),
        'game_start': Sound("audio/game_start.wav"),
}


def play_brick_sound(index):
    if index is 0 or index is 1:
        sounds['red'].play()

    if index is 2 or index is 3:
        sounds['orange'].play()

    if index is 4 or index is 5:
        sounds['yellow'].play()

    if index is 6 or index is 7:
        sounds['green'].play()

    if index is 8 or index is 9:
        sounds['blue'].play()

    if index is 10:
        sounds['paddle'].play()


class Rect():
    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height


def check_collision(rect1, rect2):
    return rect1.x < rect2.x + rect2.width and \
           rect1.x + rect1.width > rect2.x and \
           rect1.y < rect2.y + rect2.height and \
           rect1.y + rect1.height > rect2.y


class Brick():
    def __init__(self, index, loc_x, loc_y):
        self.color = get_color(index)
        self.color_index = index
        self.loc_x = loc_x
        self.loc_y = loc_y
        self.width = BRICK_WIDTH
        self.height = BRICK_HEIGHT

    def get_rect(self):
        return Rect(self.loc_x, self.loc_y, self.width, self.height)

class BreakOut(QtCore.QThread):
    loop_finished = QtCore.pyqtSignal()

    def __init__(self):
        super(BreakOut, self).__init__()
        self.bricks = []

        self.lives = NTURNS
        self.num_bricks = NBRICKS_PER_ROW * NBRICKS_ROWS
        self.score = 0
        self.best_score = 0
        self.previous_score = 0
        self.game_over = False

        self.paddle = Brick(10,
                            (WIDTH - PADDLE_WIDTH) / 2,
                            HEIGHT - PADDLE_Y_OFFSET)
        self.paddle.width = PADDLE_WIDTH
        self.paddle.height = PADDLE_HEIGHT

        self.ball = Brick(-1,
                          (WIDTH - PADDLE_WIDTH) / 2,
                          HEIGHT - (BALL_RADIUS + PADDLE_Y_OFFSET))
        self.ball.width = BALL_RADIUS
        self.ball.height = BALL_RADIUS

        self.vx = 1.0 * 2
        self.vy = -3.0 * 2

        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.move_ball)

        self.init_bricks()

        self.paddle_collision = 0
        self.red_collision = 0
        self.orange_collision = 0
        self.play_sounds = False

    def reset_game(self):
        self.lives = NTURNS
        self.num_bricks = NBRICKS_PER_ROW * NBRICKS_ROWS

        if self.score > self.best_score:
            self.best_score = self.score

        self.score = 0
        self.previous_score = 0
        self.game_over = False

        self.paddle.loc_x = (WIDTH - PADDLE_WIDTH) / 2
        self.paddle.loc_y = HEIGHT - PADDLE_Y_OFFSET

        self.ball.loc_x = (WIDTH - PADDLE_WIDTH) / 2
        self.ball.loc_y = HEIGHT - (BALL_RADIUS + PADDLE_Y_OFFSET)

        self.vx = 1.0 * 2
        self.vy = -3.0 * 2

        self.init_bricks()

        self.paddle_collision = 0
        self.red_collision = 0
        self.orange_collision = 0

    def run(self):
        self.timer.start(1000 / 60)

        if self.play_sounds:
            sounds['game_start'].play()

    def init_bricks(self):
        self.bricks = []
        loc_y = BRICK_Y_OFFSET
        start_x = (WIDTH - (NBRICKS_PER_ROW * BRICK_WIDTH) -
                   (NBRICKS_PER_ROW - 1) * BRICK_SEP) / 2

        for i in range(NBRICKS_ROWS):
            loc_x = start_x
            col = get_color(i)

            for j in range(NBRICKS_PER_ROW):
                self.bricks.append(Brick(i, loc_x, loc_y))
                loc_x += BRICK_WIDTH + BRICK_SEP

            loc_y += BRICK_HEIGHT + BRICK_SEP

    def move_ball(self):
        if self.lives == 0 or self.num_bricks == 0:
            self.timer.stop()
            self.game_over = True
            self.send_signal()
            return

        if self.ball.loc_y > HEIGHT:
            self.lives -= 1
            self.ball.loc_x = (WIDTH - BALL_RADIUS) / 2
            self.ball.loc_y = HEIGHT - (BALL_RADIUS + PADDLE_Y_OFFSET)
            self.vy = -self.vy
            self.vx = random.uniform(0.1, 3.0)

            if random.random() < 0.5:
                self.vx = -self.vx

            self.paddle_collision = 0
            self.red_collsion = 0
            self.orange_collision = 0
            if self.play_sounds:
                sounds['game_start'].play()

        self.ball.loc_x += self.vx
        self.ball.loc_y += self.vy

        if self.ball.loc_x < 0 or self.ball.loc_x > (WIDTH - BALL_RADIUS):
            if self.play_sounds:
                sounds['wall_bounce'].play()

            self.vx = -self.vx

        if self.ball.loc_y < 0:
            if self.play_sounds:
                sounds['wall_bounce'].play()

            self.vy = -self.vy

        collider = self.get_collider()

        if collider is not None:
            if self.play_sounds:
                play_brick_sound(collider.color_index)

            self.vy = -self.vy

            if collider is not self.paddle:
                self.num_bricks -= 1
                self.score += 10 - collider.color_index
                self.bricks.remove(collider)

                index = collider.color_index

                if index is 0 or index is 1:
                    self.red_collision += 1

                if index is 2 or index is 3:
                    self.orange_collision += 1
            else:
                self.previous_score = self.score
                self.paddle_collision += 1
                self.handle_paddle_collision()

                speed_up = False

                if self.paddle_collision == 5:
                    speed_up = True

                if self.paddle_collision == 12:
                    speed_up = True

                if self.red_collision == 1:
                    # Incrémente pout déactiver la fonctionalité
                    self.red_collision = 2
                    speed_up = True

                if self.orange_collision == 1:
                    # Incrémente pout déactiver la fonctionalité
                    self.orange_collision = 2
                    speed_up = True

                if speed_up is True:
                    self.vx *= 1.25
                    self.vy *= 1.25

            if self.num_bricks == 0 and self.lives > 1:
                self.game_over = True

        self.send_signal()

    def send_signal(self):
        self.loop_finished.emit()

    def get_collider(self):
        ball_rect = self.ball.get_rect()

        # Check collision with paddle
        if check_collision(ball_rect, self.paddle.get_rect()):
            return self.paddle

        # Check collision with bricks
        for brick in self.bricks:
            if check_collision(ball_rect, brick.get_rect()):
                return brick

        return None

    def handle_paddle_collision(self):
        ball_x = self.ball.loc_x
        paddle_middle_x = self.paddle.loc_x + (PADDLE_WIDTH / 2)

        diff_x = abs(ball_x - paddle_middle_x) / (PADDLE_WIDTH / 2)

        if ball_x < (paddle_middle_x - 5):
            self.vx = -(abs(self.vx) + diff_x)
        elif ball_x > (paddle_middle_x + 5):
            self.vx = (abs(self.vx) + diff_x)
        else:
            self.vx = -self.vx

    def move_paddle(self, delta):
        if self.lives is not 0 and (self.num_bricks * 2 > 0):
            self.paddle.loc_x += delta

            if self.paddle.loc_x < 0:
                self.paddle.loc_x = 0
            elif self.paddle.loc_x > WIDTH - PADDLE_WIDTH:
                self.paddle.loc_x = WIDTH - PADDLE_WIDTH
