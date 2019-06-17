# -*- coding: utf-8 -*-

# ##### BEGIN GPL LICENSE BLOCK #####
#
# This program is free software you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program if not, write to the Free Software  Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2015 Kévin Dietrich.
# All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

from screenplay_outline import outline_points
from screenplay_outline import CENTRAL_QUESTION

# Structure d'un scénario
# -- Personnage principal
#    |__Aventure
#       |__Point de contour
#          |__Point d'intrigue


# enumeration
class Act():
    orphelin, errant, guerrier, martyr = range(4)


class PointIntrigue():
    def __init__(self, tooltip, intrigue_desc):
        self.tooltip = tooltip
        self.intrigue_desc = intrigue_desc
        self.index = 0

    def __str__(self):
        text = '\t' + self.tooltip + '\n'
        return text


intrigue_defaut = [
    # I Don't Get No Respect.
    PointIntrigue("Nous rencontrons soit le héros, soit le personnage victime, d'enjeux, soit l'antagoniste.", ""),
    PointIntrigue("Nous voyons le défaut du héros en relation avec le personnage d'enjeux.", ""),
    PointIntrigue("L'antagoniste ou quelqu'un ou quelque chose qui symbolise l'antagoniste.", ""),

    # You know what trouble is?.
    PointIntrigue("Le déflateur ralentit le héros. Il le retire du chemin.", ""),
    PointIntrigue("Événement déclencheur. Le héros est maintenant impliqué émotionnellement.", ""),

    # Calls & Busy Signals.
    PointIntrigue("L'objectif du héros en ce qui concerne le caractère de l'enjeu et / ou l'intérêt d'amour. Le problème du héros est rendu clair au public.", ""),
    PointIntrigue("L'allié (véridique ou involontaire) aide le héros en le propulsant hors du statu quo.", ""),
    PointIntrigue("Le héros semble prêt à aller de l'avant vers le but et / ou le personnage enjeux, mais ne peut pas.", ""),

    # Through the Looking Glass.
    PointIntrigue("Le conflit avec l'antagoniste, ou le déflateur, stoppe le héros ou menace des enjeux émotionnels.", ""),
    PointIntrigue("La profondeur du sentiment entre le héros et le personnage des enjeux ou la gravité de la menace pour les victimes est mis en évidence.", ""),
    PointIntrigue("Le déflateur ou l'antagoniste menace de prendre le caractère d'enjeu du héros.", ""),
    PointIntrigue("Le héros décide qu'il doit agir pour sauver le personnage des enjeux.", ""),

    PointIntrigue("Oui 1", ""),
    PointIntrigue("Non 1", ""),
    PointIntrigue("Oui 2", ""),
    PointIntrigue("Non 2", ""),
    PointIntrigue("Oui 3", ""),
    PointIntrigue("Non 3", ""),
    PointIntrigue("Oui 4", ""),
    PointIntrigue("Non 4", ""),
    PointIntrigue("Oui 5", ""),
    PointIntrigue("Non 5", ""),
    PointIntrigue("Oui 6", ""),
    PointIntrigue("Non 6", ""),
    PointIntrigue("Oui 7", ""),
    PointIntrigue("Non 7", ""),

    PointIntrigue("Oui 8", ""),
    PointIntrigue("Non 8", ""),
    PointIntrigue("Oui 9", ""),
    PointIntrigue("Non 9", ""),
    PointIntrigue("Oui 10", ""),
    PointIntrigue("Non 10", ""),
    PointIntrigue("Oui 11", ""),
    PointIntrigue("Non 11", ""),
    PointIntrigue("Oui 12", ""),
    PointIntrigue("Non 12", ""),
    PointIntrigue("Oui 13", ""),
    PointIntrigue("Non 13", ""),
    PointIntrigue("Oui 14", ""),
    PointIntrigue("Non 14", ""),

    PointIntrigue("Grand Oui", ""),
    PointIntrigue("Non", ""),
    PointIntrigue("Grand Non", ""),
    PointIntrigue("Oui Final", ""),
]


class PointContour():
    def __init__(self):
        self.titre = ""
        self.pages = ""
        self.tooltip = ""
        self.plot_point_start = 0
        self.plot_point_end = 0

        self.intrigue = []

        self.contour_desc = ""

    def __str__(self):
        text = self.titre + " (" + self.pages + "):\n"

        for intrigue in self.intrigue:
            text += str(intrigue)

        return text


class Aventure():
    def __init__(self):
        self.contour = []

    @staticmethod
    def create_default():
        aventure = Aventure()

        for point in outline_points:
            contour = PointContour()
            contour.titre = point.title
            contour.pages = point.pages
            contour.plot_point_start = point.plot_point_start
            contour.plot_point_end = point.plot_point_end

            for i in range(contour.plot_point_start, contour.plot_point_end):
                index = i - 1 if (i > CENTRAL_QUESTION) else i
                contour.intrigue.append(intrigue_defaut[i])

            aventure.contour.append(contour)

        return aventure

    def __str__(self):
        text = "Aventure:\n"

        for contour in self.contour:
            text += str(contour)

        return text


class Personnage():
    def __init__(self):
        # 4 questions.
        self.who_am_i = ""
        self.what_do_i_want = ""
        self.who_stop_me = ""
        self.what_happen_when_fail = ""

        # Archetype.
        self.desc_orphelin = ""
        self.desc_errant = ""
        self.desc_guerrier = ""
        self.desc_martyr = ""

        # Formule.
        self.formule = ""
        # Formule générée comme un indice pour l'utilisateur.
        self.formule_generee = ""

        self.aventure = Aventure()

    @staticmethod
    def create_default():
        personnage = Personnage()
        personnage.aventure = Aventure.create_default()
        return personnage

    def genere_formule(self):
        formule = "Quand un personnage a/fait/veut/obtient ";
        formule += self.desc_orphelin;
        formule += ", il obtient ";
        formule += self.desc_errant;
        formule += ", seulement pour decouvrir que ";
        formule += self.desc_guerrier;
        formule += " arrive desormais et il doit répondre en faisant ";
        formule += self.desc_martyr;
        formule += ".";

        self.formule_generee = formule

        return formule

    def __str__(self):
        text = "Personnage:\n"
        text += "\tQui est-il ? " + self.who_am_i + '\n'
        text += "\tQue veut-il ? " + self.what_do_i_want + '\n'
        text += "\tQui veut l'empêcher d'obtenir ce qu'il veut ? " + self.who_stop_me + '\n'
        text += "\tQu'arrivera-t-il s'il échout ? " + self.what_happen_when_fail + '\n'

        text += "\tStatut d'orphelin: " + self.desc_orphelin + '\n'
        text += "\tStatut d'errant: " + self.desc_errant + '\n'
        text += "\tStatut de guerrier: " + self.desc_guerrier + '\n'
        text += "\tStatut de martyr: " + self.desc_martyr + '\n'

        text += "\tFormule: " + self.formule + '\n'

        text += str(self.aventure)

        return text


class Scenario():
    def __init__(self):
        # Information
        self.name = ""
        self.author = ""
        self.genre = ""
        self.source = ""
        self.start_date = ""

        # Histoire
        self.personnage = Personnage()

    @staticmethod
    def create_default():
        scenario = Scenario()
        scenario.name = "Test"
        scenario.author = "Jean-Scribe"
        scenario.genre = "Porno"
        scenario.source = "Idée Original"
        scenario.personnage = Personnage().create_default()
        return scenario

    def __str__(self):
        text = "Scénario :\n"
        text += "\tNom : " + self.name + '\n'
        text += "\tAuteur : " + self.author + '\n'
        text += "\tDate : " + self.start_date + '\n'
        text += "\tGenre : " + self.genre + '\n'
        text += "\tSource : " + self.source + '\n'
        text += str(self.personnage)
        return text


class Idee():
    def __init__(self):
        # Information
        self.nom = ""
        self.genre = ""
        self.date_creation = ""
        self.desc = ""


class Projet():
    def __init__(self):
        self.scenarios = []
        self.idees = []
