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
from structures import Idee, Scenario
from utils import make_unicode


def cree_boite_bouton():
    boite = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel)

    boite.button(QtGui.QDialogButtonBox.Ok).setText("Accepter")
    boite.button(QtGui.QDialogButtonBox.Cancel).setText("Annuler l'action")

    return boite


class DialogueIdee(QtGui.QDialog):
    def __init__(self, parent=None):
        super(DialogueIdee, self).__init__(parent)

        self.setWindowTitle(make_unicode("Informations idéales"))
        self.setWindowFlags(QtCore.Qt.Dialog |
                            QtCore.Qt.WindowStaysOnTopHint |
                            QtCore.Qt.WindowTitleHint |
                            QtCore.Qt.CustomizeWindowHint)

        self.titre_edit = QtGui.QLineEdit(self)
        self.titre_label = QtGui.QLabel("Titre", self)

        self.type_edit = QtGui.QComboBox(self)
        self.type_edit.addItem("Battement d'action")
        self.type_edit.addItem("Blague")
        self.type_edit.addItem("Cadre")
        self.type_edit.addItem("Concept")
        self.type_edit.addItem("Dialogue")
        self.type_edit.addItem("Expression")
        self.type_edit.addItem("Location")
        self.type_edit.addItem("Personnage")
        self.type_edit.addItem(make_unicode("Scène"))
        self.type_edit.addItem("Site Internet")
        self.type_edit.addItem(make_unicode("Thème"))
        self.type_edit.addItem("Titre")

        self.type_label = QtGui.QLabel("Type", self)

        self.date_edit = QtGui.QDateEdit(self)
        self.date_edit.setDate(QtCore.QDate.currentDate())
        self.date_label = QtGui.QLabel(make_unicode("Date de création"), self)

        self.desc_edit = QtGui.QTextEdit(self)
        self.desc_label = QtGui.QLabel("Descrition", self)

        self.grid_layout = QtGui.QGridLayout(self)
        self.grid_layout.addWidget(self.titre_label, 0, 0)
        self.grid_layout.addWidget(self.titre_edit, 0, 1)
        self.grid_layout.addWidget(self.type_label, 1, 0)
        self.grid_layout.addWidget(self.type_edit, 1, 1)
        self.grid_layout.addWidget(self.date_label, 2, 0)
        self.grid_layout.addWidget(self.date_edit, 2, 1)
        self.grid_layout.addWidget(self.desc_label, 3, 0)
        self.grid_layout.addWidget(self.desc_edit, 3, 1)

        button_box = cree_boite_bouton()

        self.grid_layout.addWidget(button_box, 4, 1)

        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)

    def idee(self):
        idee = Idee()

        idee.nom = str(self.titre_edit.text())
        idee.genre = str(self.type_edit.currentText())
        idee.date_creation = str(self.date_edit.text())
        idee.desc = str(self.desc_edit.toPlainText())

        return idee


class DialogueScenario(QtGui.QDialog):
    def __init__(self, parent=None):
        super(DialogueScenario, self).__init__(parent)

        self.setWindowTitle(make_unicode("Information scénaristique"))
        self.setWindowFlags(QtCore.Qt.Dialog |
                            QtCore.Qt.WindowStaysOnTopHint |
                            QtCore.Qt.WindowTitleHint |
                            QtCore.Qt.CustomizeWindowHint)

        self.name_edit = QtGui.QLineEdit(self)
        self.name_label = QtGui.QLabel("Name", self)

        self.author_edit = QtGui.QLineEdit(self)
        self.author_label = QtGui.QLabel("Auteur", self)

        self.date_edit = QtGui.QDateEdit(self)
        self.date_edit.setDate(QtCore.QDate.currentDate())
        self.date_label = QtGui.QLabel(make_unicode("Date de création"), self)

        self.genre_edit = QtGui.QComboBox(self)
        self.genre_edit.addItem("Action")
        self.genre_edit.addItem("Adolescent")
        self.genre_edit.addItem("Animation")
        self.genre_edit.addItem("Aventure")
        self.genre_edit.addItem(make_unicode("Comédie"))
        self.genre_edit.addItem(make_unicode("Crime/Détective"))
        self.genre_edit.addItem("Documentaire")
        self.genre_edit.addItem("Drame")
        self.genre_edit.addItem(make_unicode("Dramédie"))
        self.genre_edit.addItem("Horreur")
        self.genre_edit.addItem("Manga")
        self.genre_edit.addItem("Science-Fiction")
        self.genre_edit.addItem("Suspense")
        self.genre_edit.addItem("Thriller")

        self.genre_label = QtGui.QLabel("Genre", self)

        self.source_edit = QtGui.QComboBox(self)
        self.source_edit.addItem("Article - Journal")
        self.source_edit.addItem("Article - Magazine")
        self.source_edit.addItem("Contacte Personnel")
        self.source_edit.addItem("Histoire Vraie")
        self.source_edit.addItem(make_unicode("Idée Originale"))
        self.source_edit.addItem("Livre/Histoire")
        self.source_edit.addItem("Remake")
        self.source_edit.addItem("Site Internet")

        self.source_label = QtGui.QLabel("Source", self)

        self.grid_layout = QtGui.QGridLayout(self)
        self.grid_layout.addWidget(self.name_label, 0, 0)
        self.grid_layout.addWidget(self.name_edit, 0, 1)
        self.grid_layout.addWidget(self.author_label, 1, 0)
        self.grid_layout.addWidget(self.author_edit, 1, 1)
        self.grid_layout.addWidget(self.date_label, 2, 0)
        self.grid_layout.addWidget(self.date_edit, 2, 1)
        self.grid_layout.addWidget(self.genre_label, 3, 0)
        self.grid_layout.addWidget(self.genre_edit, 3, 1)
        self.grid_layout.addWidget(self.source_label, 4, 0)
        self.grid_layout.addWidget(self.source_edit, 4, 1)

        button_box = cree_boite_bouton()

        self.grid_layout.addWidget(button_box, 5, 1)

        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)

    def scenario(self):
        scenario = Scenario().create_default()

        scenario.author = str(self.author_edit.text())
        scenario.genre = str(self.genre_edit.currentText())
        scenario.source = str(self.source_edit.currentText())
        scenario.name = str(self.name_edit.text())
        scenario.start_date = str(self.date_edit.text())

        return scenario
