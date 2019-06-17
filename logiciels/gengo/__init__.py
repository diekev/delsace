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

"""Gengo.

Usage:
    __init__.py
    __init__.py --entrainement
    __init__.py (-h | --help)
    __init__.py --version

Options:
    -h, --help      Montre cet écran.
    --version       Montre la version du logiciel.
    --entrainement  Démarre l'entrainement du réseau neuronal.

"""

import sys

from docopt import docopt
from PyQt4 import QtGui
from main_window import MainWindow
from utils import make_unicode

reload(sys)
sys.setdefaultencoding('utf-8')

if __name__ == '__main__':
    arguments = docopt(__doc__, version='Gengo 0.1')

    app = QtGui.QApplication(sys.argv)

    w = MainWindow()
    w.setWindowTitle(make_unicode("言語"))
    w.show()

    if arguments["--entrainement"] is True:
        w.entraine_reseau()

    sys.exit(app.exec_())
