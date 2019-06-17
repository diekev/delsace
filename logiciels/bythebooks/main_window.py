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

from dialogs import DialogueIdee, DialogueScenario
from plot_point_widget import PlotPointWidget
from PyQt4 import QtCore, QtGui
from structures import Idee, Projet, Scenario
from screenplay_outline import outline_points
from tables import TableIdee, TableScenario
from utils import make_unicode


class MainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        self.projet = Projet()
        self.outline_text_area = []

        self.generate_file_menu()
        self.generate_help_menu()

        widget = QtGui.QWidget()
        self.vlayout = QtGui.QVBoxLayout(widget)

        scroll = QtGui.QScrollArea()
        scroll.setWidget(widget)
        scroll.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAsNeeded)
        scroll.setWidgetResizable(True)

        for point in outline_points:
            widget_point = PlotPointWidget(point.title, point.desc, self)
            self.outline_text_area.append(widget_point)
            self.vlayout.addWidget(widget_point)

        self.setCentralWidget(scroll)

        dock = QtGui.QDockWidget(make_unicode("Progès"), self)
        self.addDockWidget(QtCore.Qt.TopDockWidgetArea, dock)

        # Dock pour les idées.
        self.idee_table = TableIdee()

        dock = QtGui.QDockWidget(make_unicode("Idées"), self)
        dock.setWidget(self.idee_table)

        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, dock)

        # Dock pour les scénarios.
        self.scenario_table = TableScenario()

        dock = QtGui.QDockWidget(make_unicode("Scénarios"), self)
        dock.setWidget(self.scenario_table)

        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, dock)

    def generate_file_menu(self):
        menu = self.menuBar().addMenu("Fichier")

        action = menu.addAction(make_unicode("Nouvelle idée..."))
        action.triggered.connect(self.montre_dialogue_idee)

        action = menu.addAction(make_unicode("Nouveau scénario..."))
        action.triggered.connect(self.montre_dialogue_scenario)

        action = menu.addAction(make_unicode("Supprimer scénario..."))
        action.triggered.connect(self.montre_dialogue_suppression_scenario)

    def montre_dialogue_idee(self):
        dialog = DialogueIdee(self)
        dialog.show()

        if dialog.exec_() == QtGui.QDialog.Accepted:
            idee = dialog.idee()
            self.projet.idees.append(idee)
            self.idee_table.update_table(self.projet.idees)

    def montre_dialogue_scenario(self):
        dialog = DialogueScenario(self)
        dialog.show()

        if dialog.exec_() == QtGui.QDialog.Accepted:
            scenario = dialog.scenario()
            self.projet.scenarios.append(scenario)
            self.scenario_table.update_table(self.projet.scenarios)

    def montre_dialogue_suppression_scenario(self):
        titre = make_unicode("Supprimer scénario")
        message = make_unicode("Voulez-vous vraiment supprimer le scénario ?")

        ret = QtGui.QMessageBox.warning(self,
                                        titre,
                                        message,
                                        QtGui.QMessageBox.Ok)

        if ret == QtGui.QMessageBox.Ok:
            return

    def generate_help_menu(self):
        menu = self.menuBar().addMenu("Aide")

        action = menu.addAction(make_unicode("À propos de ByTheBooks..."))
        action.triggered.connect(self.montre_dialogue_aide)

    def montre_dialogue_aide(self):
        titre = make_unicode("À propos de ByTheBooks")
        message = "ByTheBooks offre une approche multi-étapes pour produire" \
                  " un schéma structurellement solide à partir de laquelle" \
                  " écrire des scénarios, guidant l'écrivain de l'idée de" \
                  " scénario au battement final de l'acte III."

        QtGui.QMessageBox.about(self, titre, make_unicode(message))
