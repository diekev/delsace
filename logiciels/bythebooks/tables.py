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


class TableIdee(QtGui.QWidget):
    def __init__(self, parent=None):
        super(TableIdee, self).__init__(parent)

        header_titles = QtCore.QStringList()
        header_titles.append("Nom")
        header_titles.append("Type")

        self.table = QtGui.QTableWidget(self)
        self.table.setRowCount(10)
        self.table.setColumnCount(2)
        self.table.setHorizontalHeaderLabels(header_titles)

        self.vlayout = QtGui.QVBoxLayout(self)
        self.vlayout.addWidget(self.table)

    def update_table(self, idees):
        index = 0
        for idee in idees:
            self.table.setItem(index, 0, QtGui.QTableWidgetItem(idee.nom))
            self.table.setItem(index, 1, QtGui.QTableWidgetItem(idee.genre))
            index += 1


class TableScenario(QtGui.QWidget):
    def __init__(self, parent=None):
        super(TableScenario, self).__init__(parent)

        header_titles = QtCore.QStringList()
        header_titles.append("Nom")
        header_titles.append("Genre")

        self.table = QtGui.QTableWidget(self)
        self.table.setRowCount(10)
        self.table.setColumnCount(2)
        self.table.setHorizontalHeaderLabels(header_titles)

        self.vlayout = QtGui.QVBoxLayout(self)
        self.vlayout.addWidget(self.table)

    def update_table(self, scenarios):
        index = 0
        for scenario in scenarios:
            self.table.setItem(index, 0, QtGui.QTableWidgetItem(scenario.name))
            self.table.setItem(index, 1, QtGui.QTableWidgetItem(scenario.genre))
            index += 1
