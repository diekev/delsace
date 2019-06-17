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
import time
import sys

from PyQt4 import QtCore, QtGui

app = QtGui.QApplication(sys.argv)


def time_dt():
    return int(round(time.time()))


class MainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        # Properties
        self.bpms = []
        self.bpm = 0.0
        self.average_bpm = 0.0
        self.start = 0.0
        self.number_of_beats = 0
        self.timer_started = False

        # UI
        self.grid_layout = QtGui.QGridLayout()
        self.help_label = QtGui.QLabel("Tapez 'B' pour commencer", self)
        self.bpm_label = QtGui.QLabel("BPM", self)
        self.bmp_number_label = QtGui.QLabel("0", self)
        self.average_label = QtGui.QLabel("Moyenne", self)
        self.avg_bmp_label = QtGui.QLabel("0", self)

        self.reset_button = QtGui.QPushButton("Reset", self)
        self.reset_button.pressed.connect(self.set_defaults)

        self.grid_layout.addWidget(self.help_label, 0, 1)
        self.grid_layout.addWidget(self.bpm_label, 1, 0)
        self.grid_layout.addWidget(self.bmp_number_label, 1, 1)
        self.grid_layout.addWidget(self.average_label, 2, 0)
        self.grid_layout.addWidget(self.avg_bmp_label, 2, 1)
        self.grid_layout.addWidget(self.reset_button, 3, 1)

        self.central_widget = QtGui.QWidget()
        self.central_widget.setLayout(self.grid_layout)

        self.setCentralWidget(self.central_widget)

    def set_defaults(self):
        self.bpms = []
        self.bpm = 0.0
        self.average_bpm = 0.0
        self.start = 0.0
        self.number_of_beats = 0
        self.timer_started = False
        self.bmp_number_label.setText("0")
        self.avg_bmp_label.setText("0")

    def keyPressEvent(self, e):
        if e.key() != QtCore.Qt.Key_B:
            return

        # Nous ajoutons 60 (secondes) pour éviter une multiplication
        self.number_of_beats += 60

        if self.timer_started is False:
            self.start = time_dt()
            self.timer_started = True
        else:
            delta = time_dt() - self.start

            if delta == 0.0:
                delta = 1.0

            self.bpm = self.number_of_beats / delta
            self.bmp_number_label.setText(QtCore.QString.number(self.bpm))

            self.bpms.append(self.bpm)
            self.average_bpm = sum(self.bpms) / len(self.bpms)
            self.avg_bmp_label.setText(QtCore.QString.number(self.average_bpm))


main_window = MainWindow()
main_window.setWindowTitle("BPM")
main_window.show()

sys.exit(app.exec_())
