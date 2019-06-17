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
import sys

from PyQt4 import QtGui, QtCore

app = QtGui.QApplication(sys.argv)


class Timer(QtGui.QWidget):
    def __init__(self, hours, minutes, seconds, count_up, parent=None):
        super(Timer, self).__init__(parent)

        self.label = QtGui.QLabel(self)
        self.timer = QtCore.QTimer(self)
        self.start_but = QtGui.QPushButton("Start", self)
        self.reset_but = QtGui.QPushButton("Reset", self)
        self.layout = QtGui.QHBoxLayout()

        self.hours = 0
        self.minutes = 0
        self.seconds = 0

        # original starting values,
        # used to reset timer to whatever values it was initialized with
        self.start_h = 0
        self.start_m = 0
        self.start_s = 0

        # defines whether we count up and down
        self.count_up = count_up

        # set layout
        self.set_time(hours, minutes, seconds)

        self.layout.addWidget(self.label)
        self.layout.addWidget(self.start_but)
        self.layout.addWidget(self.reset_but)

        self.setLayout(self.layout)

        self.timer.timeout.connect(self.count)
        self.start_but.clicked.connect(self.start)
        self.reset_but.clicked.connect(self.reset_time)

    def count(self):
        if self.count_up:
            self.count_up()
        else:
            self.count_down()

        self.update_display()

    def update_display(self):
        h = QtCore.QString("%1").arg(self.hours, 2, 10, QtCore.QChar('0'))
        m = QtCore.QString("%1").arg(self.minutes, 2, 10, QtCore.QChar('0'))
        s = QtCore.QString("%1").arg(self.seconds, 2, 10, QtCore.QChar('0'))

        self.label.setText(h + " : " + m + " : " + s)

    def count_up(self):
        self.seconds += 1

        if self.seconds == 60:
            self.minutes += 1
            self.seconds = 0

        if self.minutes == 60:
            self.hours += 1
            self.minutes = 0

        if self.hours == 100:
            self.hours = 0

    def count_down(self):
        self.seconds -= 1

        if (self.seconds < 0):
            if (self.minutes > 0):
                self.minutes -= 1
                self.seconds = 59
            else:
                self.seconds = 0

        if (self.minutes == 0):
            if (self.hours > 0):
                self.hours -= 1
                self.minutes = 59

    def start(self):
        if self.timer.isActive():
            self.timer.stop()
            self.start_but.setText("Start")
        else:
            self.timer.start(1000)
            self.start_but.setText("Stop")

    def set_time(self, hours, minutes, seconds):
        self.hours = self.start_h = hours
        self.minutes = self.start_m = minutes
        self.seconds = self.start_s = seconds

        self.update_display()

    def reset_time(self):
        self.hours = self.start_h
        self.minutes = self.start_m
        self.seconds = self.start_s

        self.update_display()


class MainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        self.central_widget = QtGui.QWidget(self)
        self.grid_layout = QtGui.QVBoxLayout(self.central_widget)
        self.setCentralWidget(self.central_widget)

        self.pomodoro = Timer(0, 25, 0, False)
        self.pause = Timer(0, 5, 0, False)
        self.quickie = Timer(0, 2, 0, False)

        self.grid_layout.addWidget(self.pomodoro)
        self.grid_layout.addWidget(self.pause)
        self.grid_layout.addWidget(self.quickie)


main_window = MainWindow()
main_window.setWindowTitle("Timer")
main_window.show()

sys.exit(app.exec_())
