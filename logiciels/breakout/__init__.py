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

import sys

from PyQt4 import QtGui

app = QtGui.QApplication(sys.argv)

# Nous devons créer l'application avant d'importer BreakOut, puisque BreakOut
# crée des objets utilisant Phonon qui à son tour doit créer une session D-Bus.
# L'application doit être créée avant la session D-Bus pour pouvoir exporter de
# l'audio à travers l'interface DBUS.
from main_window import MainWindow

w = MainWindow()
w.setWindowTitle("BreakOut")
w.show()
w.start_genetique()

sys.exit(app.exec_())
