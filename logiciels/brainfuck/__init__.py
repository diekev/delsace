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

from PyQt4 import QtGui
from subprocess import check_output

import subprocess
import sys

app = QtGui.QApplication(sys.argv)


class MainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)

        self.display_text = QtGui.QTextEdit(self)
        self.text_edit = QtGui.QTextEdit(self)

        self.hlayout = QtGui.QHBoxLayout()
        self.hlayout.addWidget(self.text_edit)
        self.hlayout.addWidget(self.display_text)

        self.central_widget = QtGui.QWidget(self)

        self.interpret_button = QtGui.QPushButton("Interpret")
        self.interpret_button.pressed.connect(self.interpret)

        self.vlayout = QtGui.QVBoxLayout(self.central_widget)
        self.vlayout.addLayout(self.hlayout)
        self.vlayout.addWidget(self.interpret_button)

        self.setCentralWidget(self.central_widget)

    def interpret(self):
        text = str(self.text_edit.toPlainText())

        # Crée un fichier C contenant l'interprétation du code BrainFuck.
        result = "#include <stdio.h>\n#include <stdlib.h>\n\nint main()\n{\n\t"
        result += "char mem[30000];\n\tchar *ptr = mem;\n\n"

        for c in text:
            if c == '>':
                result += "++ptr;\n"
                continue

            if c == '<':
                result += "--ptr;\n"
                continue

            if c == '+':
                result += "++(*ptr);\n"
                continue

            if c == '-':
                result += "--(*ptr);\n"
                continue

            if c == '.':
                result += "putchar(*ptr);\n"
                continue

            if c == '[':
                result += "while (*ptr) {\n"
                continue

            if c == ']':
                result += "}\n"
                continue

        result += "\treturn 0;\n}\n"

        # Écrit le résultat dans un fichier temporaire.
        with open("/tmp/brainfuck.c", 'w') as f:
            f.write(result)

        out = ""

        # Ouvre un sous-processus qui compile le fichier C.
        try:
            cmd = ["gcc", "/tmp/brainfuck.c", "-o", "/tmp/brainfuck"]
            out = check_output(cmd, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError:
            err = "Une erreur est survenue lors de la compilation du code C !\n"
            self.display_text.setText(err + out)
            return

        # Ouvre un sous-processus qui exécute l'éxecutable.
        try:
            out = check_output(["/tmp/brainfuck"])
        except subprocess.CalledProcessError:
            err = "Une erreur est survenue lors de l'exécution du code C!\n"
            self.display_text.setText(err + out)
            return

        self.display_text.setText(out)


main_window = MainWindow()
main_window.setWindowTitle("BrainFuck")
main_window.show()

sys.exit(app.exec_())
