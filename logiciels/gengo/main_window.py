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

from fenetre_reseau import DessinateurReseau
from langues import defini_langue
from PyQt4 import QtGui
from reseau_neuronal import ReseauNeuronal, EntraineurReseau
from utils import convertie_mot, make_unicode


class MainWindow(QtGui.QWidget):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        self.reseau_neuronal = ReseauNeuronal()
        self.entraineur = EntraineurReseau(self.reseau_neuronal)

        self.line_edit_label = QtGui.QLabel("Mot")
        self.line_edit = QtGui.QLineEdit(self)
        self.line_edit.editingFinished.connect(self.passe_mot)

        self.line_edit_layout = QtGui.QHBoxLayout()
        self.line_edit_layout.addWidget(self.line_edit_label)
        self.line_edit_layout.addWidget(self.line_edit)

        self.fenetre_reseau = DessinateurReseau(self.reseau_neuronal, self)

        self.guess_label = QtGui.QLabel(self)

        self.vlayout = QtGui.QVBoxLayout(self)
        self.vlayout.addLayout(self.line_edit_layout)
        self.vlayout.addWidget(self.fenetre_reseau)
        self.vlayout.addWidget(self.guess_label)

    def entraine_reseau(self):
        self.entraineur.entraine()

    def passe_mot(self):
        mot = str(self.line_edit.text())

        if len(mot) is 0:
            return

        donnees_entrees = convertie_mot(mot)

        #print donnees_entrees
        self.reseau_neuronal.avance(donnees_entrees)

        #print "Résultat:", self.reseau_neuronal.sorties
        langue_possible = defini_langue(self.reseau_neuronal.sorties)

        texte = "Langue possible: %s, confidence: %f."
        texte = texte % (langue_possible['langue'],
                         langue_possible['confidence'])

        self.guess_label.setText(make_unicode(texte))

        self.update()
