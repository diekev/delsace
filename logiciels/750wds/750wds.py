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


class MainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        self.stat_str = QtCore.QString("Mots par minute: %1.")
        self.label_str = QtCore.QString("Il y a %1 mots dans le document.")

        self.label = QtGui.QLabel(self)
        self.stat = QtGui.QLabel(self)
        self.timer = QtCore.QTimer()
        self.central_widget = QtGui.QWidget(self)
        self.text_edit = QtGui.QTextEdit(self)
        self.grid_layout = QtGui.QGridLayout(self.central_widget)

        self.word_count = 0
        self.seconds = 0
        self.words_minute = 0.0

        self.setCentralWidget(self.central_widget)

        self.update_stat()
        self.update_label()

        self.grid_layout.addWidget(self.text_edit, 0, 0)
        self.grid_layout.addWidget(self.label, 1, 0)
        self.grid_layout.addWidget(self.stat, 2, 0)

        self.timer.timeout.connect(self.count_time)
        self.text_edit.textChanged.connect(self.count_words)
        self.text_edit.textChanged.connect(self.word_per_minute)

        self.timer.start(1000)

    def update_stat(self):
        text = QtCore.QString.number(self.words_minute, 'f', 1)
        self.stat.setText(self.stat_str.arg(text))

    def update_label(self):
        text = QtCore.QString.number(self.word_count)
        self.label.setText(self.stat_str.arg(text))

    def keyPressEvent(self, e):
        if e.key() != QtCore.Qt.Key_F11:
            return

        self.setWindowState(self.windowState() ^ QtCore.Qt.WindowFullScreen)
        self.label.setHidden(not self.label.isHidden())
        self.stat.setHidden(not self.stat.isHidden())

    def count_time(self):
        self.seconds += 1

    def count_words(self):
        text = self.text_edit.toPlainText()
        self.word_wount = text.count(' ')

        # Nous avons toujours un mot de plus que d'espaces, mais nous
        # incrémentons quand l'utilisateur commence à dactylographier.
        if text.length() > 0:
            # Ne prenons pas en compte les espaces traînantes.
            if text[text.length() - 1] != ' ':
                self.word_count += 1

        self.update_label()

    def word_per_minute(self):
        self.words_minute = float(self.word_count) / float(self.seconds) * 60.0
        self.update_stat()


main_window = MainWindow()
main_window.setWindowTitle("750 Words")
main_window.show()

sys.exit(app.exec_())
